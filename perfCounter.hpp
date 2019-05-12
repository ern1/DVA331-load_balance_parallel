#ifndef __PERFCOUNTER_HPP__
#define __PERFCOUNTER_HPP__

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/unistd.h>
#include <linux/perf_event.h>
//#include <linux/smp.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/utsname.h>
#include <sys/ioctl.h>
#include <sys/file.h>

static perf_event_attr event_attr;
static int* fd;

void init_perf_events(int num_cores)
{
    /*  Finns andra event som kanske är mer relevanta om man sätter type till PERF_TYPE_HW_CACHE,
        men PERF_COUNT_HW_CACHE_MISSES används i MemGuard så troligtvis bäst att använda samma. 
        Mer info: http://web.eece.maine.edu/~vweaver/projects/perf_events/perf_event_open.html */
    memset(&event_attr, 0, sizeof(struct perf_event_attr));
	event_attr.type             = PERF_TYPE_HARDWARE;
    event_attr.config           = PERF_COUNT_HW_CACHE_MISSES;
    //event_attr.type             = PERF_TYPE_HW_CACHE;
    //event_attr.config           = PERF_COUNT_HW_CACHE_LL;
	event_attr.size             = sizeof(struct perf_event_attr);
    event_attr.pinned           = 1,  // Counter should always be on the CPU if at all possible (= 1) ---> Vettefan om denna ska vara satt till 1..
	event_attr.disabled         = 0;  // Start counter as enabled (= 0)
	event_attr.exclude_kernel   = 1;  // Don't count events that happen in kernel-space (= 1)

    fd = new int[num_cores];
}

static int perf_event_open(pid_t pid, int cpu, int grp_fd, unsigned long flags)
{
    return syscall(__NR_perf_event_open, &event_attr, pid, cpu, grp_fd, flags);
}

// Returns a file descriptor that allows measuring performance information
void start_counter(int core_id)
{
    // pid == -1, cpu >= 0 - Measures all processes/threads on the specified CPU.
    fd[core_id] = perf_event_open(-1, core_id, -1, 0);

    if(fd < 0)
        perror("Error opening performance counter");
}

// vet inte om det behöver vara en unsigned long long
unsigned long long read_counter(int core_id)
{
    long long unsigned rv;

    if(read(fd[core_id], &rv, sizeof(rv)) == 0)
        perror("Error reading performance counter");

    return rv;
}

void stop_counter(int core_id)
{
    if(close(fd[core_id]) != 0)
        perror("Error stopping performance counter");
}

#endif