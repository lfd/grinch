/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2023-2024
 *
 * Authors:
 *  Lim Ern You <ern.lim@st.oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#define dbg_fmt(x) "ttp: " x

#include <grinch/ttp.h>
#include <grinch/errno.h>
#include <grinch/alloc.h>
#include <grinch/timer.h>
#include <grinch/gcall.h>
#include <grinch/bootparam.h>

struct ttp_event {
	unsigned int id;
	timeu_t abstime;
};

DEFINE_SPINLOCK(ttp_control_lock);
static unsigned long long ttp_maxevents;
static bool ttp_trace_active;

static void __init ttp_maxevents_parse(const char *arg)
{
	ttp_maxevents = strtoul(arg, NULL, 10);
}
bootparam(ttp_maxevents, ttp_maxevents_parse);

static int ttp_start(void)
{
	if (ttp_trace_active)
	   return -EBUSY;

	ttp_trace_active = true;
	return 0;
}

static int ttp_stop(void)
{
	if (!ttp_trace_active)
	   return -EBUSY;

	ttp_trace_active = false;
	return 0;
}

static int ttp_reset(void)
{
	struct ttp_storage *stor;
	unsigned int cpu;

	if (ttp_trace_active)
		return -EBUSY;

	for_each_available_cpu(cpu) {
		stor = &(per_cpu(cpu)->ttp_stor);

		memset(stor->events, 0, ttp_maxevents * sizeof(*stor->events));
		stor->print_max = false;
		stor->eventcount = 0;
	}

	return 0;
}

static int ttp_dump(void)
{
	struct ttp_storage *ttp_stor;
	struct ttp_event *ev;
	unsigned long long i;
	unsigned int cpu;

	if (ttp_trace_active)
		return -EBUSY;

	pr("CPU; ID; Timestamp\n");
	for_each_available_cpu(cpu) {
		ttp_stor = &(per_cpu(cpu)->ttp_stor);
		for (i = 0; i < ttp_stor->eventcount; i++) {
			ev = &ttp_stor->events[i];
			pr("%u; %d; %llu\n", cpu, ev->id, ev->abstime);
		}
	}

	return 0;
}

int ttp_emit(unsigned int id)
{
	struct ttp_storage *ttp_stor;
	struct ttp_event *event;
	timeu_t abs;

	/* get the timestamp as soon as possible */
	abs = timer_get_wall_ns();

	if (!ttp_trace_active)
		return -EAGAIN;

	ttp_stor = &this_per_cpu()->ttp_stor;
	if (ttp_stor->print_max)
		return -ENOMEM;

	if (ttp_stor->eventcount == ttp_maxevents) {
		pr_warn("CPU %lu: Reached event limit\n", this_cpu_id());
		ttp_stor->print_max = true;
		return -ENOMEM;
	}

	event = &ttp_stor->events[ttp_stor->eventcount];
	event->id = id;
	event->abstime = abs;
	ttp_stor->eventcount++;

	return 0;
}

int gcall_ttp(unsigned int no)
{
        long ret;

	spin_lock(&ttp_control_lock);
        switch (no) {
                case GCALL_TTP_START:
                        ret = ttp_start();
                        break;

                case GCALL_TTP_STOP:
                        ret = ttp_stop();
                        break;

                case GCALL_TTP_DUMP:
                        ret = ttp_dump();
                        break;

                case GCALL_TTP_RESET:
                        ret = ttp_reset();
                        break;

                default:
                        ret = -ENOSYS;
                        break;
        }
	spin_unlock(&ttp_control_lock);

        return ret;
}

int __init ttp_init(void)
{
	struct ttp_storage *stor;
	unsigned long cpu;

	if (ttp_maxevents == 0)
		return 0;

	pr("allocating memory for %llu trace events per cpu\n", ttp_maxevents);
	for_each_online_cpu(cpu) {
		stor = &per_cpu(cpu)->ttp_stor;

		stor->print_max = false;
		stor->eventcount = 0;

		stor->events = kmalloc(sizeof(*stor->events) * ttp_maxevents);
		if (!stor->events)
                        return -ENOMEM;
	}

	return 0;
}
