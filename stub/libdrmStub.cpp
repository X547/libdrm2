#define HAVE_VISIBILITY 1

extern "C" {
#include <xf86drm.h>
#include "libdrm_macros.h"
#include <libdrm/amdgpu_drm.h>
}
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <dlfcn.h>

#include <SupportDefs.h>
#include <OS.h>

#include "radeon_hd.h"
#include "radeon_hd_regs.h"

extern "C" {
#include "libdrm_trace.h"
}
#include "libdrmStub.h"
#include "memory.h"
#include "fdMapper.h"

#define memclear(s) memset(&s, 0, sizeof(s))


// #pragma mark - ioctl interface

void *drmMmapStub(void *addr, size_t length, int prot, int flags, int fd, off_t offset)
{
	BReference<GpuBuffer> buffer = ThisGpuBuffer(offset);
	if (!buffer.IsSet()) return NULL;
	return buffer->adr;
}

int drmMunmapStub(void *addr, size_t length)
{
	return 0;
}


static int drmAmdgpuReadMmrReg(uint32_t offset, uint32_t *value)
{
	switch (offset) {
		case 0x263d: *value = 0x1; return 0;
		case 0xa0d4: *value = 0x1240; return 0;
		case 0x263e: *value = 0x12010002; return 0;
		
		case 0x2644 + 0x0: *value = 0x3a0112; return 0;
		case 0x2644 + 0x1: *value = 0x3a0912; return 0;
		case 0x2644 + 0x2: *value = 0x3a1112; return 0;
		case 0x2644 + 0x3: *value = 0x212912; return 0;
		case 0x2644 + 0x4: *value = 0x10a; return 0;
		case 0x2644 + 0x5: *value = 0x111912; return 0;
		case 0x2644 + 0x6: *value = 0x121112; return 0;
		case 0x2644 + 0x7: *value = 0x22112; return 0;
		case 0x2644 + 0x8: *value = 0x4; return 0;
		case 0x2644 + 0x9: *value = 0x108; return 0;
		case 0x2644 + 0xa: *value = 0x3a1110; return 0;
		case 0x2644 + 0xb: *value = 0x351110; return 0;
		case 0x2644 + 0xc: *value = 0x341910; return 0;
		case 0x2644 + 0xd: *value = 0x109; return 0;
		case 0x2644 + 0xe: *value = 0x361111; return 0;
		case 0x2644 + 0xf: *value = 0x351111; return 0;
		case 0x2644 + 0x10: *value = 0x341911; return 0;
		case 0x2644 + 0x11: *value = 0x342911; return 0;
		case 0x2644 + 0x12: *value = 0x10d; return 0;
		case 0x2644 + 0x13: *value = 0x342921; return 0;
		case 0x2644 + 0x14: *value = 0x34291d; return 0;
		case 0x2644 + 0x15: *value = 0x221111; return 0;
		case 0x2644 + 0x16: *value = 0x211111; return 0;
		case 0x2644 + 0x17: *value = 0x121111; return 0;
		case 0x2644 + 0x18: *value = 0x111911; return 0;
		case 0x2644 + 0x19: *value = 0x22111; return 0;
		case 0x2644 + 0x1a: *value = 0x22111; return 0;
		case 0x2644 + 0x1b: *value = 0x22111; return 0;
		case 0x2644 + 0x1c: *value = 0x22111; return 0;
		case 0x2644 + 0x1d: *value = 0x22111; return 0;
		case 0x2644 + 0x1e: *value = 0x12911; return 0;
		case 0x2644 + 0x1f: *value = 0; return 0;
		
		case 0x9d8: *value = 0x262; return 0;
	}
	return -1;
}

static void DumpBytes(uint8_t *adr, size_t size)
{
	uint64_t offset = 0;
	while (size > 0) {
		printf("%08" B_PRIx64 " ", offset);
		while (size > 0) {
			printf(" %02" B_PRIx8, *adr++);
			size--; offset++;
			if (offset % 0x10 == 0) break;
		}
		printf("\n");
	}
}

static void DumpInt32(uint32_t *adr, size_t size)
{
	uint64_t offset = 0;
	while (size > 0) {
		printf("%08" B_PRIx64 " ", offset);
		while (size > 0) {
			printf(" %08" B_PRIx32, *adr++);
			size -= 4; offset += 4;
			if (offset % 0x10 == 0) break;
		}
		printf("\n");
	}
}

static void RadeonWriteRegName(uint32_t reg)
{
	const char *regName = RadeonGetRegName(reg);
	if (regName != NULL) {
		printf(regName);
	} else {
		printf("?(%" B_PRIu32 ")", reg);
	}
}

#define PKT_TYPE_G(x)         (((x) >> 30) & 0x3)
#define PKT_COUNT_G(x)        (((x) >> 16) & 0x3FFF)
#define PKT3_IT_OPCODE_G(x)   (((x) >> 8) & 0xFF)

#define SI_CONFIG_REG_OFFSET       0x00008000
#define SI_CONFIG_REG_END          0x0000B000
#define SI_SH_REG_OFFSET           0x0000B000
#define SI_SH_REG_END              0x0000C000
#define SI_CONTEXT_REG_OFFSET      0x00028000
#define SI_CONTEXT_REG_END         0x00030000

static void DumpIb(uint32_t *adr, size_t size)
{
	uint32_t *end = adr + size/4;
	while (adr != end) {
		uint32_t header = *adr++;
		unsigned type = PKT_TYPE_G(header);
		switch (type) {
			case 3: {
				int count = PKT_COUNT_G(header);
				unsigned op = PKT3_IT_OPCODE_G(header);
				uint32_t *pktEnd = adr + (count + 1);
				printf("PKT3(");
				switch (op) {
					case 0x12: printf("CLEAR_STATE"); break;
					case 0x15: printf("DISPATCH_DIRECT"); break;
					case 0x27: printf("DRAW_INDEX_2"); break;
					case 0x28: printf("CONTEXT_CONTROL"); break;
					case 0x2a: printf("INDEX_TYPE"); break;
					case 0x2d: printf("DRAW_INDEX_AUTO"); break;
					case 0x2f: printf("NUM_INSTANCES"); break;
					case 0x37: printf("WRITE_DATA"); break;
					case 0x40: printf("COPY_DATA"); break;
					case 0x41: printf("CP_DMA"); break;
					case 0x42: printf("PFP_SYNC_ME"); break;
					case 0x43: printf("SURFACE_SYNC"); break;
					case 0x46: printf("EVENT_WRITE"); break;
					case 0x68: printf("SET_CONFIG_REG"); break;
					case 0x69: printf("SET_CONTEXT_REG"); break;
					case 0x76: printf("SET_SH_REG"); break;
					default: printf("?(%#02x)", op);
				}
				printf(", %d)\n", count);
				switch (op) {
					case 0x68: /* SET_CONFIG_REG */ {
						uint32_t reg = (*adr << 2) + SI_CONFIG_REG_OFFSET; adr++;
						// printf("  reg: 0x%08" B_PRIx32 "\n", reg);
						printf("  reg: ");
						const char *regName = RadeonGetRegName(reg);
						if (regName != NULL) {
							printf(regName);
						} else {
							printf("?(%" B_PRIu32 ")", reg);
						}
						printf("\n");
						while (adr != pktEnd) {
							printf("  0x%08" B_PRIx32 "\n", *adr++);
						}
						break;
					}
					case 0x69: /* SET_CONTEXT_REG */ {
						uint32_t reg = (*adr << 2) + SI_CONTEXT_REG_OFFSET; adr++;
						// printf("  reg: 0x%08" B_PRIx32 "\n", reg);
						printf("  reg: "); RadeonWriteRegName(reg); printf("\n");
						while (adr != pktEnd) {
							printf("  0x%08" B_PRIx32 "\n", *adr++);
						}
						break;
					}
					case 0x76: /* SET_SH_REG */ {
						uint32_t reg = (*adr << 2) + SI_SH_REG_OFFSET; adr++;
						// printf("  reg: 0x%08" B_PRIx32 "\n", reg);
						printf("  reg: ");
						const char *regName = RadeonGetRegName(reg);
						if (regName != NULL) {
							printf(regName);
						} else {
							printf("?(%" B_PRIu32 ")", reg);
						}
						printf("\n");
						while (adr != pktEnd) {
							printf("  0x%08" B_PRIx32 "\n", *adr++);
						}
						break;
					}
					case 0x40: /* COPY_DATA */ {
						uint32_t srcKind = *adr % 0x10;
						uint32_t dstKind = (*adr >> 8) % 0x10;
						printf("  src: ");
						switch (srcKind) {
							case 0: printf("REG("); RadeonWriteRegName(*(adr + 3) << 2); printf(")"); break;
							case 1: printf("SRC_MEM(0x%08" B_PRIx64 ")", *((uint64_t*)(adr + 1))); break;
							case 2: printf("TC_L2"); break;
							case 3: printf("GDS"); break;
							case 4: printf("PERF"); break;
							case 5: printf("IMM"); break;
							case 9: printf("TIMESTAMP"); break;
							default: printf("?(%u)", srcKind);
						}
						printf("\n");
						printf("  dst: ");
						switch (dstKind) {
							case 0: printf("REG("); RadeonWriteRegName(*(adr + 3) << 2); printf(")"); break;
							case 1: printf("DST_MEM_GRBM"); break;
							case 2: printf("TC_L2"); break;
							case 3: printf("GDS"); break;
							case 4: printf("PERF"); break;
							case 5: printf("DST_MEM(0x%08" B_PRIx64 ")", *((uint64_t*)(adr + 3))); break;
							default: printf("?(%u)", dstKind);
						}
						printf("\n");
						break;
					}
					default: {
						while (adr != pktEnd) {
							printf("  0x%08" B_PRIx32 "\n", *adr++);
						}
					}
				}
				adr = pktEnd;
				break;
			}
			case 2:
				/* type-2 nop */
				if (header == 0x80000000) {
					printf("PKT2(NOP)\n");
				} else {
					printf("PKT2(?)\n");
					return;
				}
				break;
			default:
				printf("? ( %08" B_PRIx32 ")", header);
				return;
				break;
		}
	}
}

static int HandleAmdgpuIoctl(int fd, unsigned long request, void *arg)
{
	unsigned long requestCmd = request%0x100 - DRM_COMMAND_BASE;

	switch (requestCmd) {
		case DRM_AMDGPU_INFO: {
			struct drm_amdgpu_info *request = (struct drm_amdgpu_info*)arg;
			switch (request->query) {
				case AMDGPU_INFO_ACCEL_WORKING: {
					*(uint32_t*)request->return_pointer = 1;
					return 0;
				}
				case AMDGPU_INFO_DEV_INFO: {
					struct drm_amdgpu_info_device *dev_info = (struct drm_amdgpu_info_device*)request->return_pointer;
					memset(dev_info, 0, sizeof(struct drm_amdgpu_info_device));
					dev_info->device_id = 0x683f;
					dev_info->chip_rev = 0x1;
					dev_info->external_rev = 0x29 /* CAPEVERDE */;
					dev_info->pci_rev = 0x87;
					dev_info->family = AMDGPU_FAMILY_SI;
					dev_info->num_shader_engines = 1;
					dev_info->num_shader_arrays_per_engine = 2;
					dev_info->gpu_counter_freq = 0x6978;
					dev_info->max_engine_clock = 0xc3500;
					dev_info->max_memory_clock = 0x112a88;
					dev_info->cu_active_number = 0x8;
					dev_info->cu_ao_mask = 0x1e1e;
					dev_info->cu_bitmap[0][0] = 0x1e;
					dev_info->cu_bitmap[0][1] = 0x1e;
					dev_info->enabled_rb_pipes_mask = 0xf;
					dev_info->num_rb_pipes = 0x4;
					dev_info->num_hw_gfx_contexts = 0x8;
					dev_info->virtual_address_offset = 0x200000;
					dev_info->virtual_address_max = 0xfffe00000;
					dev_info->virtual_address_alignment = 0x1000;
					dev_info->pte_fragment_size = 0x200000;
					dev_info->gart_page_size = 0x1000;
					dev_info->ce_ram_size = 0x8000;
					dev_info->vram_type = 0x5;
					dev_info->vram_bit_width = 0x80;
					dev_info->num_shader_visible_vgprs = 0x100;
					dev_info->num_cu_per_sh = 0x5;
					dev_info->num_tcc_blocks = 0x4;
					dev_info->max_gs_waves_per_vgt = 0x20;
					dev_info->cu_ao_bitmap[0][0] = 0x1e;
					dev_info->cu_ao_bitmap[0][1] = 0x1e;

					return 0;
				}
				case AMDGPU_INFO_READ_MMR_REG: {
					uint32_t *values = (uint32_t*)request->return_pointer;
					for (uint32_t i = 0; i < request->read_mmr_reg.count; i++) {
						int res = drmAmdgpuReadMmrReg(request->read_mmr_reg.dword_offset + i, &values[i]);
						if (res != 0)
							return res;
					}
					return 0;
				}
				case AMDGPU_INFO_FW_TA: {
					return 0;
				}
				case AMDGPU_INFO_VRAM_GTT: {
					return 0;
				}
				case AMDGPU_INFO_VRAM_USAGE: {
					return 0;
				}
				case AMDGPU_INFO_VIS_VRAM_USAGE: {
					return 0;
				}
				case AMDGPU_INFO_GTT_USAGE: {
					return 0;
				}
				case AMDGPU_INFO_MEMORY: {
					struct drm_amdgpu_memory_info *meminfo = (struct drm_amdgpu_memory_info*)request->return_pointer;
					meminfo->vram.total_heap_size = 0x80000000;
					meminfo->vram.usable_heap_size = 0x7e380000;
					meminfo->vram.heap_usage = 0x2664000;
					meminfo->vram.max_allocation = 0x5eaa0000;

					meminfo->cpu_accessible_vram.total_heap_size = 0x10000000;
					meminfo->cpu_accessible_vram.usable_heap_size = 0xf378000;
					meminfo->cpu_accessible_vram.heap_usage = 0x1290000;
					meminfo->cpu_accessible_vram.max_allocation = 0xb69a000;

					meminfo->gtt.total_heap_size = 0xc0000000;
					meminfo->gtt.usable_heap_size = 0xbfddd000;
					meminfo->gtt.heap_usage = 0xce1000;
					meminfo->gtt.max_allocation = 0x8fe65c00;
					return 0;
				}
				case AMDGPU_INFO_FW_VERSION: {
					struct drm_amdgpu_info_firmware *firmware = (struct drm_amdgpu_info_firmware*)request->return_pointer;
					switch (request->query_fw.fw_type) {
						case AMDGPU_INFO_FW_GFX_ME: {
							firmware->ver = 0x91;
							firmware->feature = 0x1d;
							return 0;
						}
						case AMDGPU_INFO_FW_GFX_MEC: {
							// !!!
							firmware->ver = 0;
							firmware->feature = 0;
							return 0;
						}
						case AMDGPU_INFO_FW_GFX_PFP: {
							firmware->ver = 0x54;
							firmware->feature = 0x1d;
							return 0;
						}
						case AMDGPU_INFO_FW_GFX_CE: {
							firmware->ver = 0x3d;
							firmware->feature = 0x1d;
							return 0;
						}
						case AMDGPU_INFO_FW_UVD: {
							firmware->ver = 0x40000d00;
							firmware->feature = 0;
							return 0;
						}
						case AMDGPU_INFO_FW_VCE: {
							firmware->ver = 0;
							firmware->feature = 0;
							return 0;
						}
					}
					return -1;
				}
				case AMDGPU_INFO_HW_IP_INFO: {
					struct drm_amdgpu_info_hw_ip *info = (struct drm_amdgpu_info_hw_ip*)request->return_pointer;
					switch (request->query_hw_ip.type) {
						case AMDGPU_HW_IP_GFX: {
							info->hw_ip_version_major = 0x6;
							info->hw_ip_version_minor = 0;
							info->capabilities_flags = 0;
							info->ib_start_alignment = 0x20;
							info->ib_size_alignment = 0x20;
							info->available_rings = 1;
							return 0;
						}
						case AMDGPU_HW_IP_COMPUTE: {
							info->hw_ip_version_major = 0x6;
							info->hw_ip_version_minor = 0;
							info->capabilities_flags = 0;
							info->ib_start_alignment = 0x20;
							info->ib_size_alignment = 0x20;
							info->available_rings = 3;
							return 0;
						}
						case AMDGPU_HW_IP_DMA: {
							info->hw_ip_version_major = 0x1;
							info->hw_ip_version_minor = 0;
							info->capabilities_flags = 0;
							info->ib_start_alignment = 0x100;
							info->ib_size_alignment = 0x4;
							info->available_rings = 3;
							return 0;
						}
						case AMDGPU_HW_IP_UVD: {
							info->hw_ip_version_major = 0x3;
							info->hw_ip_version_minor = 0x1;
							info->capabilities_flags = 0;
							info->ib_start_alignment = 0x40;
							info->ib_size_alignment = 0x40;
							info->available_rings = 1;
							return 0;
						}
						case AMDGPU_HW_IP_VCE: {
							return 0;
						}
						case AMDGPU_HW_IP_UVD_ENC: {
							info->hw_ip_version_major = 0x3;
							info->hw_ip_version_minor = 0x1;
							info->capabilities_flags = 0;
							info->ib_start_alignment = 0x40;
							info->ib_size_alignment = 0x40;
							info->available_rings = 0;
							return 0;
						}
						case AMDGPU_HW_IP_VCN_DEC: {
							return 0;
						}
						case AMDGPU_HW_IP_VCN_ENC: {
							return 0;
						}
						case AMDGPU_HW_IP_VCN_JPEG: {
							return 0;
						}
					}
					return -1;
				}
				default:
					return -1;
			}
			return 0;
		}
		case DRM_AMDGPU_CTX: {
			union drm_amdgpu_ctx *request = (union drm_amdgpu_ctx*)arg;
			switch (request->in.op) {
				case AMDGPU_CTX_OP_ALLOC_CTX: {
					static uint32_t newCtx = 1;
					request->out.alloc.ctx_id = newCtx++;
					return 0;
				}
				case AMDGPU_CTX_OP_FREE_CTX: {
					return 0;
				}
				case AMDGPU_CTX_OP_QUERY_STATE: {
					return 0;
				}
				case AMDGPU_CTX_OP_QUERY_STATE2: {
					return 0;
				}
				default:
					return -1;
			}
			return 0;
		}
		case DRM_AMDGPU_GEM_CREATE: {
			union drm_amdgpu_gem_create *args = (union drm_amdgpu_gem_create*)arg;
			return CreateGpuBuffer(args->in.bo_size, args->in.alignment, args->in.domains, args->in.domain_flags, args->out.handle);
		}
		case DRM_AMDGPU_GEM_VA: {
			struct drm_amdgpu_gem_va *args = (struct drm_amdgpu_gem_va*)arg;
			switch (args->operation) {
				case AMDGPU_VA_OP_MAP: {
					return MapGpuBuffer(args->handle, args->flags, args->va_address, args->offset_in_bo, args->map_size);
				}
				case AMDGPU_VA_OP_UNMAP: {
					return UnmapGpuBuffer(args->handle, args->flags, args->va_address, args->offset_in_bo, args->map_size);
				}
/*
				case AMDGPU_VA_OP_CLEAR: {
					return 0;
				}
				case AMDGPU_VA_OP_REPLACE: {
					return 0;
				}
*/
				default:
					return -1;
			}
			return 0;
		}
		case DRM_AMDGPU_GEM_MMAP: {
			union drm_amdgpu_gem_mmap *args = (union drm_amdgpu_gem_mmap*)arg;
			args->out.addr_ptr = args->in.handle;
			return 0;
		}
		case DRM_AMDGPU_CS: {
			auto args = (union drm_amdgpu_cs*)arg;
			auto chunks = (struct drm_amdgpu_cs_chunk**)args->in.chunks;
			for (size_t i = 0; i < args->in.num_chunks; i++) {
				switch(chunks[i]->chunk_id) {
					case AMDGPU_CHUNK_ID_IB: {
						auto ib = (struct drm_amdgpu_cs_chunk_ib*)chunks[i]->chunk_data;
#if 0
						printf("\n");
						//DumpMappings();
						//printf("IB, va: va_start: %#" PRIx64 ", ib_bytes: %u\n", ib->va_start, ib->ib_bytes);
						uint64_t offset;
						BReference<GpuBuffer> buffer = ThisMappedGpuBuffer(ib->va_start, offset);
						//printf("buffer: %#" PRIx64 "\n", buffer.IsSet() ? buffer->adr : 0);
						/*DumpInt32*/DumpIb((uint32_t*)((uint8_t*)buffer->adr + offset), ib->ib_bytes);
						//printf("[WAIT]"); fgetc(stdin);
#endif
						break;
					}
					case AMDGPU_CHUNK_ID_FENCE: {
						auto fence = (struct drm_amdgpu_cs_chunk_fence*)chunks[i]->chunk_data;
						break;
					}
					case AMDGPU_CHUNK_ID_DEPENDENCIES: {
						break;
					}
					case AMDGPU_CHUNK_ID_BO_HANDLES: {
						auto boList = (struct drm_amdgpu_bo_list_in*)chunks[i]->chunk_data;
						break;
					}
				}
			}
			static uint64 handle = 1;
			args->out.handle = handle++;
			return 0;
		}
		case DRM_AMDGPU_WAIT_CS: {
			union drm_amdgpu_wait_cs *args = (union drm_amdgpu_wait_cs*)arg;
			// printf("[WAIT]"); fgetc(stdin);
			args->out.status = 0;
			return 0;
		}
	}
	printf("HandleAmdgpuIoctl(): not implemented\n");
	printf("[WAIT]"); fgetc(stdin);
	return -1;
}

int drmIoctlStub(int fd, unsigned long request, void *arg)
{
	unsigned long requestCmd = request%0x100;

	if (requestCmd >= DRM_COMMAND_BASE && requestCmd < DRM_COMMAND_END)
		return HandleAmdgpuIoctl(fd, request, arg);
	
	switch (request) {
		case DRM_IOCTL_VERSION: {
			const char *name = "amdgpu";
			const char *date = "20150101";
			const char *desc = "AMD GPU";
			drm_version_t *version = (drm_version_t*)arg;
			version->version_major = 3;
			version->version_minor = 42;
			version->version_patchlevel = 0;
			version->name_len = strlen(name);
			if (version->name != NULL) strcpy(version->name, name);
			version->date_len = strlen(date);
			if (version->date != NULL) strcpy(version->date, date);
			version->desc_len = strlen(desc);
			if (version->desc != NULL) strcpy(version->desc, desc);
			return 0;
		}
		case DRM_IOCTL_GEM_CLOSE: {
			auto args = (struct drm_gem_close*)arg;
			return CloseGpuBuffer(args->handle);
		}
		case DRM_IOCTL_GET_CAP: {
			auto args = (struct drm_get_cap*)arg;
			switch (args->capability) {
				case DRM_CAP_SYNCOBJ: args->value = 1; return 0;
				case DRM_CAP_SYNCOBJ_TIMELINE: args->value = 1; return 0;
				default:;
			}
		}
		case DRM_IOCTL_PRIME_HANDLE_TO_FD: {
			auto args = (struct drm_prime_handle*)arg;
			return drmFdMapGemHandle(fd, &args->fd, args->handle);
		}
		case DRM_IOCTL_PRIME_FD_TO_HANDLE: {
			auto args = (struct drm_prime_handle*)arg;
			return drmFdUnmapGemHandle(fd, &args->handle, args->fd);
		}
		case DRM_IOCTL_SYNCOBJ_CREATE: {
			auto args = (struct drm_syncobj_create*)arg;
			static uint32_t handle = 1;
			args->handle = handle++;
			return 0;
		}
		case DRM_IOCTL_SYNCOBJ_DESTROY: {
			auto args = (struct drm_syncobj_destroy*)arg;
			return 0;
		}
		case DRM_IOCTL_SYNCOBJ_WAIT: {
			auto args = (struct drm_syncobj_wait*)arg;
			return 0;
		}
		case DRM_IOCTL_SYNCOBJ_TIMELINE_WAIT: {
			auto args = (struct drm_syncobj_timeline_wait*)arg;
			args->first_signaled = 0;
			return 0;
		}
		case DRM_IOCTL_SYNCOBJ_TRANSFER: {
			auto args = (struct drm_syncobj_transfer*)arg;
			return 0;
		}
		case DRM_IOCTL_SYNCOBJ_RESET: {
			auto args = (struct drm_syncobj_array*)arg;
			return 0;
		}
		case DRM_IOCTL_SYNCOBJ_SIGNAL: {
			auto args = (struct drm_syncobj_array*)arg;
			return 0;
		}
		case DRM_IOCTL_SYNCOBJ_HANDLE_TO_FD: {
			auto args = (struct drm_syncobj_handle*)arg;
			return drmFdMapSyncobjHandle(fd, &args->fd, args->handle);
		}
		case DRM_IOCTL_SYNCOBJ_FD_TO_HANDLE: {
			auto args = (struct drm_syncobj_handle*)arg;
			return drmFdUnmapSyncobjHandle(fd, &args->handle, args->fd);
		}
		case DRM_IOCTL_SYNCOBJ_ACCUMULATE: {
			auto args = (struct drm_syncobj_accumulate*)arg;
			return 0;
		}
		default:;
	}
	printf("HandleAmdgpuIoctl(): not implemented\n");
	printf("[WAIT]"); fgetc(stdin);

	return -1;
}

/*
	libdrm_amdgpu.so
U drmCommandWrite
U drmCommandWriteRead
U drmFreeVersion
U drmGetNodeTypeFromFd
U drmGetPrimaryDeviceNameFromFd
U drmGetVersion
U drmIoctl
U drmMsg
U drmPrimeFDToHandle
U drmPrimeHandleToFD
U drmSyncobjCreate
U drmSyncobjDestroy
U drmSyncobjExportSyncFile
U drmSyncobjFDToHandle
U drmSyncobjHandleToFD
U drmSyncobjImportSyncFile
U drmSyncobjQuery
U drmSyncobjQuery2
U drmSyncobjReset
U drmSyncobjSignal
U drmSyncobjTimelineSignal
U drmSyncobjTimelineWait
U drmSyncobjTransfer
U drmSyncobjWait

	libvulkan_radeon.so
U drmCommandWrite
U drmFreeDevice
U drmFreeDevices
U drmFreeVersion
U drmGetCap
U drmGetDevice2
U drmGetDevices2
U drmGetVersion
*/
