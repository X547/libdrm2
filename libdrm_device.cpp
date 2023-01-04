#define HAVE_VISIBILITY 1

#include <xf86drm.h>
#include "libdrm_macros.h"
#include "util_math.h"
#include <libdrm/amdgpu_drm.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>

#define memclear(s) memset(&s, 0, sizeof(s))

#define ATI_VENDOR_ID         0x1002


extern void *gDriverHooks[];

// #pragma mark -

drm_public int drmGetNodeTypeFromFd(int fd)
{
    return DRM_NODE_RENDER;
}

drm_public char *drmGetPrimaryDeviceNameFromFd(int fd)
{
    return strdup("");
}

static int drmGetMaxNodeName(void)
{
    return 256;
}

static drmDevicePtr drmDeviceAlloc(unsigned int type, const char *node, size_t bus_size, size_t device_size) {
    size_t max_node_length, extra, size;
    drmDevicePtr device;
    unsigned int i;
    char *ptr;

    max_node_length = ALIGN(drmGetMaxNodeName(), sizeof(void *));
    extra = DRM_NODE_MAX * (sizeof(void *) + max_node_length);

    size = sizeof(*device) + extra + bus_size + device_size;

    device = (drmDevicePtr)calloc(1, size);
    if (!device)
        return NULL;

    device->available_nodes = 1 << type;

    ptr = (char *)device + sizeof(*device);
    device->nodes = (char **)ptr;

    ptr += DRM_NODE_MAX * sizeof(void *);

    for (i = 0; i < DRM_NODE_MAX; i++) {
        device->nodes[i] = ptr;
        ptr += max_node_length;
    }

    memcpy(device->nodes[type], node, max_node_length);

    //*ptrp = ptr;

		device->businfo.pci = (drmPciBusInfoPtr)ptr;
		ptr += bus_size;
		device->deviceinfo.pci = (drmPciDeviceInfoPtr)ptr;
		ptr += device_size;

    return device;
}

static int drmGetDeviceInt(drmDevicePtr *device, const char *path)
{
	drmDevicePtr dev = drmDeviceAlloc(DRM_NODE_RENDER, path, sizeof(drmPciBusInfo), sizeof(drmPciDeviceInfo));

	// !!!
	*dev->businfo.pci = (drmPciBusInfo){
		.domain = 0,
		.bus = 1,
		.dev = 0,
		.func = 0,
	};
	*dev->deviceinfo.pci = (drmPciDeviceInfo){
		.vendor_id = ATI_VENDOR_ID,
		.device_id = 0x683f,
	};

	*device = dev;
	return 0;
}


drm_public int drmGetDevices2(uint32_t flags, drmDevicePtr devices[], int max_devices)
{
	fprintf(stderr, "drmGetDevices2()\n");
	int deviceCnt = 1;
	if (devices == NULL) {
		return deviceCnt;
	}
	if (max_devices >= 1) {
		drmGetDeviceInt(&devices[0], "/dev/graphics/radeon_gfx_010000" /* !!! */);
		return deviceCnt;
	}
	return 0;
}

drm_public void drmFreeDevices(drmDevicePtr devices[], int count)
{
    fprintf(stderr, "drmFreeDevices()\n");
    int i;

    if (devices == NULL)
        return;

    for (i = 0; i < count; i++)
        if (devices[i])
            drmFreeDevice(&devices[i]);
}

static void drmFreePlatformDevice(drmDevicePtr device)
{
    if (device->deviceinfo.platform) {
        if (device->deviceinfo.platform->compatible) {
            char **compatible = device->deviceinfo.platform->compatible;

            while (*compatible) {
                free(*compatible);
                compatible++;
            }

            free(device->deviceinfo.platform->compatible);
        }
    }
}

static void drmFreeHost1xDevice(drmDevicePtr device)
{
    if (device->deviceinfo.host1x) {
        if (device->deviceinfo.host1x->compatible) {
            char **compatible = device->deviceinfo.host1x->compatible;

            while (*compatible) {
                free(*compatible);
                compatible++;
            }

            free(device->deviceinfo.host1x->compatible);
        }
    }
}

drm_public void drmFreeDevice(drmDevicePtr *device)
{
    fprintf(stderr, "drmFreeDevice()\n");
    if (device == NULL)
        return;

    if (*device) {
        switch ((*device)->bustype) {
        case DRM_BUS_PLATFORM:
            drmFreePlatformDevice(*device);
            break;

        case DRM_BUS_HOST1X:
            drmFreeHost1xDevice(*device);
            break;
        }
    }

    free(*device);
    *device = NULL;
}

drm_public int drmGetDevice2(int fd, uint32_t flags, drmDevicePtr *device)
{
	fprintf(stderr, "drmGetDevice2()\n");
	drmGetDeviceInt(device, "/dev/graphics/radeon_gfx_010000" /* !!! */);
	return 0;
}

drm_public char *drmGetRenderDeviceNameFromFd(int fd)
{
    return strdup("/dev/graphics/radeon_gfx_010000" /* !!! */);
}
