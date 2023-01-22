/*
 * Copyright © 2014 Advanced Micro Devices, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>

#include <inttypes.h>

#include <libdrm/drm.h>
#include "libdrm_macros.h"
#include "xf86drm.h"
#include "amdgpu_drm.h"
#include "amdgpu_internal.h"
#include "util_math.h"

static int amdgpu_bo_create(amdgpu_device_handle dev,
			    uint64_t size,
			    uint32_t handle,
			    amdgpu_bo_handle *buf_handle)
{
	struct amdgpu_bo *bo;
	int r;

	bo = calloc(1, sizeof(struct amdgpu_bo));
	if (!bo)
		return ENOMEM;

	r = handle_table_insert(&dev->bo_handles, handle, bo);
	if (r) {
		free(bo);
		return r;
	}

	atomic_set(&bo->refcount, 1);
	bo->dev = dev;
	bo->alloc_size = size;
	bo->handle = handle;
	pthread_mutex_init(&bo->cpu_access_mutex, NULL);

	*buf_handle = bo;
	return 0;
}

drm_public int amdgpu_bo_alloc(amdgpu_device_handle dev,
			       struct amdgpu_bo_alloc_request *alloc_buffer,
			       amdgpu_bo_handle *buf_handle)
{
	uint32_t handle;
	int r;

	/* Allocate the buffer with the preferred heap. */
	r = dev->acc_amdgpu->vt->AmdgpuBoAlloc(dev->acc_amdgpu, alloc_buffer, &handle);
	if (r)
		goto out;

	pthread_mutex_lock(&dev->bo_table_mutex);
	r = amdgpu_bo_create(dev, alloc_buffer->alloc_size, handle, buf_handle);
	pthread_mutex_unlock(&dev->bo_table_mutex);
	if (r) {
		dev->acc_drm->vt->DrmCloseBufferHandle(dev->acc_drm, handle);
	}

out:
	return r;
}

drm_public int amdgpu_bo_set_metadata(amdgpu_bo_handle bo,
				      struct amdgpu_bo_metadata *info)
{
	return bo->dev->acc_amdgpu->vt->AmdgpuBoSetMetadata(bo->dev->acc_amdgpu, bo->handle, info);
}

drm_public int amdgpu_bo_query_info(amdgpu_bo_handle bo,
				    struct amdgpu_bo_info *info)
{
	return bo->dev->acc_amdgpu->vt->AmdgpuBoQueryInfo(bo->dev->acc_amdgpu, bo->handle, info);
}

static int amdgpu_bo_export_flink(amdgpu_bo_handle bo)
{
	return ENOSYS;
}

drm_public int amdgpu_bo_export(amdgpu_bo_handle bo,
				enum amdgpu_bo_handle_type type,
				uint32_t *shared_handle)
{
	int r;

	switch (type) {
	case amdgpu_bo_handle_type_gem_flink_name:
		r = amdgpu_bo_export_flink(bo);
		if (r)
			return r;

		*shared_handle = bo->flink_name;
		return 0;

	case amdgpu_bo_handle_type_kms:
	case amdgpu_bo_handle_type_kms_noimport:
		*shared_handle = bo->handle;
		return 0;

	case amdgpu_bo_handle_type_dma_buf_fd:
		return bo->dev->acc_drm->vt->DrmPrimeHandleToFD(bo->dev->acc_drm, bo->handle, DRM_CLOEXEC | DRM_RDWR, (int*)shared_handle);
	}
	return EINVAL;
}

drm_public int amdgpu_bo_import(amdgpu_device_handle dev,
				enum amdgpu_bo_handle_type type,
				uint32_t shared_handle,
		     struct amdgpu_bo_import_result *output)
{
	struct amdgpu_bo *bo = NULL;
	uint32_t handle = 0, flink_name = 0;
	uint64_t alloc_size = 0;
	int r = 0;
	uint64_t dma_buf_size = 0;
	struct amdgpu_bo_info info = {};

	/* We must maintain a list of pairs <handle, bo>, so that we always
	 * return the same amdgpu_bo instance for the same handle. */
	pthread_mutex_lock(&dev->bo_table_mutex);

	/* Convert a DMA buf handle to a KMS handle now. */
	if (type == amdgpu_bo_handle_type_dma_buf_fd) {

		/* Get a KMS handle. */
		r = bo->dev->acc_drm->vt->DrmPrimeFDToHandle(bo->dev->acc_drm, shared_handle, &handle);
		if (r)
			goto unlock;

		/* Query the buffer size. */
		r = bo->dev->acc_amdgpu->vt->AmdgpuBoQueryInfo(bo->dev->acc_amdgpu, handle, &info);
		if (r)
			goto unlock;

		dma_buf_size = info.alloc_size;
		shared_handle = handle;
	}

	/* If we have already created a buffer with this handle, find it. */
	switch (type) {
	case amdgpu_bo_handle_type_gem_flink_name:
		bo = handle_table_lookup(&dev->bo_flink_names, shared_handle);
		break;

	case amdgpu_bo_handle_type_dma_buf_fd:
		bo = handle_table_lookup(&dev->bo_handles, shared_handle);
		break;

	case amdgpu_bo_handle_type_kms:
	case amdgpu_bo_handle_type_kms_noimport:
		/* Importing a KMS handle in not allowed. */
		r = EPERM;
		goto unlock;

	default:
		r = EINVAL;
		goto unlock;
	}

	if (bo) {
		/* The buffer already exists, just bump the refcount. */
		atomic_add(&bo->refcount, 1);
		pthread_mutex_unlock(&dev->bo_table_mutex);

		output->buf_handle = bo;
		output->alloc_size = bo->alloc_size;
		return 0;
	}

	/* Open the handle. */
	switch (type) {
	case amdgpu_bo_handle_type_gem_flink_name:
		r = ENOSYS;
		goto unlock;
		break;

	case amdgpu_bo_handle_type_dma_buf_fd:
		handle = shared_handle;
		alloc_size = dma_buf_size;
		break;

	case amdgpu_bo_handle_type_kms:
	case amdgpu_bo_handle_type_kms_noimport:
		assert(0); /* unreachable */
	}

	/* Initialize it. */
	r = amdgpu_bo_create(dev, alloc_size, handle, &bo);
	if (r)
		goto free_bo_handle;

	if (flink_name) {
		bo->flink_name = flink_name;
		r = handle_table_insert(&dev->bo_flink_names, flink_name,
					bo);
		if (r)
			goto free_bo_handle;

	}

	output->buf_handle = bo;
	output->alloc_size = bo->alloc_size;
	pthread_mutex_unlock(&dev->bo_table_mutex);
	return 0;

free_bo_handle:
	if (bo)
		amdgpu_bo_free(bo);
	else
		dev->acc_drm->vt->DrmCloseBufferHandle(dev->acc_drm, handle);
unlock:
	pthread_mutex_unlock(&dev->bo_table_mutex);
	return r;
}

drm_public int amdgpu_bo_free(amdgpu_bo_handle buf_handle)
{
	struct amdgpu_device *dev;
	struct amdgpu_bo *bo = buf_handle;

	assert(bo != NULL);
	dev = bo->dev;
	pthread_mutex_lock(&dev->bo_table_mutex);

	if (update_references(&bo->refcount, NULL)) {
		/* Remove the buffer from the hash tables. */
		handle_table_remove(&dev->bo_handles, bo->handle);

		if (bo->flink_name)
			handle_table_remove(&dev->bo_flink_names,
					    bo->flink_name);

		/* Release CPU access. */
		if (bo->cpu_map_count > 0) {
			bo->cpu_map_count = 1;
			amdgpu_bo_cpu_unmap(bo);
		}

		dev->acc_drm->vt->DrmCloseBufferHandle(dev->acc_drm, bo->handle);
		pthread_mutex_destroy(&bo->cpu_access_mutex);
		free(bo);
	}

	pthread_mutex_unlock(&dev->bo_table_mutex);

	return 0;
}

drm_public void amdgpu_bo_inc_ref(amdgpu_bo_handle bo)
{
	atomic_add(&bo->refcount, 1);
}

drm_public int amdgpu_bo_cpu_map(amdgpu_bo_handle bo, void **cpu)
{
	void *ptr;
	int r;

	pthread_mutex_lock(&bo->cpu_access_mutex);

	if (bo->cpu_ptr) {
		/* already mapped */
		assert(bo->cpu_map_count > 0);
		bo->cpu_map_count++;
		*cpu = bo->cpu_ptr;
		pthread_mutex_unlock(&bo->cpu_access_mutex);
		return 0;
	}

	assert(bo->cpu_map_count == 0);

	r = bo->dev->acc_amdgpu->vt->AmdgpuBoCpuMap(bo->dev->acc_amdgpu, bo->handle, &ptr);
	if (r) {
		pthread_mutex_unlock(&bo->cpu_access_mutex);
		return r;
	}

	bo->cpu_ptr = ptr;
	bo->cpu_map_count = 1;
	pthread_mutex_unlock(&bo->cpu_access_mutex);

	*cpu = ptr;
	return 0;
}

drm_public int amdgpu_bo_cpu_unmap(amdgpu_bo_handle bo)
{
	int r;

	pthread_mutex_lock(&bo->cpu_access_mutex);
	assert(bo->cpu_map_count >= 0);

	if (bo->cpu_map_count == 0) {
		/* not mapped */
		pthread_mutex_unlock(&bo->cpu_access_mutex);
		return EINVAL;
	}

	bo->cpu_map_count--;
	if (bo->cpu_map_count > 0) {
		/* mapped multiple times */
		pthread_mutex_unlock(&bo->cpu_access_mutex);
		return 0;
	}

	r = drm_munmap(bo->cpu_ptr, bo->alloc_size) == 0 ? 0 : -errno;
	bo->cpu_ptr = NULL;
	pthread_mutex_unlock(&bo->cpu_access_mutex);
	return r;
}

drm_public int amdgpu_query_buffer_size_alignment(amdgpu_device_handle dev,
				struct amdgpu_buffer_size_alignments *info)
{
	info->size_local = dev->dev_info.pte_fragment_size;
	info->size_remote = dev->dev_info.gart_page_size;
	return 0;
}

drm_public int amdgpu_bo_wait_for_idle(amdgpu_bo_handle bo,
				       uint64_t timeout_ns,
			    bool *busy)
{
	// no implicit sync suport
	return ENOSYS;
}

drm_public int amdgpu_find_bo_by_cpu_mapping(amdgpu_device_handle dev,
					     void *cpu,
					     uint64_t size,
					     amdgpu_bo_handle *buf_handle,
					     uint64_t *offset_in_bo)
{
	struct amdgpu_bo *bo;
	uint32_t i;
	int r = 0;

	if (cpu == NULL || size == 0)
		return EINVAL;

	/*
	 * Workaround for a buggy application which tries to import previously
	 * exposed CPU pointers. If we find a real world use case we should
	 * improve that by asking the kernel for the right handle.
	 */
	pthread_mutex_lock(&dev->bo_table_mutex);
	for (i = 0; i < dev->bo_handles.max_key; i++) {
		bo = handle_table_lookup(&dev->bo_handles, i);
		if (!bo || !bo->cpu_ptr || size > bo->alloc_size)
			continue;
		if (cpu >= bo->cpu_ptr &&
		    cpu < (void*)((uintptr_t)bo->cpu_ptr + bo->alloc_size))
			break;
	}

	if (i < dev->bo_handles.max_key) {
		atomic_add(&bo->refcount, 1);
		*buf_handle = bo;
		*offset_in_bo = (uintptr_t)cpu - (uintptr_t)bo->cpu_ptr;
	} else {
		*buf_handle = NULL;
		*offset_in_bo = 0;
		r = ENXIO;
	}
	pthread_mutex_unlock(&dev->bo_table_mutex);

	return r;
}

drm_public int amdgpu_create_bo_from_user_mem(amdgpu_device_handle dev,
					      void *cpu,
					      uint64_t size,
					      amdgpu_bo_handle *buf_handle)
{
	int r;
	uint32_t handle;

/*
	flags = AMDGPU_GEM_USERPTR_ANONONLY | AMDGPU_GEM_USERPTR_REGISTER |
		AMDGPU_GEM_USERPTR_VALIDATE;
*/
	r = dev->acc_amdgpu->vt->AmdgpuCreateBoFromUserMem(dev->acc_amdgpu, cpu, size, &handle);
	if (r)
		goto out;

	pthread_mutex_lock(&dev->bo_table_mutex);
	r = amdgpu_bo_create(dev, size, handle, buf_handle);
	pthread_mutex_unlock(&dev->bo_table_mutex);
	if (r) {
		dev->acc_drm->vt->DrmCloseBufferHandle(dev->acc_drm, handle);
	}

out:
	return r;
}

drm_public int amdgpu_bo_list_create_raw(amdgpu_device_handle dev,
					 uint32_t number_of_buffers,
					 struct drm_amdgpu_bo_list_entry *buffers,
					 uint32_t *result)
{
	return ENOSYS;
}

drm_public int amdgpu_bo_list_destroy_raw(amdgpu_device_handle dev,
					  uint32_t bo_list)
{
	return ENOSYS;
}

drm_public int amdgpu_bo_list_create(amdgpu_device_handle dev,
				     uint32_t number_of_resources,
				     amdgpu_bo_handle *resources,
				     uint8_t *resource_prios,
				     amdgpu_bo_list_handle *result)
{
	return ENOSYS;
}

drm_public int amdgpu_bo_list_destroy(amdgpu_bo_list_handle list)
{
	return ENOSYS;
}

drm_public int amdgpu_bo_list_update(amdgpu_bo_list_handle handle,
				     uint32_t number_of_resources,
				     amdgpu_bo_handle *resources,
				     uint8_t *resource_prios)
{
	return ENOSYS;
}

drm_public int amdgpu_bo_va_op(amdgpu_bo_handle bo,
			       uint64_t offset,
			       uint64_t size,
			       uint64_t addr,
			       uint64_t flags,
			       uint32_t ops)
{
	amdgpu_device_handle dev = bo->dev;

	size = ALIGN(size, getpagesize());

	return amdgpu_bo_va_op_raw(dev, bo, offset, size, addr,
				   AMDGPU_VM_PAGE_READABLE |
				   AMDGPU_VM_PAGE_WRITEABLE |
				   AMDGPU_VM_PAGE_EXECUTABLE, ops);
}

drm_public int amdgpu_bo_va_op_raw(amdgpu_device_handle dev,
				   amdgpu_bo_handle bo,
				   uint64_t offset,
				   uint64_t size,
				   uint64_t addr,
				   uint64_t flags,
				   uint32_t ops)
{
	int r;

	if (ops != AMDGPU_VA_OP_MAP && ops != AMDGPU_VA_OP_UNMAP &&
	    ops != AMDGPU_VA_OP_REPLACE && ops != AMDGPU_VA_OP_CLEAR)
		return EINVAL;

	r = dev->acc_amdgpu->vt->AmdgpuBoVaOpRaw(dev->acc_amdgpu, bo ? bo->handle : 0, offset, size, addr, flags, ops);

	return r;
}
