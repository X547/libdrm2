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

drm_public void *drmMmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset)
{
	Accelerant *acc{};
	if (AccelerantRoster::Instance().FromFd(acc, fd) < B_OK) return NULL;
	BReference<Accelerant> accDeleter(acc, true);
	AccelerantDrm *accDrm = dynamic_cast<AccelerantDrm*>(acc);
	if (accDrm == NULL) return NULL;
	return accDrm->DrmMmap(addr, length, prot, flags, offset);
}

drm_public int drmMunmap(void *addr, size_t length)
{
	return munmap(addr, length);
}

drm_public int drmIoctl(int fd, unsigned long request, void *arg)
{
	if (doTrace /*&& find_thread(NULL) == getpid()*/) {
		setvbuf(stdout, NULL, _IONBF, 0);
		drmIoctlTrace(fd, request, arg, false);
	}
	ScopeExit scopeExit{[&]() {
		if (doTrace /*&& find_thread(NULL) == getpid()*/) {
			drmIoctlTrace(fd, request, arg, true);
		}
	}};

	Accelerant *acc{};
	CheckRet(AccelerantRoster::Instance().FromFd(acc, fd));
	BReference<Accelerant> accDeleter(acc, true);
	AccelerantDrm *accDrm = dynamic_cast<AccelerantDrm*>(acc);
	if (accDrm == NULL) return ENOSYS;
	int res = accDrm->DrmIoctl(request, arg);
	if (res != ENOSYS) return res;
	return drmIoctlStub(fd, request, arg);
}
