#pragma once

#include <SupportDefs.h>
#include <Referenceable.h>
#include <AutoDeleterOS.h>


struct GpuBuffer: public BReferenceable {
	AreaDeleter area;
	void* adr;
	uint64_t size;
	uint64_t alignment;
	uint64_t domains;
	uint64_t flags;
};

struct GpuMapping {
	BReference<GpuBuffer> buffer;
	uint64_t offset;
	uint64 size;

	GpuMapping(BReference<GpuBuffer> buffer, uint64_t offset, uint64_t size):
		buffer(buffer), offset(offset), size(size)
	{}
};


BReference<GpuBuffer> ThisGpuBuffer(uint32_t handle);
BReference<GpuBuffer> ThisMappedGpuBuffer(uint64_t mapAdr, uint64_t &offset);

status_t CreateGpuBuffer(uint64_t size, uint64_t alignment, uint64_t domains, uint64_t flags, uint32_t &handle);
status_t CloseGpuBuffer(uint32_t handle);
status_t MapGpuBuffer(uint32_t handle, uint32_t flags, uint64_t mapAdr, uint64_t bufOffset, uint64_t mapSize);
status_t UnmapGpuBuffer(uint32_t handle, uint32_t flags, uint64_t mapAdr, uint64_t bufOffset, uint64_t mapSize);

void DumpMappings();
