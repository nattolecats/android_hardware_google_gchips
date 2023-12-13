#include "GrallocAllocator.h"

#include <aidl/android/hardware/graphics/allocator/AllocationError.h>
#include <aidlcommonsupport/NativeHandle.h>
#include <android/binder_ibinder.h>
#include <android/binder_status.h>
#include <cutils/android_filesystem_config.h>
#include <hidl/HidlSupport.h>

#include "hidl_common/Allocator.h"

namespace pixel::allocator {

namespace AidlAllocator = aidl::android::hardware::graphics::allocator;
namespace HidlAllocator = android::hardware::graphics::allocator::V4_0;

using android::hardware::hidl_handle;
using android::hardware::hidl_vec;
using HidlError = android::hardware::graphics::mapper::V4_0::Error;

unsigned long callingPid() {
    return static_cast<unsigned long>(AIBinder_getCallingPid());
}

unsigned long callingUid() {
    return static_cast<unsigned long>(AIBinder_getCallingUid());
}

GrallocAllocator::GrallocAllocator() {}

GrallocAllocator::~GrallocAllocator() {}

ndk::ScopedAStatus GrallocAllocator::allocate(const std::vector<uint8_t>& descriptor, int32_t count,
                                              AidlAllocator::AllocationResult* result) {
    MALI_GRALLOC_LOGV("Allocation request from process: %lu", callingPid());

    buffer_descriptor_t bufferDescriptor;
    if (!arm::mapper::common::grallocDecodeBufferDescriptor(hidl_vec(descriptor),
                                                            bufferDescriptor)) {
        return ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int32_t>(AidlAllocator::AllocationError::BAD_DESCRIPTOR));
    }

    // TODO(layog@): This dependency between AIDL and HIDL backends is not good.
    // Ideally common::allocate should return the result and it should be encoded
    // by this interface into HIDL or AIDL.
    HidlError error = HidlError::NONE;
    auto hidl_cb = [&](HidlError _error, int _stride, hidl_vec<hidl_handle> _buffers) {
        if (_error != HidlError::NONE) {
            error = _error;
            return;
        }

        const uint32_t size = _buffers.size();

        result->stride = _stride;
        result->buffers.resize(size);
        for (uint32_t i = 0; i < size; i++) {
            // Dup here is necessary. After this callback returns common::allocate
            // will free the buffer which will destroy the older fd.
            result->buffers[i] = android::dupToAidl(static_cast<const native_handle*>(_buffers[i]));
        }
    };

    arm::allocator::common::allocate(bufferDescriptor, count, hidl_cb);

    switch (error) {
        case HidlError::NONE:
            break;

        case HidlError::BAD_DESCRIPTOR:
            return ndk::ScopedAStatus::fromServiceSpecificError(
                    static_cast<int32_t>(AidlAllocator::AllocationError::BAD_DESCRIPTOR));

        case HidlError::NO_RESOURCES:
            return ndk::ScopedAStatus::fromServiceSpecificError(
                    static_cast<int32_t>(AidlAllocator::AllocationError::NO_RESOURCES));

        case HidlError::UNSUPPORTED:
            return ndk::ScopedAStatus::fromServiceSpecificError(
                    static_cast<int32_t>(AidlAllocator::AllocationError::UNSUPPORTED));

        default:
            return ndk::ScopedAStatus::fromStatus(STATUS_UNKNOWN_ERROR);
    }

    return ndk::ScopedAStatus::ok();
}

buffer_descriptor_t decodeBufferDescriptorInfo(
        const AidlAllocator::BufferDescriptorInfo& descriptor) {
    buffer_descriptor_t bufferDescriptor;
    bufferDescriptor.width = descriptor.width;
    bufferDescriptor.height = descriptor.height;
    bufferDescriptor.layer_count = descriptor.layerCount;
    bufferDescriptor.hal_format = static_cast<uint64_t>(descriptor.format);
    bufferDescriptor.producer_usage = static_cast<uint64_t>(descriptor.usage);
    bufferDescriptor.consumer_usage = bufferDescriptor.producer_usage;
    bufferDescriptor.format_type = MALI_GRALLOC_FORMAT_TYPE_USAGE;
    bufferDescriptor.signature = sizeof(buffer_descriptor_t);
    bufferDescriptor.reserved_size = descriptor.reservedSize;
    const char *str = (const char*) descriptor.name.data();
    bufferDescriptor.name = std::string(str);
    return bufferDescriptor;
}

ndk::ScopedAStatus GrallocAllocator::allocate2(
        const AidlAllocator::BufferDescriptorInfo& descriptor, int32_t count,
        AidlAllocator::AllocationResult* result) {
    MALI_GRALLOC_LOGV("Allocation request from process: %lu", callingPid());

    buffer_descriptor_t bufferDescriptor = decodeBufferDescriptorInfo(descriptor);

    HidlError error = HidlError::NONE;
    auto hidl_cb = [&](HidlError _error, int _stride, hidl_vec<hidl_handle> _buffers) {
        if (_error != HidlError::NONE) {
            error = _error;
            return;
        }

        const uint32_t size = _buffers.size();

        result->stride = _stride;
        result->buffers.resize(size);
        for (uint32_t i = 0; i < size; i++) {
            // Dup here is necessary. After this callback returns common::allocate
            // will free the buffer which will destroy the older fd.
            result->buffers[i] = android::dupToAidl(static_cast<const native_handle*>(_buffers[i]));
        }
    };

    arm::allocator::common::allocate(bufferDescriptor, count, hidl_cb);

    switch (error) {
        case HidlError::NONE:
            break;

        case HidlError::BAD_DESCRIPTOR:
            return ndk::ScopedAStatus::fromServiceSpecificError(
                    static_cast<int32_t>(AidlAllocator::AllocationError::BAD_DESCRIPTOR));

        case HidlError::NO_RESOURCES:
            return ndk::ScopedAStatus::fromServiceSpecificError(
                    static_cast<int32_t>(AidlAllocator::AllocationError::NO_RESOURCES));

        case HidlError::UNSUPPORTED:
            return ndk::ScopedAStatus::fromServiceSpecificError(
                    static_cast<int32_t>(AidlAllocator::AllocationError::UNSUPPORTED));

        default:
            return ndk::ScopedAStatus::fromStatus(STATUS_UNKNOWN_ERROR);
    }

    return ndk::ScopedAStatus::ok();
}

// TODO(b/315883761): isSupported should return false for unknown-to-HAL usage
ndk::ScopedAStatus GrallocAllocator::isSupported(
        const AidlAllocator::BufferDescriptorInfo& descriptor, bool* result) {
    buffer_descriptor_t bufferDescriptor = decodeBufferDescriptorInfo(descriptor);

    int isBufferDescriptorSupported = arm::allocator::common::isSupported(&bufferDescriptor);
    *result = (isBufferDescriptorSupported == 0);

    if (isBufferDescriptorSupported) {
        MALI_GRALLOC_LOGV("Allocation for the given description will not succeed. error %d",
                          isBufferDescriptorSupported);
        return ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int32_t>(AidlAllocator::AllocationError::UNSUPPORTED));
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus GrallocAllocator::getIMapperLibrarySuffix(std::string* result) {
    *result = "";
    return ndk::ScopedAStatus::fromStatus(STATUS_UNKNOWN_ERROR);
}

binder_status_t GrallocAllocator::dump(int fd, const char** /* args */, uint32_t numArgs) {
    if (callingUid() != AID_ROOT) {
        const std::string permission_denied = "Permission Denied\n";
        write(fd, permission_denied.c_str(), permission_denied.size());
        return STATUS_PERMISSION_DENIED;
    }

    if (numArgs != 0) {
        const std::string argument_error = "No argument expected\n";
        write(fd, argument_error.c_str(), argument_error.size());
        return STATUS_BAD_VALUE;
    }

    const std::string dump_info = arm::allocator::common::dump();
    write(fd, dump_info.c_str(), dump_info.size());
    return STATUS_OK;
}

} // namespace pixel::allocator
