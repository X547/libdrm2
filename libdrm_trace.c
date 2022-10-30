#include "libdrm_trace.h"

#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>

#include <xf86drm.h>
#include "libdrm_macros.h"
#include <libdrm/amdgpu_drm.h>

/*
AMDGPU_INFO_MEMORY
AMDGPU_INFO_VIDEO_CAPS
*/

static void drmAmdgpuWriteHeapInfo(struct drm_amdgpu_heap_info *info)
{
	printf("    total_heap_size: %#" PRIx64 "\n", info->total_heap_size);
	printf("    usable_heap_size: %#" PRIx64 "\n", info->usable_heap_size);
	printf("    heap_usage: %#" PRIx64 "\n", info->heap_usage);
	printf("    max_allocation: %#" PRIx64 "\n", info->max_allocation);
}

static void drmAmdgpuWriteCmd(unsigned long cmd, void *arg, bool isPost)
{
	if (isPost) {
		switch (cmd) {
			case DRM_AMDGPU_GEM_CREATE: {
				union drm_amdgpu_gem_create *args = arg;
				printf(" -> %d", args->out.handle);
				break;
			}
			case DRM_AMDGPU_GEM_MMAP: {
				union drm_amdgpu_gem_mmap *args = arg;
				printf(" -> %#lx", args->out.addr_ptr);
				break;
			}
			case DRM_AMDGPU_INFO: {
				struct drm_amdgpu_info *args = arg;
				switch (args->query) {
					case AMDGPU_INFO_ACCEL_WORKING: printf(" -> %d", *((uint32_t*)args->return_pointer)); break;
					case AMDGPU_INFO_DEV_INFO: { printf(" -> (\n");
						struct drm_amdgpu_info_device *infoDev = (struct drm_amdgpu_info_device*)args->return_pointer;
						printf("  device_id: %#x\n", infoDev->device_id);
						printf("  chip_rev: %#x\n", infoDev->chip_rev);
						printf("  external_rev: %#x\n", infoDev->external_rev);
						printf("  pci_rev: %#x\n", infoDev->pci_rev);
						printf("  family: %#x\n", infoDev->family);
						printf("  num_shader_engines: %#x\n", infoDev->num_shader_engines);
						printf("  num_shader_arrays_per_engine: %#x\n", infoDev->num_shader_arrays_per_engine);
						printf("  gpu_counter_freq: %#x\n", infoDev->gpu_counter_freq);
						printf("  max_engine_clock: %#" PRIx64 "\n", infoDev->max_engine_clock);
						printf("  max_memory_clock: %#" PRIx64 "\n", infoDev->max_memory_clock);
						printf("  cu_active_number: %#x\n", infoDev->cu_active_number);
						printf("  cu_ao_mask: %#x\n", infoDev->cu_ao_mask);
						printf("  cu_bitmap: (");
						for (int i = 0; i < 4; i++) {
							if (i > 0) printf(", ");
							printf("(");
							for (int j = 0; j < 4; j++) {
								if (j > 0) printf(", ");
								printf("%#x", infoDev->cu_bitmap[i][j]);
							}
							printf(")");
						}
						printf(")\n");
						printf("  enabled_rb_pipes_mask: %#x\n", infoDev->enabled_rb_pipes_mask);
						printf("  num_rb_pipes: %#x\n", infoDev->num_rb_pipes);
						printf("  num_hw_gfx_contexts: %#x\n", infoDev->num_hw_gfx_contexts);
						printf("  ids_flags: %#" PRIx64 "\n", infoDev->ids_flags);
						printf("  virtual_address_offset: %#" PRIx64 "\n", infoDev->virtual_address_offset);
						printf("  virtual_address_max: %#" PRIx64 "\n", infoDev->virtual_address_max);
						printf("  virtual_address_alignment: %#x\n", infoDev->virtual_address_alignment);
						printf("  pte_fragment_size: %#x\n", infoDev->pte_fragment_size);
						printf("  gart_page_size: %#x\n", infoDev->gart_page_size);
						printf("  ce_ram_size: %#x\n", infoDev->ce_ram_size);
						printf("  vram_type: %#x\n", infoDev->vram_type);
						printf("  vram_bit_width: %#x\n", infoDev->vram_bit_width);
						printf("  vce_harvest_config: %#x\n", infoDev->vce_harvest_config);
						printf("  gc_double_offchip_lds_buf: %#x\n", infoDev->gc_double_offchip_lds_buf);
						printf("  prim_buf_gpu_addr: %#" PRIx64 "\n", infoDev->prim_buf_gpu_addr);
						printf("  pos_buf_gpu_addr: %#" PRIx64 "\n", infoDev->pos_buf_gpu_addr);
						printf("  cntl_sb_buf_gpu_addr: %#" PRIx64 "\n", infoDev->cntl_sb_buf_gpu_addr);
						printf("  param_buf_gpu_addr: %#" PRIx64 "\n", infoDev->param_buf_gpu_addr);
						printf("  prim_buf_size: %#x\n", infoDev->prim_buf_size);
						printf("  pos_buf_size: %#x\n", infoDev->pos_buf_size);
						printf("  cntl_sb_buf_size: %#x\n", infoDev->cntl_sb_buf_size);
						printf("  param_buf_size: %#x\n", infoDev->param_buf_size);
						printf("  wave_front_size: %#x\n", infoDev->wave_front_size);
						printf("  num_shader_visible_vgprs: %#x\n", infoDev->num_shader_visible_vgprs);
						printf("  num_cu_per_sh: %#x\n", infoDev->num_cu_per_sh);
						printf("  num_tcc_blocks: %#x\n", infoDev->num_tcc_blocks);
						printf("  gs_vgt_table_depth: %#x\n", infoDev->gs_vgt_table_depth);
						printf("  gs_prim_buffer_depth: %#x\n", infoDev->gs_prim_buffer_depth);
						printf("  max_gs_waves_per_vgt: %#x\n", infoDev->max_gs_waves_per_vgt);
						printf("  cu_ao_bitmap: (");
						for (int i = 0; i < 4; i++) {
							if (i > 0) printf(", ");
							printf("(");
							for (int j = 0; j < 4; j++) {
								if (j > 0) printf(", ");
								printf("%#x", infoDev->cu_ao_bitmap[i][j]);
							}
							printf(")");
						}
						printf(")\n");
						printf("  high_va_offset: %#" PRIx64 "\n", infoDev->high_va_offset);
						printf("  high_va_max: %#" PRIx64 "\n", infoDev->high_va_max);
						printf("  pa_sc_tile_steering_override: %#x\n", infoDev->pa_sc_tile_steering_override);
						printf("  tcc_disabled_mask: %#" PRIx64 "\n", infoDev->tcc_disabled_mask);
						printf(")");
						break;
					}
					case AMDGPU_INFO_HW_IP_INFO: {
						struct drm_amdgpu_info_hw_ip *info = (void*)args->return_pointer;
						printf(" -> (\n");
						printf("  hw_ip_version_major: %#" PRIx32 "\n", info->hw_ip_version_major);
						printf("  hw_ip_version_minor: %#" PRIx32 "\n", info->hw_ip_version_minor);
						printf("  capabilities_flags: %#" PRIx64 "\n", info->capabilities_flags);
						printf("  ib_start_alignment: %#" PRIx32 "\n", info->ib_start_alignment);
						printf("  ib_size_alignment: %#" PRIx32 "\n", info->ib_size_alignment);
						printf("  available_rings: %" PRIu32 "\n", info->available_rings);
						printf(")");
						break;
					}
					case AMDGPU_INFO_FW_VERSION: {
						struct drm_amdgpu_info_firmware *firmware = (void*)args->return_pointer;
						printf(" -> (");
						printf("ver: %#" PRIx32, firmware->ver);
						printf(", feature: %#" PRIx32, firmware->feature);
						printf(")");
						break;
					}
					case AMDGPU_INFO_GDS_CONFIG: {
						struct drm_amdgpu_info_gds *gds_config = (void*)args->return_pointer;
						printf(" -> (\n");
						printf("  gds_gfx_partition_size: %#" PRIx32 "\n", gds_config->gds_gfx_partition_size);
						printf("  compute_partition_size: %#" PRIx32"\n", gds_config->compute_partition_size);
						printf("  gds_total_size: %#" PRIx32"\n", gds_config->gds_total_size);
						printf("  gws_per_gfx_partition: %#" PRIx32"\n", gds_config->gws_per_gfx_partition);
						printf("  gws_per_compute_partition: %#" PRIx32"\n", gds_config->gws_per_compute_partition);
						printf("  oa_per_gfx_partition: %#" PRIx32"\n", gds_config->oa_per_gfx_partition);
						printf("  oa_per_compute_partition: %#" PRIx32"\n", gds_config->oa_per_compute_partition);
						printf(")");
						break;
					}
					case AMDGPU_INFO_VRAM_GTT: {
						struct drm_amdgpu_info_vram_gtt *vram_gtt_info = (void*)args->return_pointer;
						printf(" -> (\n");
						printf("  vram_size: %#" PRIx64 "\n", vram_gtt_info->vram_size);
						printf("  vram_cpu_accessible_size: %#" PRIx64 "\n", vram_gtt_info->vram_cpu_accessible_size);
						printf("  gtt_size: %#" PRIx64 "\n", vram_gtt_info->gtt_size);
						printf(")");
						break;
					}
					case AMDGPU_INFO_READ_MMR_REG: {
						uint32_t *values = (void*)args->return_pointer;
						printf(" -> (");
						for (uint32_t i = 0; i < args->read_mmr_reg.count; i++) {
							if (i > 0) printf(", ");
							printf("%#" PRIx32, values[i]);
						}
						printf(")");
						break;
					}
					case AMDGPU_INFO_CRTC_FROM_ID:
					case AMDGPU_INFO_HW_IP_COUNT:
					case AMDGPU_INFO_TIMESTAMP:
					case AMDGPU_INFO_NUM_BYTES_MOVED:
					case AMDGPU_INFO_VRAM_USAGE:
					case AMDGPU_INFO_GTT_USAGE:
					case AMDGPU_INFO_VIS_VRAM_USAGE:
					case AMDGPU_INFO_NUM_EVICTIONS:
					case AMDGPU_INFO_MEMORY: {
						struct drm_amdgpu_memory_info *meminfo = (void*)args->return_pointer;
						printf(" -> (\n");
						printf("  vram: (\n");
						drmAmdgpuWriteHeapInfo(&meminfo->vram);
						printf("  )\n  cpu_accessible_vram: (\n");
						drmAmdgpuWriteHeapInfo(&meminfo->cpu_accessible_vram);
						printf("  )\n  gtt: (\n");
						drmAmdgpuWriteHeapInfo(&meminfo->gtt);
						printf("  )\n");
						printf(")");
						break;
					}
					case AMDGPU_INFO_VCE_CLOCK_TABLE:
					case AMDGPU_INFO_VBIOS:
					case AMDGPU_INFO_NUM_HANDLES:
					case AMDGPU_INFO_SENSOR:
					case AMDGPU_INFO_NUM_VRAM_CPU_PAGE_FAULTS:
					case AMDGPU_INFO_VRAM_LOST_COUNTER:
					case AMDGPU_INFO_RAS_ENABLED_FEATURES:
					case 0x21 /* AMDGPU_INFO_VIDEO_CAPS */: {
						printf(" -> ?");
						break;
					}
				}
				break;
			}
			case DRM_AMDGPU_CTX: {
				union drm_amdgpu_ctx *args = arg;
				printf(" -> %" PRIu32, args->out.alloc.ctx_id);
				break;
			case DRM_AMDGPU_CS: {
				union drm_amdgpu_cs *args = arg;
				printf(" -> %" PRIu64, args->out.handle);
				break;
			}
			case DRM_AMDGPU_WAIT_CS: {
				union drm_amdgpu_wait_cs *args = arg;
				printf(" -> %" PRIu64, args->out.status);
			}
			case DRM_AMDGPU_GEM_USERPTR: {
				struct drm_amdgpu_gem_userptr *args = arg;
				printf(" -> %" PRIu32, args->handle);
			}
			}
		}
		return;
	}

	switch (cmd) {
		case 0x00: printf("AMDGPU_GEM_CREATE"); {
			union drm_amdgpu_gem_create *args = arg;
			printf("(bo_size: %#" PRIx64 ", alignment: %#" PRIx64 ", domains: ", args->in.bo_size, args->in.alignment);
			printf("{");
			bool first = true;
			for (uint32_t i = 0; i < 32; i++) {
				if (((1 << i) & args->in.domains) != 0) {
					if (first) first = false; else printf(", ");
					switch (i) {
						case 0: printf("CPU"); break;
						case 1: printf("GTT"); break;
						case 2: printf("VRAM"); break;
						case 3: printf("GDS"); break;
						case 4: printf("GWS"); break;
						case 5: printf("OA"); break;
						default: printf("?(%u)", i);
					}
				}
			}
			printf("}");
			printf(", domain_flags: ");
			printf("{");
			first = true;
			for (uint32_t i = 0; i < 32; i++) {
				if (((1 << i) & args->in.domain_flags) != 0) {
					if (first) first = false; else printf(", ");
					switch (i) {
						case 0: printf("CPU_ACCESS_REQUIRED"); break;
						case 1: printf("NO_CPU_ACCESS"); break;
						case 2: printf("CPU_GTT_USWC"); break;
						case 3: printf("VRAM_CLEARED"); break;
						case 5: printf("VRAM_CONTIGUOUS"); break;
						case 6: printf("VM_ALWAYS_VALID"); break;
						case 7: printf("EXPLICIT_SYNC"); break;
						case 8: printf("CP_MQD_GFX9"); break;
						case 9: printf("VRAM_WIPE_ON_RELEASE"); break;
						case 10: printf("CREATE_ENCRYPTED"); break;
						case 11: printf("CREATE_PREEMPTIBLE"); break;
						default: printf("?(%u)", i);
					}
				}
			}
			printf("}");
			printf(")");
			break;
		}
		case 0x01: printf("AMDGPU_GEM_MMAP"); {
			union drm_amdgpu_gem_mmap *args = arg;
			printf("(%d)", args->in.handle);
			break;
		}
		case 0x02: printf("AMDGPU_CTX"); {
			union drm_amdgpu_ctx *args = arg;
			printf("(op: ");
			switch (args->in.op) {
				case AMDGPU_CTX_OP_ALLOC_CTX: printf("ALLOC"); break;
				case AMDGPU_CTX_OP_FREE_CTX: printf("FREE"); break;
				case AMDGPU_CTX_OP_QUERY_STATE: printf("QUERY_STATE"); break;
				case AMDGPU_CTX_OP_QUERY_STATE2: printf("QUERY_STATE2"); break;
			}
			printf(", flags: %#" PRIx32, args->in.flags);
			printf(", ctx_id: %" PRIu32, args->in.ctx_id);
			printf(", priority: %" PRId32 ")", args->in.priority);
			break;
		}
		case 0x03: printf("AMDGPU_BO_LIST"); break;
		case 0x04: printf("AMDGPU_CS"); {
			union drm_amdgpu_cs *args = arg;
			printf("(ctx_id: %" PRIu32, args->in.ctx_id);
			printf(", bo_list_handle: %" PRIu32, args->in.bo_list_handle);
			printf(", num_chunks: %" PRIu32, args->in.num_chunks);
			printf(", chunks: (\n");
			struct drm_amdgpu_cs_chunk **chunks = (struct drm_amdgpu_cs_chunk**)args->in.chunks;
			for (size_t i = 0; i < args->in.num_chunks; i++) {
				printf("  (id: ");
				switch(chunks[i]->chunk_id) {
					case 0x01: printf("IB"); {
						struct drm_amdgpu_cs_chunk_ib *ib = (struct drm_amdgpu_cs_chunk_ib*)chunks[i]->chunk_data;
						printf("(flags: %#" PRIx32 ", va_start: %#" PRIx64 ", ib_bytes: %u, ip_type: %u, ip_instance: %u, ring: %u)", ib->flags, ib->va_start, ib->ib_bytes, ib->ip_type, ib->ip_instance, ib->ring);
						break;
					}
					case 0x02: printf("FENCE"); {
						struct drm_amdgpu_cs_chunk_fence *fence = (struct drm_amdgpu_cs_chunk_fence*)chunks[i]->chunk_data;
						printf("(handle: %u, offset: %u)", fence->handle, fence->offset);
						break;
					}
					case 0x03: printf("DEPENDENCIES"); {
						struct drm_amdgpu_cs_chunk_dep *depList = (struct drm_amdgpu_cs_chunk_dep*)chunks[i]->chunk_data;
						printf("(");
						for (size_t j = 0; j < chunks[i]->length_dw; j += sizeof(struct drm_amdgpu_cs_chunk_dep) / 4) {
							if (j > 0) printf(", ");
							printf("(ip_type: %" PRIu32, depList->ip_type);
							printf(", ip_instance: %" PRIu32, depList->ip_instance);
							printf(", ring: %" PRIu32, depList->ring);
							printf(", ctx_id: %" PRIu32, depList->ctx_id);
							printf(", handle: %" PRIu64 ")", depList->handle);
							depList++;
						}
						printf(")");
						break;
					}
					case 0x04: printf("SYNCOBJ_IN"); break;
					case 0x05: printf("SYNCOBJ_OUT"); break;
					case 0x06: printf("BO_HANDLES"); {
						struct drm_amdgpu_bo_list_in *boList = (struct drm_amdgpu_bo_list_in*)chunks[i]->chunk_data;
						printf("(operation: %d, list_handle: %d, bo_number: %u, bo_info_size: %u, bo_info_ptr: %#lx(", boList->operation, boList->list_handle, boList->bo_number, boList->bo_info_size, boList->bo_info_ptr);
						for (size_t j = 0; j < boList->bo_number; j++) {
							if (j > 0) printf(", ");
							printf("%d", *(int32_t*)((char*)boList->bo_info_ptr + j*boList->bo_info_size));
						}
						printf("))");
						break;
					}
					case 0x07: printf("SCHEDULED_DEPENDENCIES"); break;
					case 0x08: printf("SYNCOBJ_TIMELINE_WAIT"); {
						struct drm_amdgpu_cs_chunk_syncobj *syncobj = (struct drm_amdgpu_cs_chunk_syncobj*)chunks[i]->chunk_data;
						printf("(");
						for (size_t j = 0; j < chunks[i]->length_dw; j += sizeof(struct drm_amdgpu_cs_chunk_syncobj) / 4) {
							if (j > 0) printf(", ");
							printf("(handle: %u, flags: %#x, point: %#lx)", syncobj->handle, syncobj->flags, syncobj->point);
							syncobj++;
						}
						printf(")");
						break;
					}
					case 0x09: printf("SYNCOBJ_TIMELINE_SIGNAL"); {
						struct drm_amdgpu_cs_chunk_syncobj *syncobj = (struct drm_amdgpu_cs_chunk_syncobj*)chunks[i]->chunk_data;
						printf("(");
						for (size_t j = 0; j < chunks[i]->length_dw; j += sizeof(struct drm_amdgpu_cs_chunk_syncobj) / 4) {
							if (j > 0) printf(", ");
							printf("(handle: %u, flags: %#x, point: %#lx)", syncobj->handle, syncobj->flags, syncobj->point);
							syncobj++;
						}
						printf(")");
						break;
					}
					default: printf("?(%u)", chunks[i]->chunk_id);
				}
				printf(", length: %u, data: %#" PRIx64 ")\n", chunks[i]->length_dw, chunks[i]->chunk_data);
			}
			printf(")");
			break;
		}
		case 0x05: printf("AMDGPU_INFO"); {
			struct drm_amdgpu_info *args = arg;
			printf("(query: ");
			switch (args->query) {
				case 0x00: printf("AMDGPU_INFO_ACCEL_WORKING"); break;
				case 0x01: printf("AMDGPU_INFO_CRTC_FROM_ID"); break;
				case 0x02: printf("AMDGPU_INFO_HW_IP_INFO"); {
					printf("(type: ");
					switch (args->query_hw_ip.type) {
						case AMDGPU_HW_IP_GFX: printf("GFX"); break;
						case AMDGPU_HW_IP_COMPUTE: printf("COMPUTE"); break;
						case AMDGPU_HW_IP_DMA: printf("DMA"); break;
						case AMDGPU_HW_IP_UVD: printf("UVD"); break;
						case AMDGPU_HW_IP_VCE: printf("VCE"); break;
						case AMDGPU_HW_IP_UVD_ENC: printf("UVD_ENC"); break;
						case AMDGPU_HW_IP_VCN_DEC: printf("VCN_DEC"); break;
						case AMDGPU_HW_IP_VCN_ENC: printf("VCN_ENC"); break;
						case AMDGPU_HW_IP_VCN_JPEG: printf("VCN_JPEG"); break;
						case AMDGPU_HW_IP_NUM: printf("NUM"); break;
						default: printf("?(%#" PRIx32 ")", args->query_hw_ip.type);
					}
					printf(", ip_instance: %"  PRIu32 ")", args->query_hw_ip.ip_instance);
					break;
				}
				case 0x03: printf("AMDGPU_INFO_HW_IP_COUNT"); break;
				case 0x05: printf("AMDGPU_INFO_TIMESTAMP"); break;
				case 0x0e: printf("AMDGPU_INFO_FW_VERSION"); {
					printf("(type: ");
					switch (args->query_fw.fw_type) {
						case AMDGPU_INFO_FW_VCE: printf("VCE"); break;
						case AMDGPU_INFO_FW_UVD: printf("UVD"); break;
						case AMDGPU_INFO_FW_GMC: printf("GMC"); break;
						case AMDGPU_INFO_FW_GFX_ME: printf("GFX_ME"); break;
						case AMDGPU_INFO_FW_GFX_PFP: printf("GFX_PFP"); break;
						case AMDGPU_INFO_FW_GFX_CE: printf("GFX_CE"); break;
						case AMDGPU_INFO_FW_GFX_RLC: printf("GFX_RLC"); break;
						case AMDGPU_INFO_FW_GFX_MEC: printf("GFX_MEC"); break;
						case AMDGPU_INFO_FW_SMC: printf("SMC"); break;
						case AMDGPU_INFO_FW_SDMA: printf("SDMA"); break;
						case AMDGPU_INFO_FW_SOS: printf("SOS"); break;
						case AMDGPU_INFO_FW_ASD: printf("ASD"); break;
						case AMDGPU_INFO_FW_VCN: printf("VCN"); break;
						case AMDGPU_INFO_FW_GFX_RLC_RESTORE_LIST_CNTL: printf("GFX_RLC_RESTORE_LIST_CNTL"); break;
						case AMDGPU_INFO_FW_GFX_RLC_RESTORE_LIST_GPM_MEM: printf("GFX_RLC_RESTORE_LIST_GPM_MEM"); break;
						case AMDGPU_INFO_FW_GFX_RLC_RESTORE_LIST_SRM_MEM: printf("GFX_RLC_RESTORE_LIST_SRM_MEM"); break;
						default: printf("?(%#" PRIx32 ")", args->query_fw.fw_type);
					}
					printf(", ip_instance: %"  PRIu32, args->query_fw.ip_instance);
					printf(", index: %"  PRIu32, args->query_fw.index);
					printf(")");
					break;
				}
				case 0x0f: printf("AMDGPU_INFO_NUM_BYTES_MOVED"); break;
				case 0x10: printf("AMDGPU_INFO_VRAM_USAGE"); break;
				case 0x11: printf("AMDGPU_INFO_GTT_USAGE"); break;
				case 0x13: printf("AMDGPU_INFO_GDS_CONFIG"); break;
				case 0x14: printf("AMDGPU_INFO_VRAM_GTT"); break;
				case 0x15: printf("AMDGPU_INFO_READ_MMR_REG"); {
					printf("(dword_offset: %#" PRIx32, args->read_mmr_reg.dword_offset);
					printf(", count: %#" PRIx32, args->read_mmr_reg.count);
					printf(", instance: %#" PRIx32, args->read_mmr_reg.instance);
					printf(", flags: %#" PRIx32, args->read_mmr_reg.flags);
					printf(")");
					break;
				}
				case 0x16: printf("AMDGPU_INFO_DEV_INFO"); break;
				case 0x17: printf("AMDGPU_INFO_VIS_VRAM_USAGE"); break;
				case 0x18: printf("AMDGPU_INFO_NUM_EVICTIONS"); break;
				case 0x19: printf("AMDGPU_INFO_MEMORY"); break;
				case 0x1a: printf("AMDGPU_INFO_VCE_CLOCK_TABLE"); break;
				case 0x1b: printf("AMDGPU_INFO_VBIOS"); break;
				case 0x1c: printf("AMDGPU_INFO_NUM_HANDLES"); break;
				case 0x1d: printf("AMDGPU_INFO_SENSOR"); break;
				case 0x1e: printf("AMDGPU_INFO_NUM_VRAM_CPU_PAGE_FAULTS"); break;
				case 0x1f: printf("AMDGPU_INFO_VRAM_LOST_COUNTER"); break;
				case 0x20: printf("AMDGPU_INFO_RAS_ENABLED_FEATURES"); break;
				case 0x21: printf("AMDGPU_INFO_VIDEO_CAPS"); break;
			}
			printf(")");
			break;
		}
		case 0x06: printf("AMDGPU_GEM_METADATA"); break;
		case 0x07: printf("AMDGPU_GEM_WAIT_IDLE"); break;
		case 0x08: printf("AMDGPU_GEM_VA"); {
			struct drm_amdgpu_gem_va *args = arg;
			printf("(handle: %" PRIu32, args->handle);
			printf(", operation: ");
			switch (args->operation) {
				case AMDGPU_VA_OP_MAP: printf("MAP"); break;
				case AMDGPU_VA_OP_UNMAP: printf("UNMAP"); break;
				case AMDGPU_VA_OP_CLEAR: printf("CLEAR"); break;
				case AMDGPU_VA_OP_REPLACE: printf("REPLACE"); break;
			}
			printf(", flags: %#" PRIx32, args->flags);
			printf(", va_address: %#" PRIx64, args->va_address);
			printf(", offset_in_bo: %#" PRIx64, args->offset_in_bo);
			printf(", map_size: %#" PRIx64 ")", args->map_size);
			break;
		}
		case 0x09: printf("AMDGPU_WAIT_CS"); {
			union drm_amdgpu_wait_cs *args = arg;
			printf("(handle: %" PRIu64, args->in.handle);
			printf(", timeout: %" PRIu64, args->in.timeout);
			printf(", ip_type: %" PRIu32, args->in.ip_type);
			printf(", ip_instance: %" PRIu32, args->in.ip_instance);
			printf(", ring: %" PRIu32, args->in.ring);
			printf(", ctx_id: %" PRIu32 ")", args->in.ctx_id);
			break;
		}
		case 0x10: printf("AMDGPU_GEM_OP"); break;
		case 0x11: printf("AMDGPU_GEM_USERPTR"); {
			struct drm_amdgpu_gem_userptr *args = arg;
			printf("(addr: %" PRIu64, args->addr);
			printf(", size: %" PRIu64, args->size);
			printf(", flags: %" PRIu32 ")", args->flags);
			break;
		}
		case 0x12: printf("AMDGPU_WAIT_FENCES"); break;
		case 0x13: printf("AMDGPU_VM"); break;
		case 0x14: printf("AMDGPU_FENCE_TO_HANDLE"); break;
		case 0x15: printf("DRM_AMDGPU_SCHED"); break;

		default: printf("?(%#lx)", cmd);
	}
}

static void drmWriteCmd(unsigned long cmd, void *arg, bool isPost)
{
	if (cmd >= DRM_COMMAND_BASE && cmd < DRM_COMMAND_END)
		return drmAmdgpuWriteCmd(cmd - DRM_COMMAND_BASE, arg, isPost);

	if (isPost) {
		switch (cmd) {
			case 0x00: /* VERSION */ {
				struct drm_version *args = arg;
				printf(" -> (\n");
				printf("  version: %d.%d.%d\n", args->version_major, args->version_minor, args->version_patchlevel);
				printf("  name: \"%s\"\n", args->name);
				printf("  date: \"%s\"\n", args->date);
				printf("  desc: \"%s\"\n", args->desc);
				printf(")");
				break;
			}
			case 0x0c: /* GET_CAP */ {
				struct drm_get_cap *args = arg;
				printf(" -> %" PRIx64, args->value);
				break;
			}
			case 0xbf: /* SYNCOBJ_CREATE */ {
				struct drm_syncobj_create *args = arg;
				printf(" -> %" PRIu32, args->handle);
				break;
			}
			case 0xc1: /* SYNCOBJ_HANDLE_TO_FD */ {
				struct drm_syncobj_handle *args = arg;
				printf(" -> %" PRIu32, args->fd);
				break;
			}
			case 0x2d: /* PRIME_HANDLE_TO_FD */ {
				struct drm_prime_handle *args = arg;
				printf("-> %" PRId32, args->fd);
				break;
			}
			case 0x2e: /* PRIME_FD_TO_HANDLE */ {
				struct drm_prime_handle *args = arg;
				printf(" -> %" PRIu32, args->handle);
				break;
			}
			case 0xc3: /* SYNCOBJ_WAIT */ {
				struct drm_syncobj_wait *args = arg;
				printf(" -> %" PRIu32, args->first_signaled);
				break;
			}
			case 0xca: /* SYNCOBJ_TIMELINE_WAIT */ {
				struct drm_syncobj_timeline_wait *args = arg;
				printf(" -> %" PRIu32, args->first_signaled);
				break;
			}
		}
		return;
	}

	switch (cmd) {
		case 0x00: printf("VERSION"); break;
		case 0x01: printf("GET_UNIQUE"); break;
		case 0x02: printf("GET_MAGIC"); break;
		case 0x03: printf("IRQ_BUSID"); break;
		case 0x04: printf("GET_MAP"); break;
		case 0x05: printf("GET_CLIENT"); break;
		case 0x06: printf("GET_STATS"); break;
		case 0x07: printf("SET_VERSION"); break;
		case 0x08: printf("MODESET_CTL"); break;
		case 0x09: printf("GEM_CLOSE"); {
			struct drm_gem_close *args = arg;
			printf("(%" PRIu32 ")", args->handle);
			break;
		}
		case 0x0a: printf("GEM_FLINK"); break;
		case 0x0b: printf("GEM_OPEN"); break;
		case 0x0c: printf("GET_CAP"); {
			struct drm_get_cap *args = arg;
			printf("(");
			switch (args->capability) {
				case DRM_CAP_DUMB_BUFFER: printf("DUMB_BUFFER"); break;
				case DRM_CAP_VBLANK_HIGH_CRTC: printf("VBLANK_HIGH_CRTC"); break;
				case DRM_CAP_DUMB_PREFERRED_DEPTH: printf("DUMB_PREFERRED_DEPTH"); break;
				case DRM_CAP_DUMB_PREFER_SHADOW: printf("DUMB_PREFER_SHADOW"); break;
				case DRM_CAP_PRIME: printf("PRIME"); break;
				case DRM_CAP_TIMESTAMP_MONOTONIC: printf("TIMESTAMP_MONOTONIC"); break;
				case DRM_CAP_ASYNC_PAGE_FLIP: printf("ASYNC_PAGE_FLIP"); break;
				case DRM_CAP_CURSOR_WIDTH: printf("CURSOR_WIDTH"); break;
				case DRM_CAP_CURSOR_HEIGHT: printf("CURSOR_HEIGHT"); break;
				case DRM_CAP_ADDFB2_MODIFIERS: printf("ADDFB2_MODIFIERS"); break;
				case DRM_CAP_PAGE_FLIP_TARGET: printf("PAGE_FLIP_TARGET"); break;
				case DRM_CAP_CRTC_IN_VBLANK_EVENT: printf("CRTC_IN_VBLANK_EVENT"); break;
				case DRM_CAP_SYNCOBJ: printf("SYNCOBJ"); break;
				case DRM_CAP_SYNCOBJ_TIMELINE: printf("SYNCOBJ_TIMELINE"); break;
				default: printf("? (%" PRIu64 ")", args->capability);
			}
			printf(")");
			break;
		}
		case 0x0d: printf("SET_CLIENT_CAP"); break;

		case 0x2d: printf("PRIME_HANDLE_TO_FD"); {
			struct drm_prime_handle *args = arg;
			printf("(%" PRIu32 ")", args->handle);
			break;
		}
		case 0x2e: printf("PRIME_FD_TO_HANDLE"); {
			struct drm_prime_handle *args = arg;
			printf("(fd: %" PRIu32, args->fd);
			printf(", flags: %#" PRIx32 ")", args->flags);
			break;
		}

		case 0xbf: printf("SYNCOBJ_CREATE"); {
			struct drm_syncobj_create *args = arg;
			printf("(flags: {");
			bool first = true;
			for (uint32_t i = 0; i < 32; i++) {
				if (((1 << i) & args->flags) != 0) {
					if (first) first = false; else printf(", ");
					switch (i) {
						case 0: printf("SIGNALED"); break;
						default: printf("?(%u)", i);
					}
				}
			}
			printf("})");
			break;
		}
		case 0xc0: printf("SYNCOBJ_DESTROY"); {
			struct drm_syncobj_destroy *args = arg;
			printf("(%" PRIu32 ")", args->handle);
			break;
		}
		case 0xc1: printf("SYNCOBJ_HANDLE_TO_FD"); {
			struct drm_syncobj_handle *args = arg;
			printf("(handle: %" PRIu32, args->handle);
			printf(", flags: {");
			bool first = true;
			for (uint32_t i = 0; i < 32; i++) {
				if (((1 << i) & args->flags) != 0) {
					if (first) first = false; else printf(", ");
					switch (i) {
						case 0: printf("EXPORT_SYNC_FILE"); break;
						default: printf("?(%u)", i);
					}
				}
			}
			printf("})");
			break;
		}
		case 0xc2: printf("SYNCOBJ_FD_TO_HANDLE"); {
			struct drm_syncobj_handle *args = arg;
			printf("(fd: %d", args->fd);
			printf(", handle: %" PRIu32, args->handle);
			printf(", flags: {");
			bool first = true;
			for (uint32_t i = 0; i < 32; i++) {
				if (((1 << i) & args->flags) != 0) {
					if (first) first = false; else printf(", ");
					switch (i) {
						case 0: printf("IMPORT_SYNC_FILE"); break;
						default: printf("?(%u)", i);
					}
				}
			}
			printf("})");
			break;
		}
		case 0xc3: printf("SYNCOBJ_WAIT"); {
			struct drm_syncobj_wait *args = arg;
			uint32_t *handles = (uint32_t*)args->handles;
			printf("(handles: (");
			for (uint32_t i = 0; i < args->count_handles; i++) {
				if (i > 0) printf(", ");
				printf("%" PRIu32, handles[i]);
			}
			printf("), timeout_nsec: %" PRIu64, args->timeout_nsec);
			printf(", count_handles: %" PRIu32, args->count_handles);
			printf(", flags: {");
			bool first = true;
			for (uint32_t i = 0; i < 32; i++) {
				if (((1 << i) & args->flags) != 0) {
					if (first) first = false; else printf(", ");
					switch (i) {
						case 0: printf("ALL"); break;
						case 1: printf("FOR_SUBMIT"); break;
						case 2: printf("AVAILABLE"); break;
						default: printf("?(%u)", i);
					}
				}
			}
			printf("})");
			break;
		}
		case 0xc4: printf("SYNCOBJ_RESET"); {
			struct drm_syncobj_array *args = arg;
			uint32_t *handles = (uint32_t*)args->handles;
			printf("(handles: (");
			for (uint32_t i = 0; i < args->count_handles; i++) {
				if (i > 0) printf(", ");
				printf("%" PRIu32, handles[i]);
			}
			printf("), count_handles: %" PRIu32, args->count_handles);
			printf(")");
			break;
		}
		case 0xc5: printf("SYNCOBJ_SIGNAL"); break;

		case 0xca: printf("SYNCOBJ_TIMELINE_WAIT"); {
			struct drm_syncobj_timeline_wait *args = arg;
			uint32_t *handles = (uint32_t*)args->handles;
			uint64_t *points = (uint64_t*)args->points;
			printf("(handles: (");
			for (uint32_t i = 0; i < args->count_handles; i++) {
				if (i > 0) printf(", ");
				printf("%" PRIu32, handles[i]);
			}
			printf("), points: (");
			for (uint32_t i = 0; i < args->count_handles; i++) {
				if (i > 0) printf(", ");
				printf("%" PRIu64, points[i]);
			}
			printf("), timeout_nsec: %" PRIu64, args->timeout_nsec);
			printf(", count_handles: %" PRIu32, args->count_handles);
			printf(", flags: {");
			bool first = true;
			for (uint32_t i = 0; i < 32; i++) {
				if (((1 << i) & args->flags) != 0) {
					if (first) first = false; else printf(", ");
					switch (i) {
						case 0: printf("ALL"); break;
						case 1: printf("FOR_SUBMIT"); break;
						case 2: printf("AVAILABLE"); break;
						default: printf("?(%u)", i);
					}
				}
			}
			printf("})");
			break;
		}
		case 0xcb: printf("SYNCOBJ_QUERY"); break;
		case 0xcc: printf("SYNCOBJ_TRANSFER"); {
			struct drm_syncobj_transfer *args = arg;
			printf("(src_handle: %" PRIu32, args->src_handle);
			printf(", dst_handle: %" PRIu32, args->dst_handle);
			printf(", src_point: %" PRIu64, args->src_point);
			printf(", dst_point: %" PRIu64, args->dst_point);
			printf(", flags: {");
			bool first = true;
			for (uint32_t i = 0; i < 32; i++) {
				if (((1 << i) & args->flags) != 0) {
					if (first) first = false; else printf(", ");
					switch (i) {
						case 0: printf("ALL"); break;
						case 1: printf("FOR_SUBMIT"); break;
						case 2: printf("AVAILABLE"); break;
						default: printf("?(%u)", i);
					}
				}
			}
			printf("})");
			break;
		}
		case 0xcd: printf("SYNCOBJ_TIMELINE_SIGNAL"); break;
		case 0xcf: printf("SYNCOBJ_ACCUMULATE"); {
			struct drm_syncobj_accumulate *args = arg;
			printf("(syncobj1: %" PRIu32, args->syncobj1);
			printf(", syncobj2: %" PRIu32, args->syncobj2);
			printf(", point: %" PRIu64, args->point);
			printf(")");
			break;
		}

		default: printf("?(%#lx)", cmd);
	}
}

void drmIoctlTrace(int fd, unsigned long request, void *arg, bool isPost)
{
	unsigned long requestGroup = IOCGROUP(request);
	unsigned long requestCmd = request%0x100;
	unsigned long requestSize = IOCPARM_LEN(request);
	unsigned long requestDir = (request & IOC_DIRMASK);
	
	if (!isPost) {
		printf("drmIoctl(%d, (%lu, ", fd, requestGroup);
		drmWriteCmd(requestCmd, arg, false);
		printf(", %lu, ", requestSize);
		switch (requestDir) {
			case IOC_VOID:  printf("-"); break;
			case IOC_IN:    printf("R"); break;
			case IOC_OUT:   printf("W"); break;
			case IOC_INOUT: printf("RW"); break;
		}
		printf("))");
	} else {
		drmWriteCmd(requestCmd, arg, true);
		printf("\n");
	}
}
