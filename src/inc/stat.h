#ifndef __STAT_H__
#define __STAT_H__

#include <linux/mm.h>
#include <linux/seq_file.h>

#include "memory.h"

void print_memory_statistics(struct seq_file *m);

#endif
