#undef TRACE_SYSTEM
#define TRACE_SYSTEM sched

#if !defined(_TRACE_SCHED_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_SCHED_H

#include <linux/sched.h>
#include <linux/tracepoint.h>
#include <linux/binfmts.h>

<<<<<<< HEAD
#ifdef CONFIG_MTK_SCHED_TRACERS
/* M: states for tracking I/O & mutex events
 * notice avoid to conflict with linux/sched.h
 *
 * A bug linux not fixed:
 * 'K' for TASK_WAKEKILL specified in linux/sched.h
 * but marked 'K' in sched_switch will cause Android systrace parser confused
 * therefore for sched_switch events, these extra states will be printed
 * in the end of each line
 */
#define _MT_TASK_BLOCKED_RTMUX		(TASK_STATE_MAX << 1)
#define _MT_TASK_BLOCKED_MUTEX		(TASK_STATE_MAX << 2)
#define _MT_TASK_BLOCKED_IO		(TASK_STATE_MAX << 3)
#define _MT_EXTRA_STATE_MASK (_MT_TASK_BLOCKED_RTMUX | _MT_TASK_BLOCKED_MUTEX | \
			      _MT_TASK_BLOCKED_IO | TASK_WAKEKILL)
#endif
#define _MT_TASK_STATE_MASK		((TASK_STATE_MAX - 1) & ~(TASK_WAKEKILL | TASK_PARKED))

=======
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
/*
 * Tracepoint for calling kthread_stop, performed to end a kthread:
 */
TRACE_EVENT(sched_kthread_stop,

	TP_PROTO(struct task_struct *t),

	TP_ARGS(t),

	TP_STRUCT__entry(
		__array(	char,	comm,	TASK_COMM_LEN	)
		__field(	pid_t,	pid			)
	),

	TP_fast_assign(
		memcpy(__entry->comm, t->comm, TASK_COMM_LEN);
		__entry->pid	= t->pid;
	),

	TP_printk("comm=%s pid=%d", __entry->comm, __entry->pid)
);

/*
 * Tracepoint for the return value of the kthread stopping:
 */
TRACE_EVENT(sched_kthread_stop_ret,

	TP_PROTO(int ret),

	TP_ARGS(ret),

	TP_STRUCT__entry(
		__field(	int,	ret	)
	),

	TP_fast_assign(
		__entry->ret	= ret;
	),

	TP_printk("ret=%d", __entry->ret)
);

<<<<<<< HEAD
#ifdef CREATE_TRACE_POINTS
static inline long __trace_sched_switch_state(struct task_struct *p);
#endif

=======
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
/*
 * Tracepoint for waking up a task:
 */
DECLARE_EVENT_CLASS(sched_wakeup_template,

	TP_PROTO(struct task_struct *p, int success),

	TP_ARGS(__perf_task(p), success),

	TP_STRUCT__entry(
		__array(	char,	comm,	TASK_COMM_LEN	)
		__field(	pid_t,	pid			)
		__field(	int,	prio			)
		__field(	int,	success			)
		__field(	int,	target_cpu		)
<<<<<<< HEAD
#ifdef CONFIG_MTK_SCHED_TRACERS
		__field(long, state)
#endif
=======
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	),

	TP_fast_assign(
		memcpy(__entry->comm, p->comm, TASK_COMM_LEN);
		__entry->pid		= p->pid;
		__entry->prio		= p->prio;
		__entry->success	= success;
		__entry->target_cpu	= task_cpu(p);
<<<<<<< HEAD
#ifdef CONFIG_MTK_SCHED_TRACERS
		__entry->state	= __trace_sched_switch_state(p);
#endif
	),

	TP_printk(
#ifdef CONFIG_MTK_SCHED_TRACERS
		"comm=%s pid=%d prio=%d success=%d target_cpu=%03d state=%s",
#else
		"comm=%s pid=%d prio=%d success=%d target_cpu=%03d",
#endif
		  __entry->comm, __entry->pid, __entry->prio,
		  __entry->success, __entry->target_cpu
#ifdef CONFIG_MTK_SCHED_TRACERS
		,
		__entry->state & ~TASK_STATE_MAX ?
		  __print_flags(__entry->state & ~TASK_STATE_MAX, "|",
				{TASK_INTERRUPTIBLE, "S"},
				{TASK_UNINTERRUPTIBLE, "D"},
				{__TASK_STOPPED, "T"},
				{__TASK_TRACED, "t"},
				{EXIT_ZOMBIE, "Z"},
				{EXIT_DEAD, "X"},
				{TASK_DEAD, "x"},
				{TASK_WAKEKILL, "K"},
				{TASK_WAKING, "W"},
				{_MT_TASK_BLOCKED_RTMUX, "r"},
				{_MT_TASK_BLOCKED_MUTEX, "m"},
				{_MT_TASK_BLOCKED_IO, "d"}) : "R"
#endif
			)
=======
	),

	TP_printk("comm=%s pid=%d prio=%d success=%d target_cpu=%03d",
		  __entry->comm, __entry->pid, __entry->prio,
		  __entry->success, __entry->target_cpu)
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
);

DEFINE_EVENT(sched_wakeup_template, sched_wakeup,
	     TP_PROTO(struct task_struct *p, int success),
	     TP_ARGS(p, success));

/*
 * Tracepoint for waking up a new task:
 */
DEFINE_EVENT(sched_wakeup_template, sched_wakeup_new,
	     TP_PROTO(struct task_struct *p, int success),
	     TP_ARGS(p, success));

#ifdef CREATE_TRACE_POINTS
static inline long __trace_sched_switch_state(struct task_struct *p)
{
	long state = p->state;

#ifdef CONFIG_PREEMPT
	/*
	 * For all intents and purposes a preempted task is a running task.
	 */
	if (preempt_count() & PREEMPT_ACTIVE)
		state = TASK_RUNNING | TASK_STATE_MAX;
#endif
<<<<<<< HEAD
#ifdef CONFIG_MTK_SCHED_TRACERS
#ifdef CONFIG_RT_MUTEXES
	if (p->pi_blocked_on)
		state |= _MT_TASK_BLOCKED_RTMUX;
#endif
#ifdef CONFIG_DEBUG_MUTEXES
	if (p->blocked_on)
		state |= _MT_TASK_BLOCKED_MUTEX;
#endif
	if ((p->state & TASK_UNINTERRUPTIBLE) && p->in_iowait)
		state |= _MT_TASK_BLOCKED_IO;
#endif

	return state;
}
# if defined(CONFIG_FAIR_GROUP_SCHED) && defined(CONFIG_MTK_SCHED_TRACERS)
/*
 * legacy cgroup hierarchy depth is no more than 3, and here we limit the
 * size of each load printing no more than 10, 9 chars with a slash '/'.
 * thus, making MTK_FAIR_DBG_SZ = 100 is pretty safe from array overflow,
 * because 100 is much larger than 60, ((3 * 10) * 2), 2 for @prev and @next
 * tasks.
 */
#  define MTK_FAIR_DBG_SZ	100
/*
 * snprintf writes at most @size bytes (including the trailing null bytes
 * ('\0'), so increment 10 to 11
 */
#  define MTK_FAIR_DBG_LEN	(10 + 1)
#  define MTK_FAIR_DBG_DEP	3

static int fair_cgroup_load(char *buf, int cnt, struct task_struct *p)
{
	int loc = cnt;
	int t, depth = 0;
	unsigned long w[MTK_FAIR_DBG_DEP];
	struct sched_entity *se = p->se.parent;

	for (; se && (depth < MTK_FAIR_DBG_DEP); se = se->parent)
		w[depth++] = se->load.weight;

	switch (p->policy) {
	case SCHED_NORMAL:
		loc += snprintf(&buf[loc], 7, "NORMAL"); break;
	case SCHED_IDLE:
		loc += snprintf(&buf[loc], 5, "IDLE"); break;
	case SCHED_BATCH:
		loc += snprintf(&buf[loc], 6, "BATCH"); break;
	}

	for (depth--; depth >= 0; depth--) {
		t = snprintf(&buf[loc], MTK_FAIR_DBG_LEN, "/%lu", w[depth]);
		if ((t < MTK_FAIR_DBG_LEN) && (t > 0))
			loc += t;
		else
			loc += snprintf(&buf[loc], 7, "/ERROR");
	}

	t = snprintf(&buf[loc], MTK_FAIR_DBG_LEN, "/%lu", p->se.load.weight);
	if ((t < MTK_FAIR_DBG_LEN) && (t > 0))
		loc += t;
	else
		loc += snprintf(&buf[loc], 7, "/ERROR");

	return loc;
}

static int is_fair_preempt(char *buf, struct task_struct *prev,
			   struct task_struct *next)
{
	int cnt;
	/* nothing needs to be clarified for RT class or yielding from IDLE */
	if ((task_pid_nr(prev) == 0) || (rt_task(next) || rt_task(prev)))
		return 0;

	/* take care about preemption only */
	if (prev->state &&
	    !(task_thread_info(prev)->preempt_count & PREEMPT_ACTIVE)) {
		return 0;
	}

	memset(buf, 0, MTK_FAIR_DBG_SZ);
	cnt = fair_cgroup_load(buf, 0, prev);
	cnt += snprintf(&buf[cnt], 6, " ==> ");
	fair_cgroup_load(buf, cnt, next);
	return 1;
}
# endif
=======

	return state;
}
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
#endif

/*
 * Tracepoint for task switches, performed by the scheduler:
 */
TRACE_EVENT(sched_switch,

	TP_PROTO(struct task_struct *prev,
		 struct task_struct *next),

	TP_ARGS(prev, next),

	TP_STRUCT__entry(
		__array(	char,	prev_comm,	TASK_COMM_LEN	)
		__field(	pid_t,	prev_pid			)
		__field(	int,	prev_prio			)
		__field(	long,	prev_state			)
		__array(	char,	next_comm,	TASK_COMM_LEN	)
		__field(	pid_t,	next_pid			)
		__field(	int,	next_prio			)
<<<<<<< HEAD
#if defined(CONFIG_FAIR_GROUP_SCHED) && defined(CONFIG_MTK_SCHED_TRACERS)
			__field(int, fair_preempt)
			__array(char, fair_dbg_buf,	MTK_FAIR_DBG_SZ)
#endif
=======
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	),

	TP_fast_assign(
		memcpy(__entry->next_comm, next->comm, TASK_COMM_LEN);
		__entry->prev_pid	= prev->pid;
		__entry->prev_prio	= prev->prio;
		__entry->prev_state	= __trace_sched_switch_state(prev);
		memcpy(__entry->prev_comm, prev->comm, TASK_COMM_LEN);
		__entry->next_pid	= next->pid;
		__entry->next_prio	= next->prio;
<<<<<<< HEAD
#if defined(CONFIG_FAIR_GROUP_SCHED) && defined(CONFIG_MTK_SCHED_TRACERS)
			__entry->fair_preempt	= is_fair_preempt(__entry->fair_dbg_buf,
								  prev, next);
#endif
	),

	TP_printk(
#ifdef CONFIG_MTK_SCHED_TRACERS
				"prev_comm=%s prev_pid=%d prev_prio=%d prev_state=%s%s ==> next_comm=%s next_pid=%d next_prio=%d%s%s %s",
#else
		"prev_comm=%s prev_pid=%d prev_prio=%d prev_state=%s%s ==> next_comm=%s next_pid=%d next_prio=%d",
#endif
		__entry->prev_comm, __entry->prev_pid, __entry->prev_prio,
		__entry->prev_state & (_MT_TASK_STATE_MASK) ?
		__print_flags(__entry->prev_state & (_MT_TASK_STATE_MASK), "|",
				{ 1, "S"} , { 2, "D" }, { 4, "T" }, { 8, "t" },
				{ 16, "Z" }, { 32, "X" }, { 64, "x" },
				{128, "K"}, { 256, "W"}) : "R",
		__entry->prev_state & TASK_STATE_MAX ? "+" : "",
		__entry->next_comm, __entry->next_pid, __entry->next_prio
#ifdef CONFIG_MTK_SCHED_TRACERS
		,
		(__entry->prev_state & _MT_EXTRA_STATE_MASK) ?
			" extra_prev_state=" : "",
		__print_flags(__entry->prev_state & _MT_EXTRA_STATE_MASK, "|",
			      { TASK_WAKEKILL, "K" },
			      { TASK_PARKED, "P" },
			      { _MT_TASK_BLOCKED_RTMUX, "r" },
			      { _MT_TASK_BLOCKED_MUTEX, "m" },
			      { _MT_TASK_BLOCKED_IO, "d" })
# ifdef CONFIG_FAIR_GROUP_SCHED
			, (__entry->fair_preempt ? __entry->fair_dbg_buf : "")
# else
			, ""
# endif
#endif
		)
=======
	),

	TP_printk("prev_comm=%s prev_pid=%d prev_prio=%d prev_state=%s%s ==> next_comm=%s next_pid=%d next_prio=%d",
		__entry->prev_comm, __entry->prev_pid, __entry->prev_prio,
		__entry->prev_state & (TASK_STATE_MAX-1) ?
		  __print_flags(__entry->prev_state & (TASK_STATE_MAX-1), "|",
				{ 1, "S"} , { 2, "D" }, { 4, "T" }, { 8, "t" },
				{ 16, "Z" }, { 32, "X" }, { 64, "x" },
				{ 128, "K" }, { 256, "W" }, { 512, "P" }) : "R",
		__entry->prev_state & TASK_STATE_MAX ? "+" : "",
		__entry->next_comm, __entry->next_pid, __entry->next_prio)
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
);

/*
 * Tracepoint for a task being migrated:
 */
TRACE_EVENT(sched_migrate_task,

	TP_PROTO(struct task_struct *p, int dest_cpu),

	TP_ARGS(p, dest_cpu),

	TP_STRUCT__entry(
		__array(	char,	comm,	TASK_COMM_LEN	)
		__field(	pid_t,	pid			)
		__field(	int,	prio			)
		__field(	int,	orig_cpu		)
		__field(	int,	dest_cpu		)
<<<<<<< HEAD
#ifdef CONFIG_MTK_SCHED_TRACERS
		__field(long, state)
#endif
=======
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	),

	TP_fast_assign(
		memcpy(__entry->comm, p->comm, TASK_COMM_LEN);
		__entry->pid		= p->pid;
		__entry->prio		= p->prio;
		__entry->orig_cpu	= task_cpu(p);
		__entry->dest_cpu	= dest_cpu;
<<<<<<< HEAD
#ifdef CONFIG_MTK_SCHED_TRACERS
		__entry->state      =	__trace_sched_switch_state(p);
#endif
	),

#ifdef CONFIG_MTK_SCHED_TRACERS
	TP_printk("comm=%s pid=%d prio=%d orig_cpu=%d dest_cpu=%d state=%s",
#else
	TP_printk("comm=%s pid=%d prio=%d orig_cpu=%d dest_cpu=%d",
#endif
		  __entry->comm, __entry->pid, __entry->prio,
		  __entry->orig_cpu, __entry->dest_cpu
#ifdef CONFIG_MTK_SCHED_TRACERS
		,
		__entry->state & ~TASK_STATE_MAX ?
		  __print_flags(__entry->state & ~TASK_STATE_MAX, "|",
				{ TASK_INTERRUPTIBLE, "S"},
				{ TASK_UNINTERRUPTIBLE, "D" },
				{ __TASK_STOPPED, "T" },
				{ __TASK_TRACED, "t" },
				{ EXIT_ZOMBIE, "Z" },
				{ EXIT_DEAD, "X" },
				{ TASK_DEAD, "x" },
				{ TASK_WAKEKILL, "K" },
				{ TASK_WAKING, "W"},
				{ _MT_TASK_BLOCKED_RTMUX, "r"},
				{ _MT_TASK_BLOCKED_MUTEX, "m"},
				{ _MT_TASK_BLOCKED_IO, "d"}) : "R"
#endif
			)
=======
	),

	TP_printk("comm=%s pid=%d prio=%d orig_cpu=%d dest_cpu=%d",
		  __entry->comm, __entry->pid, __entry->prio,
		  __entry->orig_cpu, __entry->dest_cpu)
);

/*
 * Tracepoint for a CPU going offline/online:
 */
TRACE_EVENT(sched_cpu_hotplug,

	TP_PROTO(int affected_cpu, int error, int status),

	TP_ARGS(affected_cpu, error, status),

	TP_STRUCT__entry(
		__field(	int,	affected_cpu		)
		__field(	int,	error			)
		__field(	int,	status			)
	),

	TP_fast_assign(
		__entry->affected_cpu	= affected_cpu;
		__entry->error		= error;
		__entry->status		= status;
	),

	TP_printk("cpu %d %s error=%d", __entry->affected_cpu,
		__entry->status ? "online" : "offline", __entry->error)
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
);

DECLARE_EVENT_CLASS(sched_process_template,

	TP_PROTO(struct task_struct *p),

	TP_ARGS(p),

	TP_STRUCT__entry(
		__array(	char,	comm,	TASK_COMM_LEN	)
		__field(	pid_t,	pid			)
		__field(	int,	prio			)
	),

	TP_fast_assign(
		memcpy(__entry->comm, p->comm, TASK_COMM_LEN);
		__entry->pid		= p->pid;
		__entry->prio		= p->prio;
	),

	TP_printk("comm=%s pid=%d prio=%d",
		  __entry->comm, __entry->pid, __entry->prio)
);

/*
 * Tracepoint for freeing a task:
 */
DEFINE_EVENT(sched_process_template, sched_process_free,
	     TP_PROTO(struct task_struct *p),
	     TP_ARGS(p));
<<<<<<< HEAD
	     
=======

>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916

/*
 * Tracepoint for a task exiting:
 */
DEFINE_EVENT(sched_process_template, sched_process_exit,
	     TP_PROTO(struct task_struct *p),
	     TP_ARGS(p));

/*
 * Tracepoint for waiting on task to unschedule:
 */
DEFINE_EVENT(sched_process_template, sched_wait_task,
	TP_PROTO(struct task_struct *p),
	TP_ARGS(p));

/*
 * Tracepoint for a waiting task:
 */
TRACE_EVENT(sched_process_wait,

	TP_PROTO(struct pid *pid),

	TP_ARGS(pid),

	TP_STRUCT__entry(
		__array(	char,	comm,	TASK_COMM_LEN	)
		__field(	pid_t,	pid			)
		__field(	int,	prio			)
	),

	TP_fast_assign(
		memcpy(__entry->comm, current->comm, TASK_COMM_LEN);
		__entry->pid		= pid_nr(pid);
		__entry->prio		= current->prio;
	),

	TP_printk("comm=%s pid=%d prio=%d",
		  __entry->comm, __entry->pid, __entry->prio)
);

/*
 * Tracepoint for do_fork:
 */
TRACE_EVENT(sched_process_fork,

	TP_PROTO(struct task_struct *parent, struct task_struct *child),

	TP_ARGS(parent, child),

	TP_STRUCT__entry(
		__array(	char,	parent_comm,	TASK_COMM_LEN	)
		__field(	pid_t,	parent_pid			)
		__array(	char,	child_comm,	TASK_COMM_LEN	)
		__field(	pid_t,	child_pid			)
	),

	TP_fast_assign(
		memcpy(__entry->parent_comm, parent->comm, TASK_COMM_LEN);
		__entry->parent_pid	= parent->pid;
		memcpy(__entry->child_comm, child->comm, TASK_COMM_LEN);
		__entry->child_pid	= child->pid;
	),

	TP_printk("comm=%s pid=%d child_comm=%s child_pid=%d",
		__entry->parent_comm, __entry->parent_pid,
		__entry->child_comm, __entry->child_pid)
);

/*
 * Tracepoint for exec:
 */
TRACE_EVENT(sched_process_exec,

	TP_PROTO(struct task_struct *p, pid_t old_pid,
		 struct linux_binprm *bprm),

	TP_ARGS(p, old_pid, bprm),

	TP_STRUCT__entry(
		__string(	filename,	bprm->filename	)
		__field(	pid_t,		pid		)
		__field(	pid_t,		old_pid		)
	),

	TP_fast_assign(
		__assign_str(filename, bprm->filename);
		__entry->pid		= p->pid;
		__entry->old_pid	= old_pid;
	),

	TP_printk("filename=%s pid=%d old_pid=%d", __get_str(filename),
		  __entry->pid, __entry->old_pid)
);

/*
 * XXX the below sched_stat tracepoints only apply to SCHED_OTHER/BATCH/IDLE
 *     adding sched_stat support to SCHED_FIFO/RR would be welcome.
 */
DECLARE_EVENT_CLASS(sched_stat_template,

	TP_PROTO(struct task_struct *tsk, u64 delay),

	TP_ARGS(__perf_task(tsk), __perf_count(delay)),

	TP_STRUCT__entry(
		__array( char,	comm,	TASK_COMM_LEN	)
		__field( pid_t,	pid			)
		__field( u64,	delay			)
	),

	TP_fast_assign(
		memcpy(__entry->comm, tsk->comm, TASK_COMM_LEN);
		__entry->pid	= tsk->pid;
		__entry->delay	= delay;
	),

	TP_printk("comm=%s pid=%d delay=%Lu [ns]",
			__entry->comm, __entry->pid,
			(unsigned long long)__entry->delay)
);


/*
 * Tracepoint for accounting wait time (time the task is runnable
 * but not actually running due to scheduler contention).
 */
DEFINE_EVENT(sched_stat_template, sched_stat_wait,
	     TP_PROTO(struct task_struct *tsk, u64 delay),
	     TP_ARGS(tsk, delay));

/*
 * Tracepoint for accounting sleep time (time the task is not runnable,
 * including iowait, see below).
 */
DEFINE_EVENT(sched_stat_template, sched_stat_sleep,
	     TP_PROTO(struct task_struct *tsk, u64 delay),
	     TP_ARGS(tsk, delay));

/*
 * Tracepoint for accounting iowait time (time the task is not runnable
 * due to waiting on IO to complete).
 */
DEFINE_EVENT(sched_stat_template, sched_stat_iowait,
	     TP_PROTO(struct task_struct *tsk, u64 delay),
	     TP_ARGS(tsk, delay));

/*
 * Tracepoint for accounting blocked time (time the task is in uninterruptible).
 */
DEFINE_EVENT(sched_stat_template, sched_stat_blocked,
	     TP_PROTO(struct task_struct *tsk, u64 delay),
	     TP_ARGS(tsk, delay));

/*
<<<<<<< HEAD
=======
 * Tracepoint for recording the cause of uninterruptible sleep.
 */
TRACE_EVENT(sched_blocked_reason,

	TP_PROTO(struct task_struct *tsk),

	TP_ARGS(tsk),

	TP_STRUCT__entry(
		__field( pid_t,	pid	)
		__field( void*, caller	)
		__field( bool, io_wait	)
	),

	TP_fast_assign(
		__entry->pid	= tsk->pid;
		__entry->caller = (void*)get_wchan(tsk);
		__entry->io_wait = tsk->in_iowait;
	),

	TP_printk("pid=%d iowait=%d caller=%pS", __entry->pid, __entry->io_wait, __entry->caller)
);

/*
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
 * Tracepoint for accounting runtime (time the task is executing
 * on a CPU).
 */
DECLARE_EVENT_CLASS(sched_stat_runtime,

	TP_PROTO(struct task_struct *tsk, u64 runtime, u64 vruntime),

	TP_ARGS(tsk, __perf_count(runtime), vruntime),

	TP_STRUCT__entry(
		__array( char,	comm,	TASK_COMM_LEN	)
		__field( pid_t,	pid			)
		__field( u64,	runtime			)
		__field( u64,	vruntime			)
	),

	TP_fast_assign(
		memcpy(__entry->comm, tsk->comm, TASK_COMM_LEN);
		__entry->pid		= tsk->pid;
		__entry->runtime	= runtime;
		__entry->vruntime	= vruntime;
	),

	TP_printk("comm=%s pid=%d runtime=%Lu [ns] vruntime=%Lu [ns]",
			__entry->comm, __entry->pid,
			(unsigned long long)__entry->runtime,
			(unsigned long long)__entry->vruntime)
);

DEFINE_EVENT(sched_stat_runtime, sched_stat_runtime,
	     TP_PROTO(struct task_struct *tsk, u64 runtime, u64 vruntime),
	     TP_ARGS(tsk, runtime, vruntime));

/*
 * Tracepoint for showing priority inheritance modifying a tasks
 * priority.
 */
TRACE_EVENT(sched_pi_setprio,

	TP_PROTO(struct task_struct *tsk, int newprio),

	TP_ARGS(tsk, newprio),

	TP_STRUCT__entry(
		__array( char,	comm,	TASK_COMM_LEN	)
		__field( pid_t,	pid			)
		__field( int,	oldprio			)
		__field( int,	newprio			)
	),

	TP_fast_assign(
		memcpy(__entry->comm, tsk->comm, TASK_COMM_LEN);
		__entry->pid		= tsk->pid;
		__entry->oldprio	= tsk->prio;
		__entry->newprio	= newprio;
	),

	TP_printk("comm=%s pid=%d oldprio=%d newprio=%d",
			__entry->comm, __entry->pid,
			__entry->oldprio, __entry->newprio)
);

#ifdef CONFIG_DETECT_HUNG_TASK
TRACE_EVENT(sched_process_hang,
	TP_PROTO(struct task_struct *tsk),
	TP_ARGS(tsk),

	TP_STRUCT__entry(
		__array( char,	comm,	TASK_COMM_LEN	)
		__field( pid_t,	pid			)
	),

	TP_fast_assign(
		memcpy(__entry->comm, tsk->comm, TASK_COMM_LEN);
		__entry->pid = tsk->pid;
	),

	TP_printk("comm=%s pid=%d", __entry->comm, __entry->pid)
);
#endif /* CONFIG_DETECT_HUNG_TASK */

DECLARE_EVENT_CLASS(sched_move_task_template,

	TP_PROTO(struct task_struct *tsk, int src_cpu, int dst_cpu),

	TP_ARGS(tsk, src_cpu, dst_cpu),

	TP_STRUCT__entry(
		__field( pid_t,	pid			)
		__field( pid_t,	tgid			)
		__field( pid_t,	ngid			)
		__field( int,	src_cpu			)
		__field( int,	src_nid			)
		__field( int,	dst_cpu			)
		__field( int,	dst_nid			)
	),

	TP_fast_assign(
		__entry->pid		= task_pid_nr(tsk);
		__entry->tgid		= task_tgid_nr(tsk);
		__entry->ngid		= task_numa_group_id(tsk);
		__entry->src_cpu	= src_cpu;
		__entry->src_nid	= cpu_to_node(src_cpu);
		__entry->dst_cpu	= dst_cpu;
		__entry->dst_nid	= cpu_to_node(dst_cpu);
	),

	TP_printk("pid=%d tgid=%d ngid=%d src_cpu=%d src_nid=%d dst_cpu=%d dst_nid=%d",
			__entry->pid, __entry->tgid, __entry->ngid,
			__entry->src_cpu, __entry->src_nid,
			__entry->dst_cpu, __entry->dst_nid)
);

/*
 * Tracks migration of tasks from one runqueue to another. Can be used to
 * detect if automatic NUMA balancing is bouncing between nodes
 */
DEFINE_EVENT(sched_move_task_template, sched_move_numa,
	TP_PROTO(struct task_struct *tsk, int src_cpu, int dst_cpu),

	TP_ARGS(tsk, src_cpu, dst_cpu)
);

DEFINE_EVENT(sched_move_task_template, sched_stick_numa,
	TP_PROTO(struct task_struct *tsk, int src_cpu, int dst_cpu),

	TP_ARGS(tsk, src_cpu, dst_cpu)
);

TRACE_EVENT(sched_swap_numa,

	TP_PROTO(struct task_struct *src_tsk, int src_cpu,
		 struct task_struct *dst_tsk, int dst_cpu),

	TP_ARGS(src_tsk, src_cpu, dst_tsk, dst_cpu),

	TP_STRUCT__entry(
		__field( pid_t,	src_pid			)
		__field( pid_t,	src_tgid		)
		__field( pid_t,	src_ngid		)
		__field( int,	src_cpu			)
		__field( int,	src_nid			)
		__field( pid_t,	dst_pid			)
		__field( pid_t,	dst_tgid		)
		__field( pid_t,	dst_ngid		)
		__field( int,	dst_cpu			)
		__field( int,	dst_nid			)
	),

	TP_fast_assign(
		__entry->src_pid	= task_pid_nr(src_tsk);
		__entry->src_tgid	= task_tgid_nr(src_tsk);
		__entry->src_ngid	= task_numa_group_id(src_tsk);
		__entry->src_cpu	= src_cpu;
		__entry->src_nid	= cpu_to_node(src_cpu);
		__entry->dst_pid	= task_pid_nr(dst_tsk);
		__entry->dst_tgid	= task_tgid_nr(dst_tsk);
		__entry->dst_ngid	= task_numa_group_id(dst_tsk);
		__entry->dst_cpu	= dst_cpu;
		__entry->dst_nid	= cpu_to_node(dst_cpu);
	),

	TP_printk("src_pid=%d src_tgid=%d src_ngid=%d src_cpu=%d src_nid=%d dst_pid=%d dst_tgid=%d dst_ngid=%d dst_cpu=%d dst_nid=%d",
			__entry->src_pid, __entry->src_tgid, __entry->src_ngid,
			__entry->src_cpu, __entry->src_nid,
			__entry->dst_pid, __entry->dst_tgid, __entry->dst_ngid,
			__entry->dst_cpu, __entry->dst_nid)
);

/*
 * Tracepoint for waking a polling cpu without an IPI.
 */
TRACE_EVENT(sched_wake_idle_without_ipi,

	TP_PROTO(int cpu),

	TP_ARGS(cpu),

	TP_STRUCT__entry(
		__field(	int,	cpu	)
	),

	TP_fast_assign(
		__entry->cpu	= cpu;
	),

	TP_printk("cpu=%d", __entry->cpu)
);

<<<<<<< HEAD

#ifdef CONFIG_MTK_SCHED_TRACERS
/*
 * Tracepoint for showing the result of task runqueue selection
 */
TRACE_EVENT(sched_select_task_rq,

	TP_PROTO(struct task_struct *tsk, int policy, int prev_cpu, int target_cpu),

	TP_ARGS(tsk, policy, prev_cpu, target_cpu),

	TP_STRUCT__entry(
		__field(pid_t, pid)
		__field(int, policy)
		__field(int, prev_cpu)
		__field(int, target_cpu)
	),

	TP_fast_assign(
		__entry->pid              = tsk->pid;
		__entry->policy           = policy;
		__entry->prev_cpu         = prev_cpu;
		__entry->target_cpu       = target_cpu;
	),

	TP_printk("pid=%4d policy=0x%08x pre-cpu=%d target=%d",
			__entry->pid,
			__entry->policy,
			__entry->prev_cpu,
			__entry->target_cpu)
);
#endif

#ifdef CONFIG_MT_SCHED_TRACE
#define sched_trace(event) \
TRACE_EVENT(event,             		\
    TP_PROTO(char *strings),			\
    TP_ARGS(strings),				\
    TP_STRUCT__entry(				\
        __array(    char,  strings, 128)	\
    ),						\
    TP_fast_assign(				\
        memcpy(__entry->strings, strings, 128);	\
    ),						\
    TP_printk("%s",__entry->strings))		


sched_trace(sched_log);
// mtk rt enhancement 
sched_trace(sched_rt);
sched_trace(sched_rt_info);
sched_trace(sched_lb);
sched_trace(sched_lb_info);
 #ifdef CONFIG_MTK_SCHED_CMP
sched_trace(sched_cmp);
sched_trace(sched_cmp_info);
 #endif

// mtk scheduling interopertion enhancement 
 #ifdef CONFIG_MT_SCHED_INTEROP
sched_trace(sched_interop);
 #endif

 #ifdef CONFIG_MT_DEBUG_PREEMPT
sched_trace(sched_preempt);
 #endif
#endif

/*sched: add trace_sched*/
TRACE_EVENT(sched_task_entity_avg,

	TP_PROTO(unsigned int tag, struct task_struct *tsk, struct sched_avg *avg),

	TP_ARGS(tag, tsk, avg),

	TP_STRUCT__entry(
		__field(u32, tag)
		__array(char, comm,	TASK_COMM_LEN)
		__field(pid_t, tgid)
		__field(pid_t, pid)
		__field(unsigned long, contrib)
		__field(unsigned long, ratio)
		__field(u32, usage_sum)
		__field(unsigned long, rq_time)
		__field(unsigned long, live_time)
	),

	TP_fast_assign(
		__entry->tag       = tag;
		memcpy(__entry->comm, tsk->comm, TASK_COMM_LEN);
		__entry->tgid      = task_pid_nr(tsk->group_leader);
		__entry->pid       = task_pid_nr(tsk);
		__entry->contrib   = avg->load_avg_contrib;
		__entry->ratio     = 0;
		__entry->usage_sum = -1;
		__entry->rq_time   = avg->runnable_avg_sum;
		__entry->live_time = avg->avg_period;
	),

	TP_printk("[%d]comm=%s tgid=%d pid=%d contrib=%lu ratio=%lu exe_time=%d rq_time=%lu live_time=%lu",
		  __entry->tag, __entry->comm, __entry->tgid, __entry->pid,
		  __entry->contrib, __entry->ratio, __entry->usage_sum,
		  __entry->rq_time, __entry->live_time)
);

/*
 * Tracepoint for HMP (CONFIG_SCHED_HMP) task migrations.
 */
TRACE_EVENT(sched_hmp_migrate,

	TP_PROTO(struct task_struct *tsk, int dest, int force),

	TP_ARGS(tsk, dest, force),

	TP_STRUCT__entry(
		__array(char, comm, TASK_COMM_LEN)
		__field(pid_t, pid)
		__field(int,  dest)
		__field(int,  force)
	),

	TP_fast_assign(
	memcpy(__entry->comm, tsk->comm, TASK_COMM_LEN);
		__entry->pid   = tsk->pid;
		__entry->dest  = dest;
		__entry->force = force;
	),

	TP_printk("comm=%s pid=%d dest=%d force=%d",
			__entry->comm, __entry->pid,
			__entry->dest, __entry->force)
);

/*
 * sched: tracepoint for showing tracked load contribution.
 */
TRACE_EVENT(sched_task_load_contrib,

	TP_PROTO(struct task_struct *tsk, unsigned long load_contrib),

	TP_ARGS(tsk, load_contrib),

	TP_STRUCT__entry(
		__array(char, comm, TASK_COMM_LEN)
		__field(pid_t, pid)
		__field(unsigned long, load_contrib)
=======
TRACE_EVENT(sched_contrib_scale_f,

	TP_PROTO(int cpu, unsigned long freq_scale_factor,
		 unsigned long cpu_scale_factor),

	TP_ARGS(cpu, freq_scale_factor, cpu_scale_factor),

	TP_STRUCT__entry(
		__field(int, cpu)
		__field(unsigned long, freq_scale_factor)
		__field(unsigned long, cpu_scale_factor)
	),

	TP_fast_assign(
		__entry->cpu = cpu;
		__entry->freq_scale_factor = freq_scale_factor;
		__entry->cpu_scale_factor = cpu_scale_factor;
	),

	TP_printk("cpu=%d freq_scale_factor=%lu cpu_scale_factor=%lu",
		  __entry->cpu, __entry->freq_scale_factor,
		  __entry->cpu_scale_factor)
);

#ifdef CONFIG_SMP

/*
 * Tracepoint for accounting sched averages for tasks.
 */
TRACE_EVENT(sched_load_avg_task,

	TP_PROTO(struct task_struct *tsk, struct sched_avg *avg),

	TP_ARGS(tsk, avg),

	TP_STRUCT__entry(
		__array( char,	comm,	TASK_COMM_LEN		)
		__field( pid_t,	pid				)
		__field( int,	cpu				)
		__field( unsigned long,	load_avg		)
		__field( unsigned long,	util_avg		)
		__field( u64,		load_sum		)
		__field( u32,		util_sum		)
		__field( u32,		period_contrib		)
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	),

	TP_fast_assign(
		memcpy(__entry->comm, tsk->comm, TASK_COMM_LEN);
<<<<<<< HEAD
		__entry->pid            = tsk->pid;
		__entry->load_contrib   = load_contrib;
	),

	TP_printk("comm=%s pid=%d load_contrib=%lu",
			__entry->comm, __entry->pid,
			__entry->load_contrib)
);

/*
 * sched: tracepoint for showing tracked task runnable ratio [0..1023].
 */
TRACE_EVENT(sched_task_runnable_ratio,

	TP_PROTO(struct task_struct *tsk, unsigned long ratio),

	TP_ARGS(tsk, ratio),

	TP_STRUCT__entry(
		__array(char, comm, TASK_COMM_LEN)
		__field(pid_t, pid)
		__field(unsigned long, ratio)
	),

	TP_fast_assign(
	memcpy(__entry->comm, tsk->comm, TASK_COMM_LEN);
		__entry->pid   = tsk->pid;
		__entry->ratio = ratio;
	),

	TP_printk("comm=%s pid=%d ratio=%lu",
			__entry->comm, __entry->pid,
			__entry->ratio)
);

#ifdef CONFIG_HMP_TRACER
/*
 * Tracepoint for showing tracked migration information
 */
TRACE_EVENT(sched_dynamic_threshold,

	TP_PROTO(struct task_struct *tsk, unsigned int threshold,
			unsigned int status, int curr_cpu, int target_cpu, int task_load,
			struct clb_stats *B, struct clb_stats *L),

	TP_ARGS(tsk, threshold, status, curr_cpu, target_cpu, task_load, B, L),

	TP_STRUCT__entry(
		__array(char, comm, TASK_COMM_LEN)
		__field(pid_t, pid)
		__field(int, prio)
		__field(unsigned int, threshold)
		__field(unsigned int, status)
		__field(int, curr_cpu)
		__field(int, target_cpu)
		__field(int, curr_load)
		__field(int, target_load)
		__field(int, task_load)
		__field(int, B_load_avg)
		__field(int, L_load_avg)
	),

	TP_fast_assign(
		memcpy(__entry->comm, tsk->comm, TASK_COMM_LEN);
		__entry->pid              = tsk->pid;
		__entry->prio             = tsk->prio;
		__entry->threshold        = threshold;
		__entry->status           = status;
		__entry->curr_cpu         = curr_cpu;
		__entry->target_cpu       = target_cpu;
		__entry->curr_load        = cpu_rq(curr_cpu)->cfs.avg.load_avg_contrib;
		__entry->target_load      = cpu_rq(target_cpu)->cfs.avg.load_avg_contrib;
		__entry->task_load        = task_load;
		__entry->B_load_avg       = B->load_avg;
		__entry->L_load_avg       = L->load_avg;
	),

	TP_printk(
"pid=%4d prio=%d status=0x%4x dyn=%4u task-load=%4d curr-cpu=%d(%4d) target=%d(%4d) L-load-avg=%4d B-load-avg=%4d comm=%s",
				__entry->pid,
				__entry->prio,
				__entry->status,
				__entry->threshold,
				__entry->task_load,
				__entry->curr_cpu,
				__entry->curr_load,
				__entry->target_cpu,
				__entry->target_load,
				__entry->L_load_avg,
				__entry->B_load_avg,
				__entry->comm)
);

/*
 * Tracepoint for showing the result of hmp task runqueue selection
 */
TRACE_EVENT(sched_hmp_select_task_rq,

	TP_PROTO(struct task_struct *tsk, int step, int sd_flag, int prev_cpu,
			int target_cpu, int task_load, struct clb_stats *B,
			struct clb_stats *L),

	TP_ARGS(tsk, step, sd_flag, prev_cpu, target_cpu, task_load, B, L),

	TP_STRUCT__entry(
		__array(char, comm, TASK_COMM_LEN)
		__field(pid_t, pid)
		__field(int, prio)
		__field(int, step)
		__field(int, sd_flag)
		__field(int, prev_cpu)
		__field(int, target_cpu)
		__field(int, prev_load)
		__field(int, target_load)
		__field(int, task_load)
		__field(int, B_load_avg)
		__field(int, L_load_avg)
	),

	TP_fast_assign(
		memcpy(__entry->comm, tsk->comm, TASK_COMM_LEN);
		__entry->pid              = tsk->pid;
		__entry->prio             = tsk->prio;
		__entry->step             = step;
		__entry->sd_flag          = sd_flag;
		__entry->prev_cpu         = prev_cpu;
		__entry->target_cpu       = target_cpu;
		__entry->prev_load        = cpu_rq(prev_cpu)->cfs.avg.load_avg_contrib;
		__entry->target_load      = cpu_rq(target_cpu)->cfs.avg.load_avg_contrib;
		__entry->task_load        = task_load;
		__entry->B_load_avg       = B->load_avg;
		__entry->L_load_avg       = L->load_avg;
	),

	TP_printk(
"pid=%4d prio=%d task-load=%4d sd-flag=%2d step=%d pre-cpu=%d(%4d) target=%d(%4d) L-load-avg=%4d B-load-avg=%4d comm=%s",
			__entry->pid,
			__entry->prio,
			__entry->task_load,
			__entry->sd_flag,
			__entry->step,
			__entry->prev_cpu,
			__entry->prev_load,
			__entry->target_cpu,
			__entry->target_load,
			__entry->L_load_avg,
			__entry->B_load_avg,
			__entry->comm)
);

/*
 * Tracepoint for dumping hmp cluster load ratio
 */
TRACE_EVENT(sched_hmp_load,

	TP_PROTO(int B_load_avg, int L_load_avg),

	TP_ARGS(B_load_avg, L_load_avg),

	TP_STRUCT__entry(
		__field(int, B_load_avg)
		__field(int, L_load_avg)
	),

	TP_fast_assign(
		__entry->B_load_avg = B_load_avg;
		__entry->L_load_avg = L_load_avg;
	),

	TP_printk("B-load-avg=%4d L-load-avg=%4d",
			__entry->B_load_avg,
			__entry->L_load_avg)
);

/*
 * Tracepoint for dumping hmp statistics
 */
TRACE_EVENT(sched_hmp_stats,

	TP_PROTO(struct hmp_statisic *hmp_stats),

	TP_ARGS(hmp_stats),

	TP_STRUCT__entry(
		__field(unsigned int, nr_force_up)
		__field(unsigned int, nr_force_down)
	),

	TP_fast_assign(
		__entry->nr_force_up = hmp_stats->nr_force_up;
		__entry->nr_force_down = hmp_stats->nr_force_down;
	),

	TP_printk("nr-force-up=%d nr-force-down=%2d",
			__entry->nr_force_up,
			__entry->nr_force_down)
);

/*
 * Tracepoint for cfs task enqueue event
 */
TRACE_EVENT(sched_cfs_enqueue_task,

	TP_PROTO(struct task_struct *tsk, int tsk_load, int cpu_id),

	TP_ARGS(tsk, tsk_load, cpu_id),

	TP_STRUCT__entry(
		__array(char, comm, TASK_COMM_LEN)
		__field(pid_t, tsk_pid)
		__field(int, tsk_load)
		__field(int, cpu_id)
	),

	TP_fast_assign(
		memcpy(__entry->comm, tsk->comm, TASK_COMM_LEN);
		__entry->tsk_pid = tsk->pid;
		__entry->tsk_load = tsk_load;
		__entry->cpu_id = cpu_id;
	),

	TP_printk("cpu-id=%d task-pid=%4d task-load=%4d comm=%s",
			__entry->cpu_id,
			__entry->tsk_pid,
			__entry->tsk_load,
			__entry->comm)
);

/*
 * Tracepoint for cfs task dequeue event
 */
TRACE_EVENT(sched_cfs_dequeue_task,

	TP_PROTO(struct task_struct *tsk, int tsk_load, int cpu_id),

	TP_ARGS(tsk, tsk_load, cpu_id),

	TP_STRUCT__entry(
		__array(char, comm, TASK_COMM_LEN)
		__field(pid_t, tsk_pid)
		__field(int, tsk_load)
		__field(int, cpu_id)
=======
		__entry->pid			= tsk->pid;
		__entry->cpu			= task_cpu(tsk);
		__entry->load_avg		= avg->load_avg;
		__entry->util_avg		= avg->util_avg;
		__entry->load_sum		= avg->load_sum;
		__entry->util_sum		= avg->util_sum;
		__entry->period_contrib		= avg->period_contrib;
	),

	TP_printk("comm=%s pid=%d cpu=%d load_avg=%lu util_avg=%lu load_sum=%llu"
		  " util_sum=%u period_contrib=%u",
		  __entry->comm,
		  __entry->pid,
		  __entry->cpu,
		  __entry->load_avg,
		  __entry->util_avg,
		  (u64)__entry->load_sum,
		  (u32)__entry->util_sum,
		  (u32)__entry->period_contrib)
);

/*
 * Tracepoint for accounting sched averages for cpus.
 */
TRACE_EVENT(sched_load_avg_cpu,

	TP_PROTO(int cpu, struct cfs_rq *cfs_rq),

	TP_ARGS(cpu, cfs_rq),

	TP_STRUCT__entry(
		__field( int,	cpu				)
		__field( unsigned long,	load_avg		)
		__field( unsigned long,	util_avg		)
	),

	TP_fast_assign(
		__entry->cpu			= cpu;
		__entry->load_avg		= cfs_rq->avg.load_avg;
		__entry->util_avg		= cfs_rq->avg.util_avg;
	),

	TP_printk("cpu=%d load_avg=%lu util_avg=%lu",
		  __entry->cpu, __entry->load_avg, __entry->util_avg)
);

/*
 * Tracepoint for sched_tune_config settings
 */
TRACE_EVENT(sched_tune_config,

	TP_PROTO(int boost, int pb_nrg_gain, int pb_cap_gain, int pc_nrg_gain, int pc_cap_gain),

	TP_ARGS(boost, pb_nrg_gain, pb_cap_gain, pc_nrg_gain, pc_cap_gain),

	TP_STRUCT__entry(
		__field( int,	boost		)
		__field( int,	pb_nrg_gain	)
		__field( int,	pb_cap_gain	)
		__field( int,	pc_nrg_gain	)
		__field( int,	pc_cap_gain	)
	),

	TP_fast_assign(
		__entry->boost 	= boost;
		__entry->pb_nrg_gain	= pb_nrg_gain;
		__entry->pb_cap_gain	= pb_cap_gain;
		__entry->pc_nrg_gain	= pc_nrg_gain;
		__entry->pc_cap_gain	= pc_cap_gain;
	),

	TP_printk("boost=%d "
			"pb_nrg_gain=%d pb_cap_gain=%d "
			"pc_nrg_gain=%d pc_cap_gain=%d",
		__entry->boost,
		__entry->pb_nrg_gain, __entry->pb_cap_gain,
		__entry->pc_nrg_gain, __entry->pc_cap_gain)
);

/*
 * Tracepoint for accounting CPU  boosted utilization
 */
TRACE_EVENT(sched_boost_cpu,

	TP_PROTO(int cpu, unsigned long util, long margin),

	TP_ARGS(cpu, util, margin),

	TP_STRUCT__entry(
		__field( int,		cpu			)
		__field( unsigned long,	util			)
		__field(long,		margin			)
	),

	TP_fast_assign(
		__entry->cpu	= cpu;
		__entry->util	= util;
		__entry->margin	= margin;
	),

	TP_printk("cpu=%d util=%lu margin=%ld",
		  __entry->cpu,
		  __entry->util,
		  __entry->margin)
);

/*
 * Tracepoint for schedtune_tasks_update
 */
TRACE_EVENT(sched_tune_tasks_update,

	TP_PROTO(struct task_struct *tsk, int cpu, int tasks, int idx,
		int boost, int max_boost),

	TP_ARGS(tsk, cpu, tasks, idx, boost, max_boost),

	TP_STRUCT__entry(
		__array( char,	comm,	TASK_COMM_LEN	)
		__field( pid_t,		pid		)
		__field( int,		cpu		)
		__field( int,		tasks		)
		__field( int,		idx		)
		__field( int,		boost		)
		__field( int,		max_boost	)
	),

	TP_fast_assign(
		memcpy(__entry->comm, tsk->comm, TASK_COMM_LEN);
		__entry->pid		= tsk->pid;
		__entry->cpu 		= cpu;
		__entry->tasks		= tasks;
		__entry->idx 		= idx;
		__entry->boost		= boost;
		__entry->max_boost	= max_boost;
	),

	TP_printk("pid=%d comm=%s "
			"cpu=%d tasks=%d idx=%d boost=%d max_boost=%d",
		__entry->pid, __entry->comm,
		__entry->cpu, __entry->tasks, __entry->idx,
		__entry->boost, __entry->max_boost)
);

/*
 * Tracepoint for schedtune_boostgroup_update
 */
TRACE_EVENT(sched_tune_boostgroup_update,

	TP_PROTO(int cpu, int variation, int max_boost),

	TP_ARGS(cpu, variation, max_boost),

	TP_STRUCT__entry(
		__field( int,	cpu		)
		__field( int,	variation	)
		__field( int,	max_boost	)
	),

	TP_fast_assign(
		__entry->cpu		= cpu;
		__entry->variation	= variation;
		__entry->max_boost	= max_boost;
	),

	TP_printk("cpu=%d variation=%d max_boost=%d",
		__entry->cpu, __entry->variation, __entry->max_boost)
);

/*
 * Tracepoint for accounting task boosted utilization
 */
TRACE_EVENT(sched_boost_task,

	TP_PROTO(struct task_struct *tsk, unsigned long util, long margin),

	TP_ARGS(tsk, util, margin),

	TP_STRUCT__entry(
		__array( char,	comm,	TASK_COMM_LEN		)
		__field( pid_t,		pid			)
		__field( unsigned long,	util			)
		__field( long,		margin			)

>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	),

	TP_fast_assign(
		memcpy(__entry->comm, tsk->comm, TASK_COMM_LEN);
<<<<<<< HEAD
		__entry->tsk_pid = tsk->pid;
		__entry->tsk_load = tsk_load;
		__entry->cpu_id = cpu_id;
	),

	TP_printk("cpu-id=%d task-pid=%4d task-load=%4d comm=%s",
			__entry->cpu_id,
			__entry->tsk_pid,
			__entry->tsk_load,
			__entry->comm)
);

/*
 * Tracepoint for cfs runqueue load ratio update
 */
TRACE_EVENT(sched_cfs_load_update,

	TP_PROTO(struct task_struct *tsk, int tsk_load, int tsk_delta, int cpu_id),

	TP_ARGS(tsk, tsk_load, tsk_delta, cpu_id),

	TP_STRUCT__entry(
		__array(char, comm, TASK_COMM_LEN)
		__field(pid_t, tsk_pid)
		__field(int, tsk_load)
		__field(int, tsk_delta)
		__field(int, cpu_id)
=======
		__entry->pid	= tsk->pid;
		__entry->util	= util;
		__entry->margin	= margin;
	),

	TP_printk("comm=%s pid=%d util=%lu margin=%ld",
		  __entry->comm, __entry->pid,
		  __entry->util,
		  __entry->margin)
);

/*
 * Tracepoint for accounting sched group energy
 */
TRACE_EVENT(sched_energy_diff,

	TP_PROTO(struct task_struct *tsk, int scpu, int dcpu, int udelta,
		int nrgb, int nrga, int nrgd, int capb, int capa, int capd,
		int nrgn, int nrgp),

	TP_ARGS(tsk, scpu, dcpu, udelta,
		nrgb, nrga, nrgd, capb, capa, capd,
		nrgn, nrgp),

	TP_STRUCT__entry(
		__array( char,	comm,	TASK_COMM_LEN	)
		__field( pid_t,	pid	)
		__field( int,	scpu	)
		__field( int,	dcpu	)
		__field( int,	udelta	)
		__field( int,	nrgb	)
		__field( int,	nrga	)
		__field( int,	nrgd	)
		__field( int,	capb	)
		__field( int,	capa	)
		__field( int,	capd	)
		__field( int,	nrgn	)
		__field( int,	nrgp	)
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	),

	TP_fast_assign(
		memcpy(__entry->comm, tsk->comm, TASK_COMM_LEN);
<<<<<<< HEAD
		__entry->tsk_pid = tsk->pid;
		__entry->tsk_load = tsk_load;
		__entry->tsk_delta = tsk_delta;
		__entry->cpu_id = cpu_id;
	),

	TP_printk("cpu-id=%d task-pid=%4d task-load=%4d(%d) comm=%s",
			__entry->cpu_id,
			__entry->tsk_pid,
			__entry->tsk_load,
			__entry->tsk_delta,
			__entry->comm)
);

/*
 * Tracepoint for showing tracked cfs runqueue runnable load.
 */
TRACE_EVENT(sched_cfs_runnable_load,

	TP_PROTO(int cpu_id, int cpu_load, int cpu_ntask),

	TP_ARGS(cpu_id, cpu_load, cpu_ntask),

	TP_STRUCT__entry(
		__field(int, cpu_id)
		__field(int, cpu_load)
		__field(int, cpu_ntask)
	),

	TP_fast_assign(
		__entry->cpu_id = cpu_id;
		__entry->cpu_load = cpu_load;
		__entry->cpu_ntask = cpu_ntask;
	),

	TP_printk("cpu-id=%d cfs-load=%4d, cfs-ntask=%2d",
			__entry->cpu_id,
			__entry->cpu_load,
			__entry->cpu_ntask)
);

/*
 * Tracepoint for profiling runqueue length
 */
TRACE_EVENT(sched_runqueue_length,

	TP_PROTO(int cpu, int length),

	TP_ARGS(cpu, length),

	TP_STRUCT__entry(
		__field(int, cpu)
		__field(int, length)
	),

	TP_fast_assign(
		__entry->cpu = cpu;
		__entry->length = length;
	),

	TP_printk("cpu=%d rq-length=%2d",
			__entry->cpu,
			__entry->length)
);

TRACE_EVENT(sched_cfs_length,

	TP_PROTO(int cpu, int length),

	TP_ARGS(cpu, length),

	TP_STRUCT__entry(
		__field(int, cpu)
		__field(int, length)
	),

	TP_fast_assign(
		__entry->cpu = cpu;
		__entry->length = length;
	),

	TP_printk("cpu=%d cfs-length=%2d",
			__entry->cpu,
			__entry->length)
);
#endif /* CONFIG_HMP_TRACER */

/*
 * Tracepoint for showing tracked rq runnable ratio [0..1023].
 */
TRACE_EVENT(sched_rq_runnable_ratio,

	TP_PROTO(int cpu, unsigned long ratio),

	TP_ARGS(cpu, ratio),

	TP_STRUCT__entry(
		__field(int, cpu)
		__field(unsigned long, ratio)
	),

	TP_fast_assign(
		__entry->cpu   = cpu;
		__entry->ratio = ratio;
	),

	TP_printk("cpu=%d ratio=%lu",
			__entry->cpu,
			__entry->ratio)
);

/*
 * Tracepoint for showing tracked rq runnable load.
 */
TRACE_EVENT(sched_rq_runnable_load,

	TP_PROTO(int cpu, u64 load),

	TP_ARGS(cpu, load),

	TP_STRUCT__entry(
		__field(int, cpu)
		__field(u64, load)
	),

	TP_fast_assign(
		__entry->cpu  = cpu;
		__entry->load = load;
	),

	TP_printk("cpu=%d load=%llu",
			__entry->cpu,
			__entry->load)
);

/*
 * Tracepoint for showing tracked task cpu usage ratio [0..1023].
 */
TRACE_EVENT(sched_task_usage_ratio,

	TP_PROTO(struct task_struct *tsk, unsigned long ratio),

	TP_ARGS(tsk, ratio),

	TP_STRUCT__entry(
		__array(char, comm, TASK_COMM_LEN)
		__field(pid_t, pid)
		__field(unsigned long, ratio)
	),

	TP_fast_assign(
	memcpy(__entry->comm, tsk->comm, TASK_COMM_LEN);
		__entry->pid   = tsk->pid;
		__entry->ratio = ratio;
	),

	TP_printk("comm=%s pid=%d ratio=%lu",
			__entry->comm, __entry->pid,
			__entry->ratio)
);

TRACE_EVENT(sched_heavy_task,
	    TP_PROTO(const char *s),
	    TP_ARGS(s),
	    TP_STRUCT__entry(
		    __string(s, s)
	    ),
	    TP_fast_assign(
		    __assign_str(s, s);
	    ),
	    TP_printk("%s", __get_str(s))
);
=======
		__entry->pid		= tsk->pid;
		__entry->scpu 		= scpu;
		__entry->dcpu 		= dcpu;
		__entry->udelta 	= udelta;
		__entry->nrgb 		= nrgb;
		__entry->nrga 		= nrga;
		__entry->nrgd 		= nrgd;
		__entry->capb 		= capb;
		__entry->capa 		= capa;
		__entry->capd 		= capd;
		__entry->nrgn 		= nrgn;
		__entry->nrgp 		= nrgp;
	),

	TP_printk("pid=%d comm=%s "
			"src_cpu=%d dst_cpu=%d usage_delta=%d "
			"nrg_before=%d nrg_after=%d nrg_diff=%d "
			"cap_before=%d cap_after=%d cap_delta=%d "
			"nrg_delta=%d nrg_payoff=%d",
		__entry->pid, __entry->comm,
		__entry->scpu, __entry->dcpu, __entry->udelta,
		__entry->nrgb, __entry->nrga, __entry->nrgd,
		__entry->capb, __entry->capa, __entry->capd,
		__entry->nrgn, __entry->nrgp)
);

/*
 * Tracepoint for schedtune_tasks_update
 */
TRACE_EVENT(sched_tune_filter,

	TP_PROTO(int nrg_delta, int cap_delta,
		 int nrg_gain,  int cap_gain,
		 int payoff, int region),

	TP_ARGS(nrg_delta, cap_delta, nrg_gain, cap_gain, payoff, region),

	TP_STRUCT__entry(
		__field( int,	nrg_delta	)
		__field( int,	cap_delta	)
		__field( int,	nrg_gain	)
		__field( int,	cap_gain	)
		__field( int,	payoff		)
		__field( int,	region		)
	),

	TP_fast_assign(
		__entry->nrg_delta	= nrg_delta;
		__entry->cap_delta	= cap_delta;
		__entry->nrg_gain	= nrg_gain;
		__entry->cap_gain	= cap_gain;
		__entry->payoff		= payoff;
		__entry->region		= region;
	),

	TP_printk("nrg_delta=%d cap_delta=%d nrg_gain=%d cap_gain=%d payoff=%d region=%d",
		__entry->nrg_delta, __entry->cap_delta,
		__entry->nrg_gain, __entry->cap_gain,
		__entry->payoff, __entry->region)
);

/*
 * Tracepoint for system overutilized flag
 */
TRACE_EVENT(sched_overutilized,

	TP_PROTO(bool overutilized),

	TP_ARGS(overutilized),

	TP_STRUCT__entry(
		__field( bool,	overutilized	)
	),

	TP_fast_assign(
		__entry->overutilized	= overutilized;
	),

	TP_printk("overutilized=%d",
		__entry->overutilized ? 1 : 0)
);
#ifdef CONFIG_SCHED_WALT
struct rq;

extern unsigned int sched_ravg_window;
TRACE_EVENT(walt_update_task_ravg,

	TP_PROTO(struct task_struct *p, struct rq *rq, int evt,
		 u64 wallclock, u64 irqtime),

	TP_ARGS(p, rq, evt, wallclock, irqtime),

	TP_STRUCT__entry(
		__array(char,		comm,   TASK_COMM_LEN	)
		__field(pid_t,		pid			)
		__field(pid_t,		cur_pid			)
		__field(unsigned int,	cur_freq		)
		__field(u64,		wallclock		)
		__field(u64,		mark_start		)
		__field(u64,		win_start		)
		__field(u64,		irqtime			)
		__field(int,    	evt			)
		__field(unsigned int,	demand			)
		__field(unsigned int,	sum			)
		__field(unsigned int,   walt_avg                )
		__field(int,		cpu			)
		__field(u64,		cs			)
		__field(u64,		ps			)
		__field(u32,		curr_window		)
		__field(u32,		prev_window		)
		__field(u32,		active_windows		)
	),

	TP_fast_assign(
		__entry->wallclock      = wallclock;
		__entry->win_start      = rq->window_start;
		__entry->evt            = evt;
		__entry->cpu            = rq->cpu;
		__entry->cur_pid        = rq->curr->pid;
		__entry->cur_freq       = rq->cur_freq;
		memcpy(__entry->comm, 	  p->comm, TASK_COMM_LEN);
		__entry->pid            = p->pid;
		__entry->mark_start     = p->ravg.mark_start;
		__entry->demand         = p->ravg.demand;
		__entry->sum            = p->ravg.sum;
		__entry->walt_avg       = p->ravg.demand << 10;
		do_div(__entry->walt_avg, walt_ravg_window);
		__entry->irqtime        = irqtime;
		__entry->cs             = rq->curr_runnable_sum;
		__entry->ps             = rq->prev_runnable_sum;
		__entry->curr_window	= p->ravg.curr_window;
		__entry->prev_window	= p->ravg.prev_window;
		__entry->active_windows	= p->ravg.active_windows;
	),

	TP_printk("wclock=%llu win_start=%llu event=%d cpu=%d "
		  "cur_freq=%u cur_pid=%d pid=%d comm=%s mrk_start=%llu "
		  "demand=%u sum=%u walt_avg=%u irqtime=%llu "
		  "cur_rsum=%llu pre_rsum=%llu "
		  "cur_wdw=%u pre_wdw=%u act_wds=%u",
		__entry->wallclock, __entry->win_start,
		__entry->evt, __entry->cpu,
		__entry->cur_freq, __entry->cur_pid,
		__entry->pid, __entry->comm, __entry->mark_start,
		__entry->demand, __entry->sum,
		__entry->walt_avg, __entry->irqtime,
		__entry->cs, __entry->ps,
		__entry->curr_window, __entry->prev_window,
		__entry->active_windows
		)
);

TRACE_EVENT(walt_update_history,

	TP_PROTO(struct rq *rq, struct task_struct *p, u32 runtime, int samples,
			int evt),

	TP_ARGS(rq, p, runtime, samples, evt),

	TP_STRUCT__entry(
		__array(char,		comm,   TASK_COMM_LEN	)
		__field(pid_t,		pid			)
		__field(unsigned int,	runtime			)
		__field(int,		samples			)
		__field(int,		evt			)
		__field(u64,		demand			)
		__field(unsigned int,	walt_avg		)
		__field(unsigned int,	pelt_avg		)
		__array(u32,		hist, RAVG_HIST_SIZE_MAX)
		__field(int,		cpu			)
	),

	TP_fast_assign(
		memcpy(__entry->comm, 	  p->comm, TASK_COMM_LEN);
		__entry->pid            = p->pid;
		__entry->runtime        = runtime;
		__entry->samples        = samples;
		__entry->evt            = evt;
		__entry->demand         = p->ravg.demand;
		__entry->walt_avg 	= p->ravg.demand << 10;
		do_div(__entry->walt_avg, walt_ravg_window);
		__entry->pelt_avg	= p->se.avg.util_avg;
		memcpy(__entry->hist, 	  p->ravg.sum_history,
					  RAVG_HIST_SIZE_MAX * sizeof(u32));
		__entry->cpu            = rq->cpu;
	),

	TP_printk("pid=%d comm=%s runtime=%u samples=%d event=%d demand=%llu "
		  "walt=%u pelt=%u h0=%u h1=%u h2=%u h3=%u h4=%u cpu=%d",
		__entry->pid, __entry->comm,
		__entry->runtime, __entry->samples, __entry->evt,
		__entry->demand,
		__entry->walt_avg,
		__entry->pelt_avg,
		__entry->hist[0], __entry->hist[1],
		__entry->hist[2], __entry->hist[3],
		__entry->hist[4], __entry->cpu)
);

TRACE_EVENT(walt_migration_update_sum,

	TP_PROTO(struct rq *rq, struct task_struct *p),

	TP_ARGS(rq, p),

	TP_STRUCT__entry(
		__field(int,		cpu			)
		__array(char,		comm,   TASK_COMM_LEN	)
		__field(int,		pid			)
		__field(u64,		cs			)
		__field(u64,		ps			)
	),

	TP_fast_assign(
		__entry->cpu		= cpu_of(rq);
		__entry->cs		= rq->curr_runnable_sum;
		__entry->ps		= rq->prev_runnable_sum;
		__entry->pid		= p->pid;
		memcpy(__entry->comm,	  p->comm, TASK_COMM_LEN);
	),

	TP_printk("cpu=%d pid=%d comm=%s "
		  "cur_ravg=%llu pre_ravg=%llu",
		  __entry->cpu, __entry->pid, __entry->comm,
		  __entry->cs, __entry->ps)
);
#endif /* CONFIG_SCHED_WALT */

#endif /* CONFIG_SMP */

>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
#endif /* _TRACE_SCHED_H */

/* This part must be outside protection */
#include <trace/define_trace.h>
