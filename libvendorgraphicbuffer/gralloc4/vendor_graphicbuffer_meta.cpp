/*
 * Copyright (C) 2020 Samsung Electronics Co. Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "VendorGraphicBuffer"

#include <log/log.h>
#include "VendorGraphicBuffer.h"
#include "mali_gralloc_buffer.h"
#include "mali_gralloc_formats.h"
#include "hidl_common/SharedMetadata.h"
#include "exynos_format.h"

using namespace android;
using namespace vendor::graphics;

using arm::mapper::common::shared_metadata;
using aidl::android::hardware::graphics::common::Dataspace;

#define UNUSED(x) ((void)x)
#define SZ_4k 0x1000

int VendorGraphicBufferMeta::get_video_metadata_fd(buffer_handle_t hnd)
{
	const private_handle_t *gralloc_hnd = static_cast<const private_handle_t *>(hnd);

	if (!gralloc_hnd)
		return -EINVAL;

	uint64_t usage = gralloc_hnd->producer_usage | gralloc_hnd->consumer_usage;

	if (usage & VendorGraphicBufferUsage::VIDEO_PRIVATE_DATA)
		return gralloc_hnd->get_share_attr_fd();
	else
		return -EINVAL;
}

int VendorGraphicBufferMeta::get_dataspace(buffer_handle_t hnd)
{
	const private_handle_t *gralloc_hnd = static_cast<const private_handle_t *>(hnd);

	if (!gralloc_hnd)
		return -1;

	int attr_fd = gralloc_hnd->get_share_attr_fd();

	if(attr_fd < 0)
		return -1;

	shared_metadata *metadata = (shared_metadata *)mmap(0, sizeof(shared_metadata), PROT_READ, MAP_SHARED, attr_fd, 0);
	std::optional<Dataspace> dataspace = metadata->dataspace.to_std_optional();

	int32_t ret = static_cast<int32_t>(dataspace.value_or(Dataspace::UNKNOWN));

	munmap(metadata, sizeof(shared_metadata));

	return ret;
}

int VendorGraphicBufferMeta::set_dataspace(buffer_handle_t hnd, android_dataspace_t dataspace)
{
	const private_handle_t *gralloc_hnd = static_cast<const private_handle_t *>(hnd);

	if (!gralloc_hnd)
		return -1;

	arm::mapper::common::set_dataspace(gralloc_hnd, static_cast<Dataspace>(dataspace));

	return 0;
}

int VendorGraphicBufferMeta::is_afbc(buffer_handle_t hnd)
{
	const private_handle_t *gralloc_hnd = static_cast<const private_handle_t *>(hnd);

	if (!gralloc_hnd)
		return 0;

	if (gralloc_hnd->alloc_format & MALI_GRALLOC_INTFMT_AFBCENABLE_MASK)
		return 1;

	return 0;
}

int VendorGraphicBufferMeta::is_sbwc(buffer_handle_t buffer_hnd_p)
{
	const private_handle_t *hnd = static_cast<const private_handle_t *>(buffer_hnd_p);

    return is_sbwc_format(static_cast<uint32_t>(hnd->alloc_format & MALI_GRALLOC_INTFMT_FMT_MASK));
}

#define GRALLOC_META_GETTER(__type__, __name__, __member__) \
__type__ VendorGraphicBufferMeta::get_##__name__(buffer_handle_t hnd) \
{ \
	const private_handle_t *gralloc_hnd = static_cast<const private_handle_t *>(hnd); \
	if (!gralloc_hnd) return 0; \
	return gralloc_hnd->__member__; \
} \


uint32_t VendorGraphicBufferMeta::get_format(buffer_handle_t hnd) 
{
	const private_handle_t *gralloc_hnd = static_cast<const private_handle_t *>(hnd);
	if (!gralloc_hnd)
		return 0;

	return static_cast<uint32_t>(gralloc_hnd->alloc_format);
}

uint64_t VendorGraphicBufferMeta::get_internal_format(buffer_handle_t hnd)
{
	const private_handle_t *gralloc_hnd = static_cast<const private_handle_t *>(hnd);
	if (!gralloc_hnd)
		return 0;

	return static_cast<uint64_t>(gralloc_hnd->alloc_format & MALI_GRALLOC_INTFMT_FMT_MASK);
}

GRALLOC_META_GETTER(uint64_t, frameworkFormat, req_format);

GRALLOC_META_GETTER(int, width, width);
GRALLOC_META_GETTER(int, height, height);
GRALLOC_META_GETTER(uint32_t, stride, stride);
GRALLOC_META_GETTER(uint32_t, vstride, plane_info[0].alloc_height);

GRALLOC_META_GETTER(uint64_t, producer_usage, producer_usage);
GRALLOC_META_GETTER(uint64_t, consumer_usage, consumer_usage);

GRALLOC_META_GETTER(uint64_t, flags, flags);


int VendorGraphicBufferMeta::get_fd(buffer_handle_t hnd, int num)
{
	const private_handle_t *gralloc_hnd = static_cast<const private_handle_t *>(hnd);

	if (!gralloc_hnd)
		return -1;

	if (num > 2)
		return -1;

	return gralloc_hnd->fds[num];
}

int VendorGraphicBufferMeta::get_size(buffer_handle_t hnd, int num)
{
	const private_handle_t *gralloc_hnd = static_cast<const private_handle_t *>(hnd);

	if (!gralloc_hnd)
		return 0;

	if (num > 2)
		return 0;

	return gralloc_hnd->alloc_sizes[num];
}


uint64_t VendorGraphicBufferMeta::get_usage(buffer_handle_t hnd)
{
	const private_handle_t *gralloc_hnd = static_cast<const private_handle_t *>(hnd);

	if (!gralloc_hnd)
		return 0;

	return gralloc_hnd->producer_usage | gralloc_hnd->consumer_usage;
}

void* VendorGraphicBufferMeta::get_video_metadata(buffer_handle_t hnd)
{
	private_handle_t *gralloc_hnd =
		static_cast<private_handle_t *>(const_cast<native_handle_t *>(hnd));

	if (!gralloc_hnd)
		return nullptr;

	return gralloc_hnd->attr_base;
}

void* VendorGraphicBufferMeta::get_video_metadata_roiinfo(buffer_handle_t hnd)
{
	private_handle_t *gralloc_hnd =
		static_cast<private_handle_t *>(const_cast<native_handle_t *>(hnd));

	if (!gralloc_hnd)
		return nullptr;

	if (gralloc_hnd->get_usage() & VendorGraphicBufferUsage::ROIINFO)
		return static_cast<char*>(gralloc_hnd->attr_base) + SZ_4k * 2;

	return nullptr;
}

void VendorGraphicBufferMeta::init(const buffer_handle_t handle)
{
	const private_handle_t *gralloc_hnd = static_cast<const private_handle_t *>(handle);

	if (!gralloc_hnd)
		return;

	fd = gralloc_hnd->fds[0];
	fd1 = gralloc_hnd->fds[1];
	fd2 = gralloc_hnd->fds[2];

	size   = gralloc_hnd->alloc_sizes[0];
	size1  = gralloc_hnd->alloc_sizes[1];
	size2  = gralloc_hnd->alloc_sizes[2];

	offsets[0] = gralloc_hnd->plane_info[0].offset;
	offsets[1] = gralloc_hnd->plane_info[1].offset;
	offsets[2] = gralloc_hnd->plane_info[2].offset;

	uint64_t usage = gralloc_hnd->producer_usage | gralloc_hnd->consumer_usage;
	if (usage & VendorGraphicBufferUsage::VIDEO_PRIVATE_DATA) {
		switch (gralloc_hnd->get_share_attr_fd_index()) {
		case 1:
			size1 = gralloc_hnd->attr_size;
			break;
		case 2:
			size2 = gralloc_hnd->attr_size;
			break;
		}
	}

	internal_format = gralloc_hnd->alloc_format;
	frameworkFormat = gralloc_hnd->req_format;

	width  = gralloc_hnd->width;
	height = gralloc_hnd->height;
	stride = gralloc_hnd->stride;
	vstride = gralloc_hnd->plane_info[0].alloc_height;

	producer_usage = gralloc_hnd->producer_usage;
	consumer_usage = gralloc_hnd->consumer_usage;

	flags = gralloc_hnd->flags;

	unique_id = gralloc_hnd->backing_store_id;
}

VendorGraphicBufferMeta::VendorGraphicBufferMeta(buffer_handle_t handle)
{
	init(handle);
}

