#include "fdMapper.h"
#include <pthread.h>
#include <private/shared/AutoDeleterPosix.h>
#include <private/shared/PthreadMutexLocker.h>

#include <map>


enum class HandleType {
	gem,
	syncobj,
};

struct FdMapEntry {
	HandleType type;
	uint32_t handle;
};

static pthread_mutex_t sFdMapLock = PTHREAD_MUTEX_INITIALIZER;
static std::map<int, FdMapEntry> sFdMap;


status_t drmFdClose(int devFd, int fd)
{
	PthreadMutexLocker lock(&sFdMapLock);
	auto it = sFdMap.find(fd);
	if (it == sFdMap.end()) {
		return ENOENT;
	}
	close(it->first);
	sFdMap.erase(it);
	return B_OK;
}

static status_t drmFdMapHandle(int devFd, int *fd, HandleType handleType, uint32_t handle)
{
	*fd = open("/dev/null", O_CLOEXEC);
	PthreadMutexLocker lock(&sFdMapLock);
	sFdMap.emplace(*fd, FdMapEntry{.type = handleType, .handle = handle});

	return B_OK;
}

static status_t drmFdUnmapHandle(int devFd, uint32_t *handle, HandleType handleType, int fd)
{
	PthreadMutexLocker lock(&sFdMapLock);
	auto it = sFdMap.find(fd);
	if (it == sFdMap.end()) {
		return ENOENT;
	}
	FdMapEntry &entry = it->second;
	if (entry.type != handleType) {
		return ENOENT;
	}
	sFdMap.erase(it);

	*handle = entry.handle;
	return B_OK;
}

status_t drmFdMapGemHandle(int devFd, int *fd, uint32_t handle)
{
	return drmFdMapHandle(devFd, fd, HandleType::gem, handle);
}

status_t drmFdUnmapGemHandle(int devFd, uint32_t *handle, int fd)
{
	return drmFdUnmapHandle(devFd, handle, HandleType::gem, fd);
}

status_t drmFdMapSyncobjHandle(int devFd, int *fd, uint32_t handle)
{
	return drmFdMapHandle(devFd, fd, HandleType::syncobj, handle);
}

status_t drmFdUnmapSyncobjHandle(int devFd, uint32_t *handle, int fd)
{
	return drmFdUnmapHandle(devFd, handle, HandleType::syncobj, fd);
}
