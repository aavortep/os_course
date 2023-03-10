#include <linux/module.h>
#include <linux/proc_fs.h> 
#include <linux/time.h>
#include <linux/kthread.h>

#include "hooks.h"
#include "memory.h"
#include "stat.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Petrova Anna");
MODULE_DESCRIPTION("A utility for monitoring RAM usage and number of syscalls");

static struct proc_dir_entry *proc_root = NULL;
static struct proc_dir_entry *proc_mem_file = NULL, *proc_syscall_file = NULL;
static struct task_struct *worker_task = NULL;

extern ktime_t start_time;
/* default syscall range value is 10 min */
static ktime_t syscalls_range_in_seconds = 600;

static int show_memory(struct seq_file *m, void *v) {
    print_memory_statistics(m);
    return 0;
}

static int proc_memory_open(struct inode *sp_inode, struct file *sp_file) {
    return single_open(sp_file, show_memory, NULL);
}

static int show_syscalls(struct seq_file *m, void *v) {
    print_syscall_statistics(m, start_time, syscalls_range_in_seconds);
    return 0;
}

static int proc_syscalls_open(struct inode *sp_inode, struct file *sp_file) {
    return single_open(sp_file, show_syscalls, NULL);
}

static int proc_release(struct inode *sp_node, struct file *sp_file) {
    return 0;
}

mem_info_t mem_info_array[MEMORY_ARRAY_SIZE];
int mem_info_calls_cnt;

int memory_cnt_task_handler_fn(void *args) {
    struct sysinfo i;
    struct timespec64 t;

    ENTER_LOG();

	allow_signal(SIGKILL);

	while (!kthread_should_stop()) {
        si_meminfo(&i);

        ktime_get_real_ts64(&t);

        mem_info_array[mem_info_calls_cnt].free = i.freeram;
        mem_info_array[mem_info_calls_cnt].available = si_mem_available();
        mem_info_array[mem_info_calls_cnt].shared = i.sharedram;
        mem_info_array[mem_info_calls_cnt].buffers = i.bufferram;
        mem_info_array[mem_info_calls_cnt].totalhigh = i.totalhigh;
        mem_info_array[mem_info_calls_cnt].freehigh = i.freehigh;
        mem_info_array[mem_info_calls_cnt++].time_secs = t.tv_sec;

        ssleep(1);

		if (signal_pending(worker_task)) {
			break;
        }
	}

    EXIT_LOG();
	do_exit(0);
	return 0;
}

#define CHAR_TO_INT(ch) (ch - '0')

static ktime_t convert_strf_to_seconds(char buf[]) {
    /* time format: xxhyymzzs. For example: 01h23m45s */
    ktime_t hours, min, secs;

    hours = CHAR_TO_INT(buf[0]) * 10 + CHAR_TO_INT(buf[1]);
    min = CHAR_TO_INT(buf[3]) * 10 + CHAR_TO_INT(buf[4]);
    secs = CHAR_TO_INT(buf[6]) * 10 + CHAR_TO_INT(buf[7]);

    return hours * 60 * 60 + min * 60 + secs;
}

static ssize_t proc_syscall_write(struct file *file, const char __user *buf, size_t len, loff_t *ppos) {
    char syscalls_time_range[10];

    ENTER_LOG();

    if (copy_from_user(&syscalls_time_range, buf, len) != 0) {
        EXIT_LOG()
        return -EFAULT;
    }

    syscalls_range_in_seconds = convert_strf_to_seconds(syscalls_time_range);

    EXIT_LOG();
    return len;
}

static const struct proc_ops mem_ops = {
    proc_read: seq_read,
    proc_open: proc_memory_open,
    proc_release: proc_release,
};

static const struct proc_ops syscalls_ops = {
    proc_read: seq_read,
    proc_open: proc_syscalls_open,
    proc_release: proc_release,
    proc_write: proc_syscall_write,
};

static void cleanup(void) {
    ENTER_LOG();

    if (worker_task) {
		kthread_stop(worker_task);
    }

    if (proc_mem_file != NULL) {
        remove_proc_entry("memory", proc_root);
    }

    if (proc_syscall_file != NULL) {
        remove_proc_entry("syscalls", proc_root);
    }

    if (proc_root != NULL) {
        remove_proc_entry(MODULE_NAME, NULL);
    }

    remove_hooks();

    EXIT_LOG();
}

static int proc_init(void) {
    ENTER_LOG();

    if ((proc_root = proc_mkdir(MODULE_NAME, NULL)) == NULL) {
        goto err;
    }

    if ((proc_mem_file = proc_create("memory", 066, proc_root, &mem_ops)) == NULL) {
        goto err;
    }

    if ((proc_syscall_file = proc_create("syscalls", 066, proc_root, &syscalls_ops)) == NULL)
    {
        goto err;
    }

    EXIT_LOG();
    return 0;

err:
    cleanup();
    EXIT_LOG();
    return -ENOMEM;
}

static int __init md_init(void) {
    int rc;
    int cpu;

    ENTER_LOG();

    if ((rc = proc_init())) {
        return rc;
    }

    if ((rc = install_hooks())) {
        cleanup();
        return rc;
    }

    start_time = ktime_get_boottime_seconds();

    cpu = get_cpu();
	worker_task = kthread_create(memory_cnt_task_handler_fn, NULL, "memory counter thread");
	kthread_bind(worker_task, cpu);

    if (worker_task == NULL) {
        cleanup();
        return -1;
    }

    wake_up_process(worker_task);

    printk("%s: module loaded\n", MODULE_NAME);
    EXIT_LOG();

    return 0;
}

static void __exit md_exit(void) { 
    cleanup();

    printk("%s: module unloaded\n", MODULE_NAME); 
}

module_init(md_init);
module_exit(md_exit);

