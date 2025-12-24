#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/version.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/timekeeping.h>
#include <linux/time64.h>


#define PROCFS_NAME "tsu_proc"

static struct proc_dir_entry *proc_file;
	
static u64 isqrt_u64(u64 x)
{
	u64 op = x;
	u64 res = 0;
	u64 one = (u64)1 << 62;

	while (one > op)
		one >>= 2;

	while (one != 0) {
		if (op >= res + one) {
			op  -= res + one;
			res += one << 1;
		}
		res >>= 1;
		one >>= 2;
	}
	return res;
}

static ssize_t procfile_read(struct file *file_pointer,
							 char __user *buffer,
							 size_t buffer_length,
							 loff_t *offset)
{
	static int finished = 0;
	char s[64];
	int len;

	const u64 h_m  = 570000;   // ~570k km above the ground
	const u32 g_mm = 9810;	 // 9810 мм/с^2 

	u64 num; // for подкоренного выражения
	u64 t_ms;
	u32 t_sec;
	u32 t_min;

	if (finished) {
		finished = 0;
		return 0;
	}

	// t = sqrt((2* h * 10^9) / g)
	num = 2ULL * h_m * 1000000000ULL;
	num = div_u64(num, g_mm);
	t_ms = isqrt_u64(num);

	t_sec = (u32)(t_ms / 1000ULL);
	t_min = t_sec / 60U;

	time64_t now = ktime_get_real_seconds();
	pr_info("now=%lld\n", (long long)now);
    time64_t impact = now + t_sec;

    struct tm tm_impact;
    time64_to_tm(impact, 0, &tm_impact);

	len = scnprintf(s, sizeof(s),
        "Impact (UTC): %04ld-%02d-%02d %02d:%02d:%02d\n",
        (long)(tm_impact.tm_year + 1900),
        tm_impact.tm_mon + 1,
        tm_impact.tm_mday,
        tm_impact.tm_hour,
        tm_impact.tm_min,
        tm_impact.tm_sec);

	if (buffer_length < len)
		return -EINVAL;

	if (copy_to_user(buffer, s, len))
		return -EFAULT;

	finished = 1;

	pr_info("procfile read %s: t=%u minutes\n",
			file_pointer->f_path.dentry->d_name.name,
			t_min);

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