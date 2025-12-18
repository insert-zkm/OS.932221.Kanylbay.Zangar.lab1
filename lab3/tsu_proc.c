#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/version.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>

#define PROCFS_NAME "tsu_proc"

static struct proc_dir_entry *proc_file;

static ssize_t procfile_read(struct file *file_pointer,
                             char __user *buffer,
                             size_t buffer_length,
                             loff_t *offset)
{
    char s[18] = "Preps for nextlab\n";
    int len = 18;
    static int finished = 0;

    if (finished) {
        finished = 0;
        return 0;
    }

    if (buffer_length < len)
        return -EINVAL;

    if (copy_to_user(buffer, s, len))
        return -EFAULT;

    finished = 1;

    pr_info("procfile read %s\n",
            file_pointer->f_path.dentry->d_name.name);
    return len;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
static const struct proc_ops proc_file_fops = {
    .proc_read = procfile_read,
};
#else
static const struct file_operations proc_file_fops = {
    .read = procfile_read,
};
#endif

static int __init tsu_init(void)
{
    pr_info("Welcome to the Tomsk State University\n");

    proc_file = proc_create(PROCFS_NAME, 0444, NULL, &proc_file_fops);
    if (!proc_file) {
        pr_err("Cannot create /proc/%s\n", PROCFS_NAME);
        return -ENOMEM;
    }

    return 0;
}

static void __exit tsu_exit(void)
{
    if (proc_file)
        proc_remove(proc_file);

    pr_info("Tomsk State University forever!\n");
}

module_init(tsu_init);
module_exit(tsu_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("TSU student");