#include <AccelerantRoster.h>

extern "C" {
#include "xf86drm.h"
#include "amdgpu_drm.h"
#include "amdgpu_internal.h"
}

#define CheckRet(err) {status_t _err = (err); if (_err < B_OK) return _err;}


drm_public int amdgpu_device_initialize(int fd,
					uint32_t *major_version,
					uint32_t *minor_version,
					amdgpu_device_handle *device_handle)
{
	Accelerant *acc{};
	CheckRet(AccelerantRoster::Instance().FromFd(acc, fd));
	BReference<Accelerant> accDeleter(acc, true);
	accelerant_base *acc_base = (accelerant_base*)static_cast<AccelerantBase*>(acc);

	return amdgpu_device_initialize_haiku(acc_base, major_version, minor_version, device_handle);
}
