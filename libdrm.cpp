#define HAVE_VISIBILITY 1

extern "C" {
#include <xf86drm.h>
#include "libdrm_macros.h"
}
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dlfcn.h>

#include <SupportDefs.h>
#include <OS.h>
#include <AccelerantRoster.h>

#include "ScopeExit.h"

extern "C" {
#include "libdrm_trace.h"
}
#include "libdrmStub.h"

#define CheckRet(err) {status_t _err = (err); if (_err < B_OK) return _err;}


static bool doTrace = false;


// #pragma mark - driver loader

class DriverLoader {
public:
	DriverLoader(bool useProxy)
	{
		const char* trace = getenv("LIBDRM_TRACE");
		doTrace = (trace != NULL);
	}
} gDriverLoader(true);


// #pragma mark - debug

drm_public void drmMsg(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);
}


// #pragma mark - ioctl interface

drm_public int drmIoctl(int fd, unsigned long request, void *arg)
{
	return drmIoctlStub(fd, request, arg);
}
