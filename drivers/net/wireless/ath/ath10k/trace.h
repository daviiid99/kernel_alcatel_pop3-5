/*
 * Copyright (c) 2005-2011 Atheros Communications Inc.
 * Copyright (c) 2011-2013 Qualcomm Atheros, Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#if !defined(_TRACE_H_) || defined(TRACE_HEADER_MULTI_READ)

#include <linux/tracepoint.h>
#include "core.h"

#define _TRACE_H_

/* create empty functions when tracing is disabled */
#if !defined(CONFIG_ATH10K_TRACING)
#undef TRACE_EVENT
#define TRACE_EVENT(name, proto, ...) \
static inline void trace_ ## name(proto) {}
#undef DECLARE_EVENT_CLASS
#define DECLARE_EVENT_CLASS(...)
#undef DEFINE_EVENT
#define DEFINE_EVENT(evt_class, name, proto, ...) \
static inline void trace_ ## name(proto) {}
#endif /* !CONFIG_ATH10K_TRACING || __CHECKER__ */

#undef TRACE_SYSTEM
#define TRACE_SYSTEM ath10k

#define ATH10K_MSG_MAX 200

DECLARE_EVENT_CLASS(ath10k_log_event,
	TP_PROTO(struct ath10k *ar, struct va_format *vaf),
	TP_ARGS(ar, vaf),
	TP_STRUCT__entry(
		__string(device, dev_name(ar->dev))
		__string(driver, dev_driver_string(ar->dev))
		__dynamic_array(char, msg, ATH10K_MSG_MAX)
	),
	TP_fast_assign(
		__assign_str(device, dev_name(ar->dev));
		__assign_str(driver, dev_driver_string(ar->dev));
		WARN_ON_ONCE(vsnprintf(__get_dynamic_array(msg),
				       ATH10K_MSG_MAX,
				       vaf->fmt,
				       *vaf->va) >= ATH10K_MSG_MAX);
	),
	TP_printk(
		"%s %s %s",
		__get_str(driver),
		__get_str(device),
		__get_str(msg)
	)
);

DEFINE_EVENT(ath10k_log_event, ath10k_log_err,
	     TP_PROTO(struct ath10k *ar, struct va_format *vaf),
	     TP_ARGS(ar, vaf)
);

DEFINE_EVENT(ath10k_log_event, ath10k_log_warn,
	     TP_PROTO(struct ath10k *ar, struct va_format *vaf),
	     TP_ARGS(ar, vaf)
);

DEFINE_EVENT(ath10k_log_event, ath10k_log_info,
	     TP_PROTO(struct ath10k *ar, struct va_format *vaf),
	     TP_ARGS(ar, vaf)
);

TRACE_EVENT(ath10k_log_dbg,
	TP_PROTO(struct ath10k *ar, unsigned int level, struct va_format *vaf),
	TP_ARGS(ar, level, vaf),
	TP_STRUCT__entry(
		__string(device, dev_name(ar->dev))
		__string(driver, dev_driver_string(ar->dev))
		__field(unsigned int, level)
		__dynamic_array(char, msg, ATH10K_MSG_MAX)
	),
	TP_fast_assign(
		__assign_str(device, dev_name(ar->dev));
		__assign_str(driver, dev_driver_string(ar->dev));
		__entry->level = level;
		WARN_ON_ONCE(vsnprintf(__get_dynamic_array(msg),
				       ATH10K_MSG_MAX,
				       vaf->fmt,
				       *vaf->va) >= ATH10K_MSG_MAX);
	),
	TP_printk(
		"%s %s %s",
		__get_str(driver),
		__get_str(device),
		__get_str(msg)
	)
);

TRACE_EVENT(ath10k_log_dbg_dump,
	TP_PROTO(struct ath10k *ar, const char *msg, const char *prefix,
		 const void *buf, size_t buf_len),

	TP_ARGS(ar, msg, prefix, buf, buf_len),

	TP_STRUCT__entry(
		__string(device, dev_name(ar->dev))
		__string(driver, dev_driver_string(ar->dev))
		__string(msg, msg)
		__string(prefix, prefix)
		__field(size_t, buf_len)
		__dynamic_array(u8, buf, buf_len)
	),

	TP_fast_assign(
		__assign_str(device, dev_name(ar->dev));
		__assign_str(driver, dev_driver_string(ar->dev));
		__assign_str(msg, msg);
		__assign_str(prefix, prefix);
		__entry->buf_len = buf_len;
		memcpy(__get_dynamic_array(buf), buf, buf_len);
	),

	TP_printk(
		"%s %s %s/%s\n",
		__get_str(driver),
		__get_str(device),
		__get_str(prefix),
		__get_str(msg)
	)
);

TRACE_EVENT(ath10k_wmi_cmd,
<<<<<<< HEAD
	TP_PROTO(struct ath10k *ar, int id, void *buf, size_t buf_len, int ret),

	TP_ARGS(ar, id, buf, buf_len, ret),
=======
	TP_PROTO(struct ath10k *ar, int id, void *buf, size_t buf_len),

	TP_ARGS(ar, id, buf, buf_len),
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916

	TP_STRUCT__entry(
		__string(device, dev_name(ar->dev))
		__string(driver, dev_driver_string(ar->dev))
		__field(unsigned int, id)
		__field(size_t, buf_len)
		__dynamic_array(u8, buf, buf_len)
<<<<<<< HEAD
		__field(int, ret)
=======
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	),

	TP_fast_assign(
		__assign_str(device, dev_name(ar->dev));
		__assign_str(driver, dev_driver_string(ar->dev));
		__entry->id = id;
		__entry->buf_len = buf_len;
<<<<<<< HEAD
		__entry->ret = ret;
=======
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
		memcpy(__get_dynamic_array(buf), buf, buf_len);
	),

	TP_printk(
<<<<<<< HEAD
		"%s %s id %d len %zu ret %d",
		__get_str(driver),
		__get_str(device),
		__entry->id,
		__entry->buf_len,
		__entry->ret
=======
		"%s %s id %d len %zu",
		__get_str(driver),
		__get_str(device),
		__entry->id,
		__entry->buf_len
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	)
);

TRACE_EVENT(ath10k_wmi_event,
	TP_PROTO(struct ath10k *ar, int id, void *buf, size_t buf_len),

	TP_ARGS(ar, id, buf, buf_len),

	TP_STRUCT__entry(
		__string(device, dev_name(ar->dev))
		__string(driver, dev_driver_string(ar->dev))
		__field(unsigned int, id)
		__field(size_t, buf_len)
		__dynamic_array(u8, buf, buf_len)
	),

	TP_fast_assign(
		__assign_str(device, dev_name(ar->dev));
		__assign_str(driver, dev_driver_string(ar->dev));
		__entry->id = id;
		__entry->buf_len = buf_len;
		memcpy(__get_dynamic_array(buf), buf, buf_len);
	),

	TP_printk(
		"%s %s id %d len %zu",
		__get_str(driver),
		__get_str(device),
		__entry->id,
		__entry->buf_len
	)
);

TRACE_EVENT(ath10k_htt_stats,
	TP_PROTO(struct ath10k *ar, void *buf, size_t buf_len),

	TP_ARGS(ar, buf, buf_len),

	TP_STRUCT__entry(
		__string(device, dev_name(ar->dev))
		__string(driver, dev_driver_string(ar->dev))
		__field(size_t, buf_len)
		__dynamic_array(u8, buf, buf_len)
	),

	TP_fast_assign(
		__assign_str(device, dev_name(ar->dev));
		__assign_str(driver, dev_driver_string(ar->dev));
		__entry->buf_len = buf_len;
		memcpy(__get_dynamic_array(buf), buf, buf_len);
	),

	TP_printk(
		"%s %s len %zu",
		__get_str(driver),
		__get_str(device),
		__entry->buf_len
	)
);

TRACE_EVENT(ath10k_wmi_dbglog,
	TP_PROTO(struct ath10k *ar, void *buf, size_t buf_len),

	TP_ARGS(ar, buf, buf_len),

	TP_STRUCT__entry(
		__string(device, dev_name(ar->dev))
		__string(driver, dev_driver_string(ar->dev))
		__field(size_t, buf_len)
		__dynamic_array(u8, buf, buf_len)
	),

	TP_fast_assign(
		__assign_str(device, dev_name(ar->dev));
		__assign_str(driver, dev_driver_string(ar->dev));
		__entry->buf_len = buf_len;
		memcpy(__get_dynamic_array(buf), buf, buf_len);
	),

	TP_printk(
		"%s %s len %zu",
		__get_str(driver),
		__get_str(device),
		__entry->buf_len
	)
);

#endif /* _TRACE_H_ || TRACE_HEADER_MULTI_READ*/

/* we don't want to use include/trace/events */
#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH .
#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_FILE trace

/* This part must be outside protection */
#include <trace/define_trace.h>
