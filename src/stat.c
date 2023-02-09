#include "stat.h"

#define TASK_STATE_FIELD state
#define TASK_STATE_SPEC "%ld"

static inline long convert_to_kb(const long n) {
    return n << (PAGE_SHIFT - 10);
}

void print_memory_statistics(struct seq_file *m) {
    struct sysinfo info;
    long long secs;
	long sys_occupied, apps_occupied;
    int i;

    ENTER_LOG();

    si_meminfo(&info);
    show_int_message(m, "Memory total: \t%ld kB\n", convert_to_kb(info.totalram));

    for (i = 0; i < mem_info_calls_cnt; i++) {
        secs = mem_info_array[i].time_secs;
        show_int3_message(m, "\nTime %.2llu:%.2llu:%.2llu\n", (secs / 3600 + 3) % 24, secs / 60 % 60, secs % 60);
        show_int_message(m, "Free:      \t\t\t%ld kB\n", convert_to_kb(mem_info_array[i].free));
        show_int_message(m, "Available: \t\t\t%ld kB\n", convert_to_kb(mem_info_array[i].available));
		sys_occupied = convert_to_kb(info.totalram) - convert_to_kb(mem_info_array[i].available);
		show_int_message(m, "Occupied: \t%ld kB\n", sys_occupied);
    }

    EXIT_LOG();
}
