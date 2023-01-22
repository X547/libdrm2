/*
 * Copyright 2014 Advanced Micro Devices, Inc.
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

/**
 * \file amdgpu_device.c
 *
 *  Implementation of functions for AMD GPU device
 *
 */

#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "xf86drm.h"
#include "amdgpu_drm.h"
#include "amdgpu_internal.h"
#include "util_math.h"

#define PTR_TO_UINT(x) ((unsigned)((intptr_t)(x)))

static pthread_mutex_t dev_mutex = PTHREAD_MUTEX_INITIALIZER;
static amdgpu_device_handle dev_list;

static void amdgpu_device_free_internal(amdgpu_device_handle dev)
{
	amdgpu_device_handle *node = &dev_list;

	pthread_mutex_lock(&dev_mutex);
	while (*node != dev && (*node)->next)
		node = &(*node)->next;
	*node = (*node)->next;
	pthread_mutex_unlock(&dev_mutex);

	dev->acc_base->vt->ReleaseReference(dev->acc_base);

	amdgpu_vamgr_deinit(&dev->vamgr_32);
	amdgpu_vamgr_deinit(&dev->vamgr);
	amdgpu_vamgr_deinit(&dev->vamgr_high_32);
	amdgpu_vamgr_deinit(&dev->vamgr_high);
	handle_table_fini(&dev->bo_handles);
	handle_table_fini(&dev->bo_flink_names);
	pthread_mutex_destroy(&dev->bo_table_mutex);
	free(dev->marketing_name);
	free(dev);
}


/**
 * Assignment between two amdgpu_device pointers with reference counting.
 *
 * Usage:
 *    struct amdgpu_device *dst = ... , *src = ...;
 *
 *    dst = src;
 *    // No reference counting. Only use this when you need to move
 *    // a reference from one pointer to another.
 *
 *    amdgpu_device_reference(&dst, src);
 *    // Reference counters are updated. dst is decremented and src is
 *    // incremented. dst is freed if its reference counter is 0.
 */
static void amdgpu_device_reference(struct amdgpu_device **dst,
				    struct amdgpu_device *src)
{
	if (update_references(&(*dst)->refcount, &src->refcount))
		amdgpu_device_free_internal(*dst);
	*dst = src;
}

drm_public int amdgpu_device_initialize_haiku(struct accelerant_base *acc,
					uint32_t *major_version,
					uint32_t *minor_version,
					amdgpu_device_handle *device_handle)
{
	struct amdgpu_device *dev;
	struct drm_version version;
	int r;
	uint32_t accel_working = 0;
	uint64_t start, max;

	*device_handle = NULL;

	pthread_mutex_lock(&dev_mutex);

	for (dev = dev_list; dev; dev = dev->next)
		if (dev->acc_base == acc)
			break;

	if (dev) {
		*major_version = dev->major_version;
		*minor_version = dev->minor_version;
		amdgpu_device_reference(device_handle, dev);
		pthread_mutex_unlock(&dev_mutex);
		return 0;
	}

	dev = calloc(1, sizeof(struct amdgpu_device));
	if (!dev) {
		fprintf(stderr, "%s: calloc failed\n", __func__);
		pthread_mutex_unlock(&dev_mutex);
		return ENOMEM;
	}

	dev->acc_base = NULL;
	dev->acc_drm = NULL;
	dev->acc_amdgpu = NULL;

	atomic_set(&dev->refcount, 1);

	acc->vt->AcquireReference(acc);
	dev->acc_base = acc;
	dev->acc_drm = (accelerant_drm*)dev->acc_base->vt->QueryInterface(dev->acc_base, B_ACCELERANT_IFACE_DRM);
	if (!dev->acc_drm) {
		fprintf(stderr, "%s: Accelerant don't provide DRM interface\n", __func__);
		r = EBADF;
		goto cleanup;
	}
	dev->acc_amdgpu = (accelerant_amdgpu*)dev->acc_base->vt->QueryInterface(dev->acc_base, B_ACCELERANT_IFACE_AMDGPU);
	if (!dev->acc_amdgpu) {
		fprintf(stderr, "%s: Accelerant don't provide AMDGPU interface\n", __func__);
		r = EBADF;
		goto cleanup;
	}

	memset(&version, 0, sizeof(version));
	dev->acc_drm->vt->DrmVersion(dev->acc_drm, &version);

	if (version.version_major != 3) {
		fprintf(stderr, "%s: DRM version is %d.%d.%d but this driver is "
			"only compatible with 3.x.x.\n",
			__func__,
			version.version_major,
			version.version_minor,
			version.version_patchlevel);
		r = EBADF;
		goto cleanup;
	}
	dev->major_version = version.version_major;
	dev->minor_version = version.version_minor;

	pthread_mutex_init(&dev->bo_table_mutex, NULL);

	/* Check if acceleration is working. */
	r = amdgpu_query_info(dev, AMDGPU_INFO_ACCEL_WORKING, 4, &accel_working);
	if (r) {
		fprintf(stderr, "%s: amdgpu_query_info(ACCEL_WORKING) failed (%i)\n",
			__func__, r);
		goto cleanup;
	}
	if (!accel_working) {
		fprintf(stderr, "%s: AMDGPU_INFO_ACCEL_WORKING = 0\n", __func__);
		r = EBADF;
		goto cleanup;
	}

	r = amdgpu_query_gpu_info_init(dev);
	if (r) {
		fprintf(stderr, "%s: amdgpu_query_gpu_info_init failed\n", __func__);
		goto cleanup;
	}

	start = dev->dev_info.virtual_address_offset;
	max = MIN2(dev->dev_info.virtual_address_max, 0x100000000ULL);
	amdgpu_vamgr_init(&dev->vamgr_32, start, max,
			  dev->dev_info.virtual_address_alignment);

	start = max;
	max = MAX2(dev->dev_info.virtual_address_max, 0x100000000ULL);
	amdgpu_vamgr_init(&dev->vamgr, start, max,
			  dev->dev_info.virtual_address_alignment);

	start = dev->dev_info.high_va_offset;
	max = MIN2(dev->dev_info.high_va_max, (start & ~0xffffffffULL) +
		   0x100000000ULL);
	amdgpu_vamgr_init(&dev->vamgr_high_32, start, max,
			  dev->dev_info.virtual_address_alignment);

	start = max;
	max = MAX2(dev->dev_info.high_va_max, (start & ~0xffffffffULL) +
		   0x100000000ULL);
	amdgpu_vamgr_init(&dev->vamgr_high, start, max,
			  dev->dev_info.virtual_address_alignment);

	amdgpu_parse_asic_ids(dev);

	*major_version = dev->major_version;
	*minor_version = dev->minor_version;
	*device_handle = dev;
	dev->next = dev_list;
	dev_list = dev;
	pthread_mutex_unlock(&dev_mutex);

	return 0;

cleanup:
	if (dev->acc_base != NULL)
		dev->acc_base->vt->ReleaseReference(dev->acc_base);
	free(dev);
	pthread_mutex_unlock(&dev_mutex);
	return r;
}

drm_public int amdgpu_device_deinitialize(amdgpu_device_handle dev)
{
	amdgpu_device_reference(&dev, NULL);
	return 0;
}

drm_public accelerant_base *amdgpu_device_get_accelerant(amdgpu_device_handle device_handle)
{
	return device_handle->acc_base;
}

drm_public const char *amdgpu_get_marketing_name(amdgpu_device_handle dev)
{
	return dev->marketing_name;
}

drm_public int amdgpu_query_sw_info(amdgpu_device_handle dev,
				    enum amdgpu_sw_info info,
				    void *value)
{
	uint32_t *val32 = (uint32_t*)value;

	switch (info) {
	case amdgpu_sw_info_address32_hi:
		if (dev->vamgr_high_32.va_max)
			*val32 = (dev->vamgr_high_32.va_max - 1) >> 32;
		else
			*val32 = (dev->vamgr_32.va_max - 1) >> 32;
		return 0;
	}
	return EINVAL;
}
