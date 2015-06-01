#ifndef __BACKPORT_IW_HANDLER_H
#define __BACKPORT_IW_HANDLER_H
#include_next <net/iw_handler.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,1,0)
static inline char *
iwe_stream_add_event_check(struct iw_request_info *info, char *stream,
			   char *ends, struct iw_event *iwe, int event_len)
{
	char *res = iwe_stream_add_event(info, stream, ends, iwe, event_len);

	if (res == stream)
		return ERR_PTR(-E2BIG);
	return res;
}

static inline char *
iwe_stream_add_point_check(struct iw_request_info *info, char *stream,
			   char *ends, struct iw_event *iwe, char *extra)
{
	char *res = iwe_stream_add_point(info, stream, ends, iwe, extra);

	if (res == stream)
		return ERR_PTR(-E2BIG);
	return res;
}
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(4,1,0) */
#endif /* __BACKPORT_IW_HANDLER_H */
