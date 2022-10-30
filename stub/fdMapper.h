#pragma once
#include <SupportDefs.h>

#if defined(__cplusplus)
extern "C" {
#endif

status_t _EXPORT drmFdClose(int devFd, int fd);
status_t drmFdMapGemHandle(int devFd, int *fd, uint32_t handle);
status_t drmFdUnmapGemHandle(int devFd, uint32_t *handle, int fd);
status_t drmFdMapSyncobjHandle(int devFd, int *fd, uint32_t handle);
status_t drmFdUnmapSyncobjHandle(int devFd, uint32_t *handle, int fd);

#if defined(__cplusplus)
}
#endif
