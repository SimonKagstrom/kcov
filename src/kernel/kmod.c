/*
 * lib/kprobe-coverage.c
 *
 * Copyright (C) 2012 Simon Kagstrom
 *
 * Author: Simon Kagstrom <simon.kagstrom@gmail.com>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 */
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/list.h>
#include <linux/debugfs.h>
#include <linux/workqueue.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/fs.h>
#include <linux/vmalloc.h>
#include <linux/uaccess.h>

struct kprobe_coverage
{
	struct dentry *debugfs_root;

	wait_queue_head_t wq;

	/* Each coverage entry is on exactly one of these lists */
	struct list_head pending_list;
	struct list_head hit_list;
	spinlock_t list_lock;
};

struct kprobe_coverage_entry
{
	struct kprobe_coverage *parent;
	unsigned long base_addr;
	const char *module_name;

	struct kprobe kp;
	struct work_struct work;

	struct list_head lh;
};


static int kpc_pre_handler(struct kprobe *kp, struct pt_regs *regs)
{
	struct kprobe_coverage_entry *entry = (struct kprobe_coverage_entry *)
			container_of(kp, struct kprobe_coverage_entry, kp);

	/* Schedule it for removal */
	schedule_work(&entry->work);

	return 0;
}

static void kpc_probe_work(struct work_struct *work)
{
	struct kprobe_coverage_entry *entry = (struct kprobe_coverage_entry *)
			container_of(work, struct kprobe_coverage_entry, work);
	struct kprobe_coverage *kpc = entry->parent;

	/* Move from pending list to the hit list */
	spin_lock(&kpc->list_lock);
	list_del(&entry->lh);
	list_add_tail(&entry->lh, &kpc->hit_list);
	unregister_kprobe(&entry->kp);
	spin_unlock(&kpc->list_lock);

	/* Wake up the listener */
	wake_up(&kpc->wq);
}


static void kpc_free_entry(struct kprobe_coverage_entry *entry)
{
	vfree(entry->module_name);
	vfree(entry);
}

static void kpc_new_entry(struct kprobe_coverage *kpc, const char *module_name,
		unsigned long base_addr, unsigned long where)
{
	struct kprobe_coverage_entry *out;
	char *name;
	int err;

	name = vmalloc(strlen(module_name) + 1);
	if (!name)
		return;

	out = vmalloc(sizeof(*out));
	if (!out) {
		vfree(name);
		return;
	}
	memset(out, 0, sizeof(*out));
	strcpy(name, module_name);
	name[strlen(module_name)] = '\0';

	out->module_name = name;
	out->base_addr = base_addr;
	out->parent = kpc;
	out->kp.addr = (void *)(base_addr + where);
	out->kp.pre_handler = kpc_pre_handler;
	INIT_WORK(&out->work, kpc_probe_work);

	if ( (err = register_kprobe(&out->kp)) < 0)
		goto err;

	spin_lock(&kpc->list_lock);
	list_add(&out->lh, &kpc->pending_list);
	spin_unlock(&kpc->list_lock);

	if (enable_kprobe(&out->kp) < 0) {
		unregister_kprobe(&out->kp);
		goto err;
	}

	return;
err:
	kpc_free_entry(out);
}

static void kpc_add_probe(struct kprobe_coverage *kpc,
		const char *module_name, unsigned long where)
{
	static char cur_module[MODULE_NAME_LEN];
	static unsigned long cur_base_addr;

	/* Cache the module data to avoid having to iterate through all
	 * the modules each time.
	 */
	if (module_name == NULL) {
		/* The kernel itself */
		strcpy(cur_module, "");
		cur_base_addr = 0;
	}
	else if (strcmp(cur_module, module_name) != 0) {
		struct module *module;

		preempt_disable();
		module = find_module(module_name);
		if (module) {
			strncpy(cur_module, module->name, MODULE_NAME_LEN);
			cur_base_addr = (unsigned long)module->module_core;
		}
		preempt_enable();
		if (!module)
			return;
	}

	/* If we are unlucky we might end up with stale data here if the
	 * module has been removed. However, it just means that the kprobe
	 * insertion will fail or that we instrument the wrong module, i.e.,
	 * generating confusing but harmless data.
	 */
	kpc_new_entry(kpc, cur_module, cur_base_addr, where);
}

static void kpc_clear_list(struct kprobe_coverage *kpc,
		struct list_head *list)
{
	struct list_head *iter;

	list_for_each(iter, list) {
		struct kprobe_coverage_entry *entry;

		entry = (struct kprobe_coverage_entry *)container_of(iter,
				struct kprobe_coverage_entry, lh);
		unregister_kprobe(&entry->kp);
		kpc_free_entry(entry);
	}
}

static void kpc_clear(struct kprobe_coverage *kpc)
{
	/* Free everything on the two lists */
	spin_lock(&kpc->list_lock);

	kpc_clear_list(kpc, &kpc->hit_list);
	kpc_clear_list(kpc, &kpc->pending_list);

	INIT_LIST_HEAD(&kpc->pending_list);
	INIT_LIST_HEAD(&kpc->hit_list);
	spin_unlock(&kpc->list_lock);
}

static void *kpc_unlink_next(struct kprobe_coverage *kpc)
{
	struct kprobe_coverage_entry *entry;
	int err;

	/* Wait for something to arrive on the hit list, abort on signal */
	err = wait_event_interruptible(kpc->wq, !list_empty(&kpc->hit_list));
	if (err < 0)
		return NULL;
	spin_lock(&kpc->list_lock);
	entry = list_first_entry(&kpc->hit_list, struct kprobe_coverage_entry, lh);
	list_del(&entry->lh);
	spin_unlock(&kpc->list_lock);

	return entry;
}


static int kpc_control_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private; /* kpc */

	return 0;
}

static int kpc_show_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;

	return 0;
}

static ssize_t kpc_show_read(struct file *file, char __user *user_buf,
		size_t count, loff_t *off)
{
	struct kprobe_coverage *kpc = file->private_data;
	struct kprobe_coverage_entry *entry;
	unsigned long addr;
	char *buf;
	size_t buf_size;
	int n = -ENOMEM;

	/* Wait for an entry */
	entry = kpc_unlink_next(kpc);
	if (!entry)
		return 0;

	buf_size = strlen(entry->module_name) + 24;
	buf = vmalloc(buf_size);
	if (!buf)
		goto out;

	/* Write it out to the user buffer */
	memset(buf, 0, buf_size);
	addr = (unsigned long)entry->kp.addr - entry->base_addr;
	n = snprintf(buf, buf_size, "%s:0x%lx\n", entry->module_name, addr);
	if (copy_to_user(user_buf + *off, buf, n))
		n = -EFAULT;

out:
	vfree(buf);
	kpc_free_entry(entry);

	return n;
}

static ssize_t kpc_control_write(struct file *file, const char __user *user_buf,
		size_t count, loff_t *off)
{
	struct kprobe_coverage *kpc = file->private_data;
	ssize_t out = 0;
	char *line;
	char *buf;
	char *p;

	/* Assure it's NULL terminated */
	buf = (char *)vzalloc(count + 1);
	if (!buf)
		return -ENOMEM;

	if (copy_from_user(buf, user_buf, out)) {
		vfree(buf);
		return -EFAULT;
	}
	p = buf;

	while ( (line = strsep(&p, "\r\n")) ) {
		unsigned long addr;
		char *module = NULL; /* Assume for the kernel */
		char *colon;
		char *addr_p;
		char *endp;

		if (!*line || !p)
			break;

		out += p - line;

		if (strcmp(line, "clear") == 0) {
			kpc_clear(kpc);
			break;
		}
		colon = strstr(line, ":");
		if (colon) {
			/* For a module */
			addr_p = colon + 1;
			*colon = '\0';
			module = line;
		} else
			addr_p = line;

		addr = simple_strtoul(addr_p, &endp, 16);
		if (endp == addr_p || *endp != '\0')
			continue;

		kpc_add_probe(kpc, module, addr);
	}

	vfree(buf);
	*off += out;

	return out;
}

static const struct file_operations kpc_control_fops =
{
	.owner = THIS_MODULE,
	.open = kpc_control_open,
	.write = kpc_control_write
};

static const struct file_operations kpc_show_fops =
{
	.owner = THIS_MODULE,
	.open = kpc_show_open,
	.read = kpc_show_read,
};

static int __init kpc_init_one(struct kprobe_coverage *kpc)
{
	/* Create debugfs entries */
	kpc->debugfs_root = debugfs_create_dir("kprobe-coverage", NULL);
	if (!kpc->debugfs_root) {
		printk(KERN_ERR "kprobe-coverage: creating root dir failed\n");
		return -ENODEV;
	}
	if (!debugfs_create_file("control", 0200, kpc->debugfs_root, kpc,
			&kpc_control_fops))
		goto out_files;

	if (!debugfs_create_file("show", 0400, kpc->debugfs_root, kpc,
			&kpc_show_fops))
		goto out_files;

	INIT_LIST_HEAD(&kpc->pending_list);
	INIT_LIST_HEAD(&kpc->hit_list);
	init_waitqueue_head(&kpc->wq);
	spin_lock_init(&kpc->list_lock);

	return 0;

out_files:
	debugfs_remove_recursive(kpc->debugfs_root);

	return -EINVAL;
}

static struct kprobe_coverage *global_kpc;
static int __init kpc_init(void)
{
	int out;

	global_kpc = vmalloc(sizeof(*global_kpc));
	if (!global_kpc)
		return -ENOMEM;
	memset(global_kpc, 0, sizeof(*global_kpc));

	out = kpc_init_one(global_kpc);
	if (out < 0)
		vfree(global_kpc);

	return out;
}

static void __exit kpc_exit(void)
{
	debugfs_remove_recursive(global_kpc->debugfs_root);
	kpc_clear(global_kpc);

	vfree(global_kpc);
	global_kpc = NULL;
}

module_init(kpc_init);
module_exit(kpc_exit);
MODULE_AUTHOR("Simon Kagstrom <simon.kagstrom@gmail.com>");
MODULE_DESCRIPTION("Code coverage through kprobes and debugfs");
MODULE_LICENSE("GPL");
