#pragma once

#include <stdint.h>
#include <sys/types.h>
#include <SupportDefs.h>

void *drmMmapStub(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
int drmMunmapStub(void *addr, size_t length);
int drmIoctlStub(int fd, unsigned long request, void *arg);
