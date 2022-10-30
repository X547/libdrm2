#include "memory.h"

#include <stdio.h>
#include <map>


std::map<uint32_t, BReference<GpuBuffer> > gGpuBufferHandleTable;
uint32_t gLastGpuBufferHandleTable = 1;

std::map<uint64_t, GpuMapping> gGpuBufferMappingTable;


BReference<GpuBuffer> ThisGpuBuffer(uint32_t handle)
{
	auto it = gGpuBufferHandleTable.find(handle);
	if (it == gGpuBufferHandleTable.end()) return NULL;
	return it->second;
}

BReference<GpuBuffer> ThisMappedGpuBuffer(uint64_t mapAdr, uint64_t &offset)
{
	printf("ThisMappedGpuBuffer(%#" PRIx64 ")\n", mapAdr);
	auto it = gGpuBufferMappingTable.upper_bound(mapAdr);
	it--;
	// if (it == gGpuBufferMappingTable.end()) return NULL;
	uint64_t bufferMapAdr = it->first;
	GpuMapping& mapping = it->second;
	if (!(mapAdr < bufferMapAdr + mapping.size)) return NULL;
	printf("  bufferMapAdr: %#" PRIx64 "\n", bufferMapAdr);
	offset = mapping.offset + mapAdr - bufferMapAdr;
	return mapping.buffer;
}

status_t CreateGpuBuffer(uint64_t size, uint64_t alignment, uint64_t domains, uint64_t flags, uint32_t &_handle)
{
	BReference<GpuBuffer> buffer(new GpuBuffer(), true);
	if (!buffer.IsSet()) return B_NO_MEMORY;

	buffer->area.SetTo(create_area("GPU buffer", &buffer->adr, B_ANY_ADDRESS, size, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA));
	buffer->size = size;
	buffer->alignment = alignment;
	buffer->domains = domains;
	buffer->flags = flags;
	
	if (!buffer->area.IsSet()) return B_NO_MEMORY;

	uint32_t handle = gLastGpuBufferHandleTable++;
	gGpuBufferHandleTable.emplace(handle, buffer);
	
	_handle = handle;
	return B_OK;
}

status_t CloseGpuBuffer(uint32_t handle)
{
	auto it = gGpuBufferHandleTable.find(handle);
	if (it == gGpuBufferHandleTable.end()) return B_ERROR;
	gGpuBufferHandleTable.erase(it);
	return B_OK;
}


status_t MapGpuBuffer(uint32_t handle, uint32_t flags, uint64_t mapAdr, uint64_t bufOffset, uint64_t mapSize)
{
	BReference<GpuBuffer> buffer = ThisGpuBuffer(handle);
	if (!buffer.IsSet()) return B_ERROR;
	gGpuBufferMappingTable.emplace(mapAdr, GpuMapping(buffer, bufOffset, mapSize));
	return B_OK;
}

status_t UnmapGpuBuffer(uint32_t handle, uint32_t flags, uint64_t mapAdr, uint64_t bufOffset, uint64_t mapSize)
{
	auto it = gGpuBufferMappingTable.find(mapAdr);
	if (it == gGpuBufferMappingTable.end()) return B_ERROR;
	gGpuBufferMappingTable.erase(it);
	return B_OK;
}


void DumpMappings()
{
	printf("mappings:\n");
	for (auto it = gGpuBufferMappingTable.begin(); it != gGpuBufferMappingTable.end(); it++) {
		printf("  0x%09" PRIx64 " - 0x%09" PRIx64 "\n", it->first, it->first + it->second.size - 1);
	}
}
