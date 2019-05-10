#ifndef __PERFCOUNTERS_HPP__
#define __PERFCOUNTERS_HPP__

#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/perf_event.h>

struct perfCounter {
    int wtf = 1234;
};

void init_perf_counter(perfCounter perf_cnt){
    return;
}

#if USE_RCFS
static struct perf_event *init_counting_counter(int cpu, int id)
{
    struct perf_event *event = NULL;
    struct perf_event_attr sched_perf_hw_attr = {
        /* use generalized hardware abstraction */
        .type           = PERF_TYPE_HARDWARE,
        .config         = id,
        .size        = sizeof(struct perf_event_attr),
        .pinned        = 1,
        .disabled    = 1,
        .exclude_kernel = 1,   /* TODO: 1 mean, no kernel mode counting */
    };

    /* Try to register using hardware perf events */
    event = perf_event_create_kernel_counter(
        &sched_perf_hw_attr,
        cpu, NULL,
        NULL
#if LINUX_VERSION_CODE > KERNEL_VERSION(3, 2, 0)
        ,NULL
#endif
        );

    if (!event)
        return NULL;

    if (IS_ERR(event)) {
        /* vary the KERN level based on the returned errno */
        if (PTR_ERR(event) == -EOPNOTSUPP)
            pr_info("cpu%d. not supported\n", cpu);
        else if (PTR_ERR(event) == -ENOENT)
            pr_info("cpu%d. not h/w event\n", cpu);
        else
            pr_err("cpu%d. unable to create perf event: %ld\n",
                   cpu, PTR_ERR(event));
        return NULL;
    }

    /* This is needed since 4.1? */
    perf_event_enable(event);
    
    /* success path */
    pr_info("cpu%d enabled counter type %d.\n", cpu, (int)id);

    return event;
}
#endif /* RCFS */

static struct perf_event *init_counter(int cpu, int budget)
{
    struct perf_event *event = NULL;
    struct perf_event_attr sched_perf_hw_attr = {
        /* use generalized hardware abstraction */
        .type           = PERF_TYPE_HARDWARE,
        .config         = PERF_COUNT_HW_CACHE_MISSES,
        .size        = sizeof(struct perf_event_attr),
        .pinned        = 1,
        .disabled    = 1,
        .exclude_kernel = 1,   /* TODO: 1 mean, no kernel mode counting */
    };

    if (!strcmp(g_hw_type, "core2")) {
        sched_perf_hw_attr.type           = PERF_TYPE_RAW;
        sched_perf_hw_attr.config         = 0x7024; /* 7024 - incl. prefetch
                                   5024 - only prefetch
                                   4024 - excl. prefetch */
    } else if (!strcmp(g_hw_type, "snb")) {
        sched_perf_hw_attr.type           = PERF_TYPE_RAW;
        sched_perf_hw_attr.config         = 0x08b0; /* 08b0 - incl. prefetch */
    } else if (!strcmp(g_hw_type, "armv7")) {
        sched_perf_hw_attr.type           = PERF_TYPE_RAW;
        sched_perf_hw_attr.config         = 0x17; /* Level 2 data cache refill */
    } else if (!strcmp(g_hw_type, "soft")) {
        sched_perf_hw_attr.type           = PERF_TYPE_SOFTWARE;
        sched_perf_hw_attr.config         = PERF_COUNT_SW_CPU_CLOCK;
    }

    /* select based on requested event type */
    sched_perf_hw_attr.sample_period = budget;

    /* Try to register using hardware perf events */
    event = perf_event_create_kernel_counter(
        &sched_perf_hw_attr,
        cpu, NULL,
        event_overflow_callback
#if LINUX_VERSION_CODE > KERNEL_VERSION(3, 2, 0)
        ,NULL
#endif
        );

    if (!event)
        return NULL;

    if (IS_ERR(event)) {
        /* vary the KERN level based on the returned errno */
        if (PTR_ERR(event) == -EOPNOTSUPP)
            pr_info("cpu%d. not supported\n", cpu);
        else if (PTR_ERR(event) == -ENOENT)
            pr_info("cpu%d. not h/w event\n", cpu);
        else
            pr_err("cpu%d. unable to create perf event: %ld\n",
                   cpu, PTR_ERR(event));
        return NULL;
    }

    /* This is needed since 4.1? */
    perf_event_enable(event);

    /* success path */
    pr_info("cpu%d enabled counter.\n", cpu);

    return event;
}

static void __disable_counter(void *info)
{
    struct core_info *cinfo = this_cpu_ptr(core_info);
    BUG_ON(!cinfo->event);

    /* stop the counter */
    cinfo->event->pmu->stop(cinfo->event, PERF_EF_UPDATE);
    cinfo->event->pmu->del(cinfo->event, 0);

    pr_info("LLC bandwidth throttling disabled\n");
}

static void disable_counters(void)
{
    on_each_cpu(__disable_counter, NULL, 0);
}


static void __start_counter(void* info)
{
    struct core_info *cinfo = this_cpu_ptr(core_info);
    cinfo->event->pmu->add(cinfo->event, PERF_EF_START);
#if USE_RCFS
    cinfo->cycle_event->pmu->add(cinfo->cycle_event, PERF_EF_START);
    cinfo->instr_event->pmu->add(cinfo->instr_event, PERF_EF_START);
#endif
}

static void start_counters(void)
{
    on_each_cpu(__start_counter, NULL, 0);
}


#endif