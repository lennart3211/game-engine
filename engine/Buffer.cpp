#include "Buffer.h"

#include <cassert>
#include <cstring>

namespace engine {

/**
 * Returns the minimum instance_ size required to be compatible with devices minOffsetAlignment
 *
 * @param instanceSize The size of an instance_
 * @param minOffsetAlignment The minimum required alignment, in bytes, for the offset member (eg
 * minUniformBufferOffsetAlignment)
 *
 * @return VkResult of the buffer mapping call
 */
    VkDeviceSize Buffer::getAlignment(VkDeviceSize instanceSize, VkDeviceSize minOffsetAlignment) {
        if (minOffsetAlignment > 0) {
            return (instanceSize + minOffsetAlignment - 1) & ~(minOffsetAlignment - 1);
        }
        return instanceSize;
    }

    Buffer::Buffer(
            Device &device,
            VkDeviceSize instanceSize,
            uint32_t instanceCount,
            VkBufferUsageFlags usageFlags,
            VkMemoryPropertyFlags memoryPropertyFlags,
            VkDeviceSize minOffsetAlignment)
            : _device{device}, _instanceSize{instanceSize},
          _instanceCount{instanceCount}, _usageFlags{usageFlags},
          _memoryPropertyFlags{memoryPropertyFlags} {
        _alignmentSize = getAlignment(instanceSize, minOffsetAlignment);
        _bufferSize = _alignmentSize * instanceCount;
        device.createBuffer(_bufferSize, usageFlags, memoryPropertyFlags,
                            _buffer, _memory);
    }

    Buffer::~Buffer() {
        unmap();
        vkDeviceWaitIdle(_device.device());
        vkDestroyBuffer(_device.device(), _buffer, nullptr);
        vkFreeMemory(_device.device(), _memory, nullptr);
    }

/**
 * Map a memory range of this buffer. If successful, mapped points to the specified buffer range.
 *
 * @param size (Optional) Size of the memory range to map. Pass VK_WHOLE_SIZE to map the complete
 * buffer range.
 * @param offset (Optional) Byte offset from beginning
 *
 * @return VkResult of the buffer mapping call
 */
    VkResult Buffer::map(VkDeviceSize size, VkDeviceSize offset) {
        assert(_buffer && _memory && "Called map on buffer before create");
        return vkMapMemory(_device.device(), _memory, offset, size, 0, &_mapped);
    }

/**
 * Unmap a mapped memory range
 *
 * @note Does not return a result as vkUnmapMemory cant fail
 * @note Does not return a result as vkUnmapMemory cant fail
 */
    void Buffer::unmap() {
        if (_mapped) {
            vkUnmapMemory(_device.device(), _memory);
            _mapped = nullptr;
        }
    }

/**
 * Copies the specified data to the mapped buffer. Default value writes whole buffer range
 *
 * @param data Pointer to the data to copy
 * @param size (Optional) Size of the data to copy. Pass VK_WHOLE_SIZE to flush the complete buffer
 * range.
 * @param offset (Optional) Byte offset from beginning of mapped region
 *
 */
    void Buffer::writeToBuffer(void *data, VkDeviceSize size, VkDeviceSize offset) {
        assert(_mapped && "Cannot copy to unmapped buffer");

        if (size == (VkDeviceSize) VK_WHOLE_SIZE) {
            memcpy(_mapped, data, _bufferSize);
        } else {
            char *memOffset = (char *)_mapped;
            memOffset += offset;
            memcpy(memOffset, data, size);
        }
    }

/**
 * Flush a memory range of the buffer to make it visible to the device
 *
 * @note Only required for non-coherent memory
 *
 * @param size (Optional) Size of the memory range to flush. Pass VK_WHOLE_SIZE to flush the
 * complete buffer range.
 * @param offset (Optional) Byte offset from beginning
 *
 * @return VkResult of the flush call
 */
    VkResult Buffer::flush(VkDeviceSize size, VkDeviceSize offset) {
        VkMappedMemoryRange mappedRange = {};
        mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        mappedRange.memory = _memory;
        mappedRange.offset = offset;
        mappedRange.size = size;
        return vkFlushMappedMemoryRanges(_device.device(), 1, &mappedRange);
    }

/**
 * Invalidate a memory range of the buffer to make it visible to the host
 *
 * @note Only required for non-coherent memory
 *
 * @param size (Optional) Size of the memory range to invalidate. Pass VK_WHOLE_SIZE to invalidate
 * the complete buffer range.
 * @param offset (Optional) Byte offset from beginning
 *
 * @return VkResult of the invalidate call
 */
    VkResult Buffer::invalidate(VkDeviceSize size, VkDeviceSize offset) {
        VkMappedMemoryRange mappedRange = {};
        mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        mappedRange.memory = _memory;
        mappedRange.offset = offset;
        mappedRange.size = size;
        return vkInvalidateMappedMemoryRanges(_device.device(), 1, &mappedRange);
    }

/**
 * Create a buffer info descriptor
 *
 * @param size (Optional) Size of the memory range of the descriptor
 * @param offset (Optional) Byte offset from beginning
 *
 * @return VkDescriptorBufferInfo of specified offset and range
 */
    VkDescriptorBufferInfo Buffer::descriptorInfo(VkDeviceSize size, VkDeviceSize offset) {
        return VkDescriptorBufferInfo{
            _buffer,
                offset,
                size,
        };
    }

/**
 * Copies "instanceSize" bytes of data to the mapped buffer at an offset of index * alignmentSize
 *
 * @param data Pointer to the data to copy
 * @param index Used in offset calculation
 *
 */
    void Buffer::writeToIndex(void *data, int index) {
        writeToBuffer(data, _instanceSize, index * _alignmentSize);
    }

/**
 *  Flush the memory range at index * alignmentSize of the buffer to make it visible to the device
 *
 * @param index Used in offset calculation
 *
 */
    VkResult Buffer::flushIndex(int index) { return flush(_alignmentSize, index * _alignmentSize); }

/**
 * Create a buffer info descriptor
 *
 * @param index Specifies the region given by index * alignmentSize
 *
 * @return VkDescriptorBufferInfo for instance_ at index
 */
    VkDescriptorBufferInfo Buffer::descriptorInfoForIndex(int index) {
        return descriptorInfo(_alignmentSize, index * _alignmentSize);
    }

/**
 * Invalidate a memory range of the buffer to make it visible to the host
 *
 * @note Only required for non-coherent memory
 *
 * @param index Specifies the region to invalidate: index * alignmentSize
 *
 * @return VkResult of the invalidate call
 */
    VkResult Buffer::invalidateIndex(int index) {
        return invalidate(_alignmentSize, index * _alignmentSize);
    }

}  // namespace engine