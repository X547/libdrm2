#define HAVE_VISIBILITY 1

#include <xf86drm.h>
#include "libdrm_macros.h"
#include "util_math.h"
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>

#include <algorithm>

#include <Entry.h>
#include <Directory.h>

#include "Poke.h"

#define memclear(s) memset(&s, 0, sizeof(s))

static const char sDevPath[] = "/dev/graphics/";


static bool ParseDevPciAdr(uint8 &bus, uint8 &device, uint8 function, const char *name)
{
	size_t nameLen = strlen(name);
	if (nameLen <= 7 || name[nameLen - 7] != '_') return false;
	char part[3] = "00";
	char *end;
	memcpy(part, &name[nameLen - 6], 2);
	bus = (uint8)strtol(part, &end, 16);
	if (end - part != 2) return false;
	memcpy(part, &name[nameLen - 4], 2);
	device = (uint8)strtol(part, &end, 16);
	if (end - part != 2) return false;
	memcpy(part, &name[nameLen - 2], 2);
	function = (uint8)strtol(part, &end, 16);
	if (end - part != 2) return false;
	return true;
}

static status_t LookupPciDevice(pci_info &info, uint8 bus, uint8 device, uint8 function)
{
	uint32 index = 0;
	for (;;) {
		status_t res = gPoke.GetNthPciInfo(index, info);
		if (res < B_OK) return res;
		if (info.bus == bus && info.device == device && info.function == function) return B_OK;
		index++;
	}
}


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

static int drmGetDeviceInt(drmDevicePtr *device, const char *name)
{
	*device = NULL;

	pci_info info {};
	if (!ParseDevPciAdr(info.bus, info.device, info.function, name)) return ENOENT;

	status_t res = LookupPciDevice(info, info.bus, info.device, info.function);
	if (res < B_OK) return res;

	char path[MAXPATHLEN];
	sprintf(path, "%s%s", sDevPath, name);

	drmDevicePtr dev = drmDeviceAlloc(DRM_NODE_RENDER, path, sizeof(drmPciBusInfo), sizeof(drmPciDeviceInfo));
	if (dev == NULL) return B_NO_MEMORY;

	*dev->businfo.pci = (drmPciBusInfo){
		.domain = 0, // !!!
		.bus = info.bus,
		.dev = info.device,
		.func = info.function,
	};
	*dev->deviceinfo.pci = (drmPciDeviceInfo){
		.vendor_id = info.vendor_id,
		.device_id = info.device_id,
	};

	*device = dev;
	return 0;
}


drm_public int drmGetDevices2(uint32_t flags, drmDevicePtr devices[], int max_devices)
{
	fprintf(stderr, "drmGetDevices2()\n");

	BDirectory dir(sDevPath);
	dir.Rewind();
	int deviceCnt;
	for (deviceCnt = 0; deviceCnt < max_devices;) {
		BEntry entry;
		if (dir.GetNextEntry(&entry, true) < B_OK) break;
		if (devices != NULL) {
			drmGetDeviceInt(&devices[deviceCnt], entry.Name());
			if (devices[deviceCnt] != NULL) deviceCnt++;
		}
	}
	return deviceCnt;
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

	struct stat st {};
	if (fstat(fd, &st) < 0) return B_ERROR;

	BDirectory dir(sDevPath);
	dir.Rewind();
	BEntry entry;
	while (dir.GetNextEntry(&entry, true) >= B_OK) {
		struct stat st2 {};
		if (entry.GetStat(&st2) < B_OK) return B_ERROR;
		if (st.st_dev == st2.st_dev && st.st_ino == st2.st_ino) {
			drmGetDeviceInt(device, entry.Name());
			return 0;
		}
	}
	return B_ERROR;
}

drm_public char *drmGetRenderDeviceNameFromFd(int fd)
{
	struct stat st {};
	if (fstat(fd, &st) < 0) return NULL;

	BDirectory dir(sDevPath);
	dir.Rewind();
	BEntry entry;
	while (dir.GetNextEntry(&entry, true) >= B_OK) {
		struct stat st2 {};
		if (entry.GetStat(&st2) < B_OK) return NULL;
		if (st.st_dev == st2.st_dev && st.st_ino == st2.st_ino) {
			char *path = (char*)malloc(strlen(sDevPath) + strlen(entry.Name() + 1));
			if (path == NULL) return NULL;
			sprintf(path, "%s%s", sDevPath, entry.Name());
			return path;
		}
	}
	return NULL;
}
