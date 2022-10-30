#define HAVE_VISIBILITY 1

#include "xf86drm.h"
#include "libdrm_macros.h"
#include <errno.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h>

#define memclear(s) memset(&s, 0, sizeof(s))


// #pragma mark - utils

drm_public void *drmMalloc(int size)
{
    return calloc(1, size);
}

drm_public void drmFree(void *pt)
{
    free(pt);
}

drm_public int drmError(int err, const char *label)
{
    switch (err) {
    case DRM_ERR_NO_DEVICE:
        fprintf(stderr, "%s: no device\n", label);
        break;
    case DRM_ERR_NO_ACCESS:
        fprintf(stderr, "%s: no access\n", label);
        break;
    case DRM_ERR_NOT_ROOT:
        fprintf(stderr, "%s: not root\n", label);
        break;
    case DRM_ERR_INVALID:
        fprintf(stderr, "%s: invalid args\n", label);
        break;
    default:
        fprintf( stderr, "%s: error %d (%s)\n", label, err, strerror(err) );
        break;
    }

    return 1;
}


// #pragma mark - drmCommand

drm_public int drmCommandNone(int fd, unsigned long drmCommandIndex)
{
    unsigned long request;

    request = DRM_IO( DRM_COMMAND_BASE + drmCommandIndex);

    if (drmIoctl(fd, request, NULL)) {
        return errno;
    }
    return 0;
}

drm_public int drmCommandRead(int fd, unsigned long drmCommandIndex,
                              void *data, unsigned long size)
{
    unsigned long request;

    request = DRM_IOC( DRM_IOC_READ, DRM_IOCTL_BASE,
        DRM_COMMAND_BASE + drmCommandIndex, size);

    if (drmIoctl(fd, request, data)) {
        return errno;
    }
    return 0;
}

drm_public int drmCommandWrite(int fd, unsigned long drmCommandIndex,
                               void *data, unsigned long size)
{
    unsigned long request;

    request = DRM_IOC( DRM_IOC_WRITE, DRM_IOCTL_BASE,
        DRM_COMMAND_BASE + drmCommandIndex, size);

    if (drmIoctl(fd, request, data)) {
        return errno;
    }
    return 0;
}

drm_public int drmCommandWriteRead(int fd, unsigned long drmCommandIndex,
                                   void *data, unsigned long size)
{
    unsigned long request;

    request = DRM_IOC( DRM_IOC_READ|DRM_IOC_WRITE, DRM_IOCTL_BASE,
        DRM_COMMAND_BASE + drmCommandIndex, size);

    if (drmIoctl(fd, request, data))
        return errno;
    return 0;
}


// #pragma mark - version

static void drmFreeKernelVersion(drm_version_t *v)
{
    if (!v)
        return;
    drmFree(v->name);
    drmFree(v->date);
    drmFree(v->desc);
    drmFree(v);
}

static void drmCopyVersion(drmVersionPtr d, const drm_version_t *s)
{
    d->version_major      = s->version_major;
    d->version_minor      = s->version_minor;
    d->version_patchlevel = s->version_patchlevel;
    d->name_len           = s->name_len;
    d->name               = strdup(s->name);
    d->date_len           = s->date_len;
    d->date               = strdup(s->date);
    d->desc_len           = s->desc_len;
    d->desc               = strdup(s->desc);
}

drm_public drmVersionPtr drmGetVersion(int fd)
{
    drmVersionPtr retval;
    drm_version_t *version = drmMalloc(sizeof(*version));

    if (drmIoctl(fd, DRM_IOCTL_VERSION, version)) {
        drmFreeKernelVersion(version);
        return NULL;
    }

    if (version->name_len)
        version->name    = drmMalloc(version->name_len + 1);
    if (version->date_len)
        version->date    = drmMalloc(version->date_len + 1);
    if (version->desc_len)
        version->desc    = drmMalloc(version->desc_len + 1);

    if (drmIoctl(fd, DRM_IOCTL_VERSION, version)) {
        drmMsg("DRM_IOCTL_VERSION: %s\n", strerror(errno));
        drmFreeKernelVersion(version);
        return NULL;
    }

    /* The results might not be null-terminated strings, so terminate them. */
    if (version->name_len) version->name[version->name_len] = '\0';
    if (version->date_len) version->date[version->date_len] = '\0';
    if (version->desc_len) version->desc[version->desc_len] = '\0';

    retval = drmMalloc(sizeof(*retval));
    drmCopyVersion(retval, version);
    drmFreeKernelVersion(version);
    return retval;
}

drm_public void drmFreeVersion(drmVersionPtr v)
{
    if (!v)
        return;
    drmFree(v->name);
    drmFree(v->date);
    drmFree(v->desc);
    drmFree(v);
}

drm_public int drmGetCap(int fd, uint64_t capability, uint64_t *value)
{
    struct drm_get_cap cap;
    int ret;

    memclear(cap);
    cap.capability = capability;

    ret = drmIoctl(fd, DRM_IOCTL_GET_CAP, &cap);
    if (ret)
        return ret;

    *value = cap.value;
    return 0;
}

drm_public int drmSetClientCap(int fd, uint64_t capability, uint64_t value)
{
    struct drm_set_client_cap cap;

    memclear(cap);
    cap.capability = capability;
    cap.value = value;

    return drmIoctl(fd, DRM_IOCTL_SET_CLIENT_CAP, &cap);
}


// #pragma mark syncobj

drm_public int drmSyncobjCreate(int fd, uint32_t flags, uint32_t *handle)
{
    struct drm_syncobj_create args;
    int ret;

    memclear(args);
    args.flags = flags;
    args.handle = 0;
    ret = drmIoctl(fd, DRM_IOCTL_SYNCOBJ_CREATE, &args);
    if (ret)
        return ret;
    *handle = args.handle;
    return 0;
}

drm_public int drmSyncobjDestroy(int fd, uint32_t handle)
{
    struct drm_syncobj_destroy args;

    memclear(args);
    args.handle = handle;
    return drmIoctl(fd, DRM_IOCTL_SYNCOBJ_DESTROY, &args);
}

drm_public int drmSyncobjHandleToFD(int fd, uint32_t handle, int *obj_fd)
{
    struct drm_syncobj_handle args;
    int ret;

    memclear(args);
    args.fd = -1;
    args.handle = handle;
    ret = drmIoctl(fd, DRM_IOCTL_SYNCOBJ_HANDLE_TO_FD, &args);
    if (ret)
        return ret;
    *obj_fd = args.fd;
    return 0;
}

drm_public int drmSyncobjFDToHandle(int fd, int obj_fd, uint32_t *handle)
{
    struct drm_syncobj_handle args;
    int ret;

    memclear(args);
    args.fd = obj_fd;
    args.handle = 0;
    ret = drmIoctl(fd, DRM_IOCTL_SYNCOBJ_FD_TO_HANDLE, &args);
    if (ret)
        return ret;
    *handle = args.handle;
    return 0;
}

drm_public int drmSyncobjImportSyncFile(int fd, uint32_t handle,
                                        int sync_file_fd)
{
    struct drm_syncobj_handle args;

    memclear(args);
    args.fd = sync_file_fd;
    args.handle = handle;
    args.flags = DRM_SYNCOBJ_FD_TO_HANDLE_FLAGS_IMPORT_SYNC_FILE;
    return drmIoctl(fd, DRM_IOCTL_SYNCOBJ_FD_TO_HANDLE, &args);
}

drm_public int drmSyncobjExportSyncFile(int fd, uint32_t handle,
                                        int *sync_file_fd)
{
    struct drm_syncobj_handle args;
    int ret;

    memclear(args);
    args.fd = -1;
    args.handle = handle;
    args.flags = DRM_SYNCOBJ_HANDLE_TO_FD_FLAGS_EXPORT_SYNC_FILE;
    ret = drmIoctl(fd, DRM_IOCTL_SYNCOBJ_HANDLE_TO_FD, &args);
    if (ret)
        return ret;
    *sync_file_fd = args.fd;
    return 0;
}

drm_public int drmSyncobjWait(int fd, uint32_t *handles, unsigned num_handles,
                              int64_t timeout_nsec, unsigned flags,
                              uint32_t *first_signaled)
{
    struct drm_syncobj_wait args;
    int ret;

    memclear(args);
    args.handles = (uintptr_t)handles;
    args.timeout_nsec = timeout_nsec;
    args.count_handles = num_handles;
    args.flags = flags;

    ret = drmIoctl(fd, DRM_IOCTL_SYNCOBJ_WAIT, &args);
    if (ret < 0)
        return errno;

    if (first_signaled)
        *first_signaled = args.first_signaled;
    return ret;
}

drm_public int drmSyncobjReset(int fd, const uint32_t *handles,
                               uint32_t handle_count)
{
    struct drm_syncobj_array args;
    int ret;

    memclear(args);
    args.handles = (uintptr_t)handles;
    args.count_handles = handle_count;

    ret = drmIoctl(fd, DRM_IOCTL_SYNCOBJ_RESET, &args);
    return ret;
}

drm_public int drmSyncobjSignal(int fd, const uint32_t *handles,
                                uint32_t handle_count)
{
    struct drm_syncobj_array args;
    int ret;

    memclear(args);
    args.handles = (uintptr_t)handles;
    args.count_handles = handle_count;

    ret = drmIoctl(fd, DRM_IOCTL_SYNCOBJ_SIGNAL, &args);
    return ret;
}

drm_public int drmSyncobjTimelineSignal(int fd, const uint32_t *handles,
					uint64_t *points, uint32_t handle_count)
{
    struct drm_syncobj_timeline_array args;
    int ret;

    memclear(args);
    args.handles = (uintptr_t)handles;
    args.points = (uintptr_t)points;
    args.count_handles = handle_count;

    ret = drmIoctl(fd, DRM_IOCTL_SYNCOBJ_TIMELINE_SIGNAL, &args);
    return ret;
}

drm_public int drmSyncobjTimelineWait(int fd, uint32_t *handles, uint64_t *points,
				      unsigned num_handles,
				      int64_t timeout_nsec, unsigned flags,
				      uint32_t *first_signaled)
{
    struct drm_syncobj_timeline_wait args;
    int ret;

    memclear(args);
    args.handles = (uintptr_t)handles;
    args.points = (uintptr_t)points;
    args.timeout_nsec = timeout_nsec;
    args.count_handles = num_handles;
    args.flags = flags;

    ret = drmIoctl(fd, DRM_IOCTL_SYNCOBJ_TIMELINE_WAIT, &args);
    if (ret < 0)
        return errno;

    if (first_signaled)
        *first_signaled = args.first_signaled;
    return ret;
}


drm_public int drmSyncobjQuery(int fd, uint32_t *handles, uint64_t *points,
			       uint32_t handle_count)
{
    struct drm_syncobj_timeline_array args;
    int ret;

    memclear(args);
    args.handles = (uintptr_t)handles;
    args.points = (uintptr_t)points;
    args.count_handles = handle_count;

    ret = drmIoctl(fd, DRM_IOCTL_SYNCOBJ_QUERY, &args);
    if (ret)
        return ret;
    return 0;
}

drm_public int drmSyncobjQuery2(int fd, uint32_t *handles, uint64_t *points,
				uint32_t handle_count, uint32_t flags)
{
    struct drm_syncobj_timeline_array args;

    memclear(args);
    args.handles = (uintptr_t)handles;
    args.points = (uintptr_t)points;
    args.count_handles = handle_count;
    args.flags = flags;

    return drmIoctl(fd, DRM_IOCTL_SYNCOBJ_QUERY, &args);
}

drm_public int drmSyncobjTransfer(int fd,
				  uint32_t dst_handle, uint64_t dst_point,
				  uint32_t src_handle, uint64_t src_point,
				  uint32_t flags)
{
    struct drm_syncobj_transfer args;
    int ret;

    memclear(args);
    args.src_handle = src_handle;
    args.dst_handle = dst_handle;
    args.src_point = src_point;
    args.dst_point = dst_point;
    args.flags = flags;

    ret = drmIoctl(fd, DRM_IOCTL_SYNCOBJ_TRANSFER, &args);

    return ret;
}

drm_public int drmSyncobjAccumulate(int fd,
			      uint32_t syncobj1, uint32_t syncobj2,
			      uint64_t point)
{
    struct drm_syncobj_accumulate args;
    int ret;

    memclear(args);
    args.syncobj1 = syncobj1;
    args.syncobj2 = syncobj2;
    args.point = point;

    ret = drmIoctl(fd, DRM_IOCTL_SYNCOBJ_ACCUMULATE, &args);

    return ret;
}


// #pragma mark -

drm_public int drmPrimeHandleToFD(int fd, uint32_t handle, uint32_t flags,
                                  int *prime_fd)
{
    struct drm_prime_handle args;
    int ret;

    memclear(args);
    args.fd = -1;
    args.handle = handle;
    args.flags = flags;
    ret = drmIoctl(fd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &args);
    if (ret)
        return ret;

    *prime_fd = args.fd;
    return 0;
}

drm_public int drmPrimeFDToHandle(int fd, int prime_fd, uint32_t *handle)
{
    struct drm_prime_handle args;
    int ret;

    memclear(args);
    args.fd = prime_fd;
    ret = drmIoctl(fd, DRM_IOCTL_PRIME_FD_TO_HANDLE, &args);
    if (ret)
        return ret;

    *handle = args.handle;
    return 0;
}


// #pragma mark -

drm_public int drmCloseBufferHandle(int fd, uint32_t handle)
{
    struct drm_gem_close args;

    memclear(args);
    args.handle = handle;
    return drmIoctl(fd, DRM_IOCTL_GEM_CLOSE, &args);
}

drm_public int drmDevicesEqual(drmDevicePtr a, drmDevicePtr b)
{
    if (a == NULL || b == NULL)
        return 0;

    if (a->bustype != b->bustype)
        return 0;

    switch (a->bustype) {
    case DRM_BUS_PCI:
        return memcmp(a->businfo.pci, b->businfo.pci, sizeof(drmPciBusInfo)) == 0;

    case DRM_BUS_USB:
        return memcmp(a->businfo.usb, b->businfo.usb, sizeof(drmUsbBusInfo)) == 0;

    case DRM_BUS_PLATFORM:
        return memcmp(a->businfo.platform, b->businfo.platform, sizeof(drmPlatformBusInfo)) == 0;

    case DRM_BUS_HOST1X:
        return memcmp(a->businfo.host1x, b->businfo.host1x, sizeof(drmHost1xBusInfo)) == 0;

    default:
        break;
    }

    return 0;
}


drm_public int drmAuthMagic(int fd, drm_magic_t magic)
{
    drm_auth_t auth;

    memclear(auth);
    auth.magic = magic;
    if (drmIoctl(fd, DRM_IOCTL_AUTH_MAGIC, &auth))
        return -errno;
    return 0;
}
