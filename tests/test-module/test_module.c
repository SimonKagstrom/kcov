/*
 *
 * Copyright (C) 2012 Simon Kagstrom
 *
 * Author: Simon Kagstrom <simon.kagstrom@gmail.com>
 *
 * Most of the stuff comes from Jonathan Corbets seqfile example here:
 *
 *   http://lwn.net/Articles/22359/
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 */
#include <linux/module.h>
#include <linux/init.h>

#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>

static int hz_show(struct seq_file *m, void *v)
{
    seq_printf(m, "%d\n", HZ);

    return 0;
}

static int hz_open(struct inode *inode, struct file *file)
{
    return single_open(file, hz_show, NULL);
}

static const struct file_operations ct_file_ops = {
    .owner	= THIS_MODULE,
    .open	= hz_open,
    .read	= seq_read,
    .llseek	= seq_lseek,
    .release = single_release,
};

static int __init test_init(void)
{
	struct proc_dir_entry *entry;

	printk(KERN_INFO "initing test module!\n");

	entry = create_proc_entry("test_module", 0, NULL);
	if (entry)
		entry->proc_fops = &ct_file_ops;
	return 0;
}

static void __exit test_exit(void)
{
	remove_proc_entry("test_module", NULL);
}

module_init(test_init);
module_exit(test_exit);
MODULE_AUTHOR("Simon Kagstrom <simon.kagstrom@gmail.com>");
MODULE_DESCRIPTION("Test");
MODULE_LICENSE("GPL");
