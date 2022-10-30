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

#include "ScopeExit.h"

extern "C" {
#include "libdrm_trace.h"
}
#include "libdrmStub.h"
#include "libdrmProxy.h"


static bool doTrace = false;


// #pragma mark - driver loader

struct DriverHooks {
	status_t (*Init)();
	void *(*Mmap)(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
	int (*Munmap)(void *addr, size_t length);
	int (*Ioctl)(int fd, unsigned long request, void *arg);
};

void *gDriverImage = NULL;
DriverHooks gDriverHooks = {0};

static status_t LoadDriver()
{
	gDriverImage = dlopen("/boot/home/Tests/GL/RadeonGfx/build.x86_64/RadeonGfx", RTLD_NOW | RTLD_LOCAL);
	if (gDriverImage == NULL) return B_ERROR;
	*(void**)&gDriverHooks.Init   = dlsym(gDriverImage, "drmInit");   if (gDriverHooks.Init   == NULL) return B_ERROR;
	*(void**)&gDriverHooks.Mmap   = dlsym(gDriverImage, "drmMmap");   if (gDriverHooks.Mmap   == NULL) return B_ERROR;
	*(void**)&gDriverHooks.Munmap = dlsym(gDriverImage, "drmMunmap"); if (gDriverHooks.Munmap == NULL) return B_ERROR;
	*(void**)&gDriverHooks.Ioctl  = dlsym(gDriverImage, "drmIoctl");  if (gDriverHooks.Ioctl  == NULL) return B_ERROR;
	status_t res = gDriverHooks.Init();
	if (res < B_OK) return res;

	return B_OK;
}

static status_t LoadProxy()
{
	gDriverHooks.Init   = drmProxyInit;
	gDriverHooks.Mmap   = drmMmapProxy;
	gDriverHooks.Munmap = drmMunmapProxy;
	gDriverHooks.Ioctl  = drmIoctlProxy;
	status_t res = gDriverHooks.Init();
	if (res < B_OK) return res;

	return B_OK;
}

class DriverLoader {
public:
	DriverLoader(bool useProxy)
	{
		status_t res;
		const char* trace = getenv("LIBDRM_TRACE");
		doTrace = (trace != NULL);
		if (useProxy) {
			res = LoadProxy();
		} else {
			res = LoadDriver();
		}
		if (res < B_OK) {
			fprintf(stderr, "[!] libdrm2: can't load driver\n");
			memset(&gDriverHooks, 0, sizeof(gDriverHooks));
			//abort();
		}
	}

	~DriverLoader()
	{
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
	if (gDriverHooks.Mmap != NULL) {
		return gDriverHooks.Mmap(addr, length, prot, flags, fd, offset);
	}
	return drmMmapStub(addr, length, prot, flags, fd, offset);
}

drm_public int drmMunmap(void *addr, size_t length)
{
	if (gDriverHooks.Munmap != NULL) {
		return gDriverHooks.Munmap(addr, length);
	}
	return drmMunmapStub(addr, length);
}

drm_public int drmIoctl(int fd, unsigned long request, void *arg)
{
	if (doTrace /*&& find_thread(NULL) == getpid()*/) {
		setvbuf(stdout, NULL, _IONBF, 0);
		drmIoctlTrace(fd, request, arg, false);
	}
	const auto& scopeExit = MakeScopeExit([&]() {
		if (doTrace /*&& find_thread(NULL) == getpid()*/) {
			drmIoctlTrace(fd, request, arg, true);
		}
	});

	if (gDriverHooks.Ioctl != NULL) {
		int ret = gDriverHooks.Ioctl(fd, request, arg);
		//printf(", %d ", ret);
		if (ret != ENOSYS) {
			return ret;
		}
	}

	return drmIoctlStub(fd, request, arg);
}
