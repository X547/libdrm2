#ifndef _LIBDRM_TRACE_H_
#define _LIBDRM_TRACE_H_

#include <stdbool.h>

void drmIoctlTrace(int fd, unsigned long request, void *arg, bool isPost);

#endif	// _LIBDRM_TRACE_H_
