#ifndef __PERFCOUNTER_HPP__
#define __PERFCOUNTER_HPP__

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/unistd.h>
#include <linux/perf_event.h>
//#include <linux/smp.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/utsname.h>
#include <sys/ioctl.h>
#include <sys/file.h>

static struct perf_event_attr event_attr;
static int* fd;
static int num_fd;

static int perf_event_open(pid_t pid, int cpu, int grp_fd, unsigned long flags)
{
    return syscall(__NR_perf_event_open, &event_attr, pid, cpu, grp_fd, flags);
}

void init_counters(int num_cores)
{
    fd = new int[num_cores];
    num_fd = num_cores;

    /*  http://web.eece.maine.edu/~vweaver/projects/perf_events/perf_event_open.html */
    memset(&event_attr, 0, sizeof(struct perf_event_attr));
	event_attr.type             = PERF_TYPE_HARDWARE;
    event_attr.config           = PERF_COUNT_HW_CACHE_MISSES;
    //event_attr.type             = PERF_TYPE_HW_CACHE;
    //event_attr.config           = PERF_COUNT_HW_CACHE_LL | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
	event_attr.size             = sizeof(struct perf_event_attr);
    event_attr.pinned           = 1;  // Counter should always be on the CPU if at all possible (= 1) ---> Vettefan om denna ska vara satt till 1..
	event_attr.disabled         = 0;
    
    for(int i = 0; i < num_cores; i++)
    {
        // pid == -1, cpu >= 0 - Measures all processes/threads on the specified CPU.
        fd[i] = perf_event_open(-1, i, -1, 0);

        if(fd[i] < 0)
            perror("Error opening performance counter");
    }
}

void reset_counter(int core_id)
{
    ioctl(fd[core_id], PERF_EVENT_IOC_RESET, 0);
    //ioctl(fd[core_id], PERF_EVENT_IOC_ENABLE, 0);
}

// vet inte om det behöver vara en unsigned long long
unsigned long long read_counter(int core_id)
{
    unsigned long long rv;
    if(read(fd[core_id], &rv, sizeof(rv)) == 0)
        perror("Error reading performance counter");

    return rv;
}

void stop_counters()
{
    for(int i = 0; i < num_fd; i++)
    {
        if(close(fd[i]) != 0)
            perror("Error stopping performance counter");
    }
}

#endif


/*
event = perf_event_create_kernel_counter(&sched_perf_hw_attr, cpu, NULL, NULL);

if (!event)
    return NULL;

if (IS_ERR(event)) {
    // vary the KERN level based on the returned errno
    if (PTR_ERR(event) == -EOPNOTSUPP)
        pr_info("cpu%d. not supported\n", cpu);
    else if (PTR_ERR(event) == -ENOENT)
        pr_info("cpu%d. not h/w event\n", cpu);
    else
        pr_err("cpu%d. unable to create perf event: %ld\n",
                cpu, PTR_ERR(event));
    return NULL;
}

// This is needed since 4.1?
perf_event_enable(event);
event = perf_event_create_kernel_counter(&sched_perf_hw_attr, cpu, NULL, NULL);
perf_event_enable(event);
*/