/*
 * lib/kprobe-coverage.c
 *
 * Copyright (C) 2014 Simon Kagstrom
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
#include <linux/mutex.h>
#include <linux/debugfs.h>
#include <linux/workqueue.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

struct kprobe_coverage
{
	struct dentry *debugfs_root;

	wait_queue_head_t wq;

	/* Each coverage entry is on exactly one of these lists */
	struct list_head deferred_list; /* Probes for not-yet-loaded-modules */

	struct list_head pending_list;  /* Probes which has not yet triggered */
	struct list_head hit_list;      /* Triggered probes awaiting readout */

	const char *module_names[32];
	unsigned int name_count;

	struct mutex lock;
};

struct kprobe_coverage_entry
{
	struct kprobe kp;
	int name_index; /* an index into the name table above (0 is always the kernel */
	unsigned long base_addr;

	struct list_head lh;
	struct work_struct work;
};

static struct kprobe_coverage *global_kpc;

/* Return index into module name vector. Should be called with
 * mutex held. */
static int module_name_to_index(struct kprobe_coverage *kpc,
		const char *module_name)
{
	int i;

	/* The first is always the kernel (NULL) */
	if (module_name == NULL)
		return 0;

	for (i = 1; i < kpc->name_count; i++) {
		if (strcmp(kpc->module_names[i], module_name) == 0)
			return i;
	}

	return -1;
}

static int kpc_allocate_module_name_index(struct kprobe_coverage *kpc,
		const char *module_name)
{
	int index;

	mutex_lock(&kpc->lock);
	index = module_name_to_index(kpc, module_name);

	/* Not found? If so allocate new entry unless vector is full */
	if (index < 0 && kpc->name_count < ARRAY_SIZE(kpc->module_names)) {
		index = kpc->name_count;

		kpc->module_names[index] = kstrdup(module_name, GFP_KERNEL);
		kpc->name_count++;
	}
	mutex_unlock(&kpc->lock);

	return index;

}

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
	struct kprobe_coverage *kpc = global_kpc;

	unregister_kprobe(&entry->kp);

	/* Move from pending list to the hit list */
	mutex_lock(&kpc->lock);

	list_del(&entry->lh);
	list_add_tail(&entry->lh, &kpc->hit_list);

	/* Wake up the listener */
	wake_up(&kpc->wq);

	mutex_unlock(&kpc->lock);
}


static void kpc_free_entry(struct kprobe_coverage_entry *entry)
{
	kfree(entry);
}

static struct kprobe_coverage_entry *kpc_new_entry(struct kprobe_coverage *kpc,
		const char *module_name, struct module *mod, unsigned long where)
{
	struct kprobe_coverage_entry *out;
	unsigned long base_addr = 0;

	out = kzalloc(sizeof(*out), GFP_KERNEL);
	if (!out)
		return NULL;

	/* Setup the base address if we know it, otherwise keep at 0 */
	if (mod)
		base_addr = (unsigned long)mod->module_core;

	out->name_index = kpc_allocate_module_name_index(kpc, module_name);
	out->base_addr = base_addr;
	out->kp.addr = (void *)(base_addr + where);
	out->kp.pre_handler = kpc_pre_handler;
	INIT_WORK(&out->work, kpc_probe_work);

	return out;
}

/* Called with the lock held */
static int enable_probe(struct kprobe_coverage *kpc,
		struct kprobe_coverage_entry *entry)
{
	int err;

	if ( (err = register_kprobe(&entry->kp)) < 0)
		return -EINVAL;

	list_add(&entry->lh, &kpc->pending_list);

	if (enable_kprobe(&entry->kp) < 0) {
		unregister_kprobe(&entry->kp);

		return -EINVAL;
	}

	return 0;
}

/* Called with the lock held */
static void defer_probe(struct kprobe_coverage *kpc,
		struct kprobe_coverage_entry *entry)
{
	list_add(&entry->lh, &kpc->deferred_list);
}

static void kpc_add_probe(struct kprobe_coverage *kpc, const char *module_name,
		unsigned long where)
{
	struct module *module = NULL;
	struct kprobe_coverage_entry *entry;
	int rv = 0;

	/* Lookup the module which should be instrumented */
	if (module_name) {
		preempt_disable();
		module = find_module(module_name);
		preempt_enable();
	}

	entry = kpc_new_entry(kpc, module_name, module, where);

	/* Three cases:
	 *
	 * 1. pending module - module_name is !NULL, module is NULL: Defer
	 *    instrumentation
	 *
	 * 2. vmlinux - module_name and module is NULL: Instrument directly
	 *
	 * 3. loaded module - module_name and module is !NULL: Instrument directly
	 */
	mutex_lock(&kpc->lock);
	if (module_name && !module)
		defer_probe(kpc, entry);
	else
		rv = enable_probe(kpc, entry);
	mutex_unlock(&kpc->lock);

	/* Probe enabling might fail, just free the entry */
	if (rv < 0)
		kpc_free_entry(entry);
}

/* Called with lock held */
static void clear_list(struct kprobe_coverage *kpc,
		struct list_head *list, int do_unregister)
{
	struct list_head *iter;
	struct list_head *tmp;

	list_for_each_safe(iter, tmp, list) {
		struct kprobe_coverage_entry *entry;

		entry = (struct kprobe_coverage_entry *)container_of(iter,
				struct kprobe_coverage_entry, lh);
		if (do_unregister)
			unregister_kprobe(&entry->kp);
		list_del(&entry->lh);
		kpc_free_entry(entry);
	}
}

static void kpc_clear(struct kprobe_coverage *kpc)
{
	int i;

	/* Free everything on the lists */
	mutex_lock(&kpc->lock);

	clear_list(kpc, &kpc->deferred_list, 0);
	INIT_LIST_HEAD(&kpc->deferred_list);

	clear_list(kpc, &kpc->hit_list, 1);
	clear_list(kpc, &kpc->pending_list, 1);

	INIT_LIST_HEAD(&kpc->pending_list);
	INIT_LIST_HEAD(&kpc->hit_list);

	mutex_unlock(&kpc->lock);

	for (i = 0; i < kpc->name_count; i++) {
		kfree(kpc->module_names[i]);

		kpc->module_names[i] = NULL;
	}
	/* Kernel is still at 0 */
	kpc->name_count = 1;
}

static void *kpc_unlink_next(struct kprobe_coverage *kpc)
{
	struct kprobe_coverage_entry *entry;
	int err;

	/* Wait for something to arrive on the hit list, abort on signal */
	err = wait_event_interruptible(kpc->wq, !list_empty(&kpc->hit_list));
	if (err < 0)
		return NULL;
	mutex_lock(&kpc->lock);
	entry = list_first_entry(&kpc->hit_list, struct kprobe_coverage_entry, lh);
	list_del(&entry->lh);
	mutex_unlock(&kpc->lock);

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
	const char *module_name;
	unsigned long addr;
	char buf[MODULE_NAME_LEN + 20];
	int n;

	/* Wait for an entry */
	entry = kpc_unlink_next(kpc);
	if (!entry)
		return 0;

	module_name = kpc->module_names[entry->name_index];

	/* Write it out to the user buffer */
	addr = (unsigned long)entry->kp.addr - entry->base_addr;

	n = snprintf(buf, sizeof(buf), "%s%s0x%016lx\n",
			module_name ? module_name : "",
			module_name ? ":" : "",
			addr);

	kpc_free_entry(entry);

	/* Should never happen if snprintf works correctly */
	if (n >= sizeof(buf))
		return -EINVAL;

	if (copy_to_user(user_buf + *off, buf, n))
		n = -EFAULT;

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
	buf = (char *)kzalloc(count + 1, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	if (copy_from_user(buf, user_buf, count)) {
		kfree(buf);
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

	kfree(buf);
	*off += out;

	return out;
}

static void kpc_handle_coming_module(struct kprobe_coverage *kpc,
		struct module *mod)
{
	struct list_head *iter;
	struct list_head *tmp;

	mutex_lock(&kpc->lock);
	list_for_each_safe(iter, tmp, &kpc->deferred_list) {
		struct kprobe_coverage_entry *entry;

		entry = (struct kprobe_coverage_entry *)container_of(iter,
				struct kprobe_coverage_entry, lh);

		if (module_name_to_index(kpc, mod->name) != entry->name_index)
			continue;

		/* Move the deferred entry to the pending list and enable */
		list_del(&entry->lh);
		entry->base_addr = (unsigned long)mod->module_core;
		entry->kp.addr += entry->base_addr;

		if (enable_probe(kpc, entry) < 0)
			kpc_free_entry(entry);
	}
	mutex_unlock(&kpc->lock);
}

static void kpc_handle_going_module(struct kprobe_coverage *kpc,
		struct module *mod)
{
	struct list_head *iter;
	struct list_head *tmp;

	mutex_lock(&kpc->lock);
	list_for_each_safe(iter, tmp, &kpc->pending_list) {
		struct kprobe_coverage_entry *entry;

		entry = (struct kprobe_coverage_entry *)container_of(iter,
				struct kprobe_coverage_entry, lh);

		if (module_name_to_index(kpc, mod->name) != entry->name_index)
			continue;

		/* Remove pending entries for the current module */
		unregister_kprobe(&entry->kp);
		list_del(&entry->lh);
	}
	mutex_unlock(&kpc->lock);
}

static int kpc_module_notifier(struct notifier_block *nb,
		unsigned long event, void *data)
{
	struct module *mod = data;

	if (event == MODULE_STATE_COMING)
		kpc_handle_coming_module(global_kpc, mod);
	else if (event == MODULE_STATE_GOING)
		kpc_handle_going_module(global_kpc, mod);

	return 0;
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

static struct notifier_block kpc_module_notifier_block =
{
	.notifier_call = kpc_module_notifier,
};

static int __init kpc_init(struct kprobe_coverage *kpc)
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

	if (register_module_notifier(&kpc_module_notifier_block) < 0)
		goto out_files;

	INIT_LIST_HEAD(&kpc->pending_list);
	INIT_LIST_HEAD(&kpc->hit_list);
	INIT_LIST_HEAD(&kpc->deferred_list);

	init_waitqueue_head(&kpc->wq);
	mutex_init(&kpc->lock);

	/* The kernel is always index 0 */
	kpc->name_count = 1;

	return 0;

out_files:
	debugfs_remove_recursive(kpc->debugfs_root);

	return -EINVAL;
}

static int __init kpc_init_module(void)
{
	int out;

	global_kpc = kzalloc(sizeof(*global_kpc), GFP_KERNEL);
	if (!global_kpc)
		return -ENOMEM;

	out = kpc_init(global_kpc);
	if (out < 0) {
		kfree(global_kpc);

		global_kpc = NULL;
	}

	return out;
}

static void __exit kpc_exit_module(void)
{
	kpc_clear(global_kpc);

	debugfs_remove_recursive(global_kpc->debugfs_root);
	unregister_module_notifier(&kpc_module_notifier_block);

	kfree(global_kpc);
	global_kpc = NULL;
}

module_init(kpc_init_module);
module_exit(kpc_exit_module);
MODULE_AUTHOR("Simon Kagstrom <simon.kagstrom@gmail.com>");
MODULE_DESCRIPTION("Code coverage through kprobes and debugfs");
MODULE_LICENSE("GPL");
