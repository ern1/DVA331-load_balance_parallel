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

struct perf_event_attr init_perf_event()
{
    /*  Finns andra event som kanske är mer relevanta om man sätter type till PERF_TYPE_HW_CACHE,
        men PERF_COUNT_HW_CACHE_MISSES används i MemGuard så troligtvis bäst att använda samma. 
        Mer info: http://web.eece.maine.edu/~vweaver/projects/perf_events/perf_event_open.html */
    struct perf_event_attr attr;
    memset(&attr, 0, sizeof(struct perf_event_attr));
	attr.type             = PERF_TYPE_HARDWARE;
    attr.config           = PERF_COUNT_HW_CACHE_MISSES;
	attr.size             = sizeof(struct perf_event_attr);
    attr.pinned           = 1,  // Counter should always be on the CPU if at all possible (= 1) ---> Vettefan om denna ska vara satt till 1..
	attr.disabled         = 0;  // Start counter as enabled (= 0)
	attr.exclude_kernel   = 1;  // Don't count excludes events that happen in kernel-space (= 1) ---> Rätt? (fast varför skulle inte dom räknas?)

    return attr;

    /* --------- Saker jag stal från MemGuard nedan --------- */
    //struct perf_event *event = NULL;
    // struct perf_event_attr attr = {
    //     .type           = PERF_TYPE_HARDWARE,
    //     .config         = PERF_COUNT_HW_CACHE_MISSES,
    //     .size           = sizeof(struct perf_event_attr),
    //     .pinned         = 1,
    //     .disabled       = 1,
    //     .exclude_kernel = 1,   /* TODO: 1 mean, no kernel mode counting */
    // };

    // ska denna verkligen användas? (verkar inte så, så tog bort cpu som argument)
    //event = perf_event_create_kernel_counter(&attr, cpu, NULL, NULL, NULL);

    // if(!event)
    //     return NULL;

    // perf_event_enable(event);
    // pr_info("cpu%d enabled counter type %d.\n", cpu, (int)id);

    //return event;
}

static int perf_event_open(struct perf_event_attr *event_attr, pid_t pid, int cpu, int grp_fd, unsigned long flags)
{
    return syscall(__NR_perf_event_open, event_attr, pid, cpu, grp_fd, flags);
}

// Returns a file descriptor that allows measuring performance information
int start_counter(perf_event_attr* attr, int cpu)
{
    // pid == -1, cpu >= 0 - Measures all processes/threads on the specified CPU.
    int fd = perf_event_open(attr, -1, cpu, -1, 0);

    if(fd < 0)
        perror("Error: Opening performance counter");

    return fd;
}

unsigned long long read_counter(int fd)
{
    long long unsigned rv;

    if(read(fd, &rv, sizeof(rv)) == 0)
        perror("Error: Reading performance counter");

    return rv;
}

void stop_counter(int fd)
{
    if(close(fd) != 0)
        perror("Error: Stopping performance counter");
}


/* Körs på varje cpu i MemGuard (med hjälp av on_each_cpu som finns i smp.h,
    men den funktionen behöver nog inte vi).*/
// void start_counter(perf_event* event)
// {
//     event->pmu->add(event, PERF_EF_START);
// }
// void stop_counter(perf_event* event)
// {

// }

#endif