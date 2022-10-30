#pragma once

#include <stdint.h>
#include <sys/types.h>
#include <OS.h>

enum RadeonHandleType {
	radeonHandleBuffer,
	radeonHandleSyncobj,
};

status_t drmProxyInit();

void *drmMmapProxy(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
int drmMunmapProxy(void *addr, size_t length);
int drmIoctlProxy(int fd, unsigned long request, void *arg);
extern "C" _EXPORT status_t radeonGfxDuplicateHandle(int fd, uint32_t *dstHandle, uint32_t srcHandle, RadeonHandleType handleType, team_id dstTeam, team_id srcTeam);
