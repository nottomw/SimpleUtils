#ifndef RELATIVEALLOCATOR_HPP
#define RELATIVEALLOCATOR_HPP

#include <cstdint>
#include <set>

namespace kiss
{

class RelativeAllocator
{
  public:
    using RelativePtr = uint32_t;

    RelativeAllocator(uint8_t *const allocatorInternalSharedMem, const uint32_t memoryRegionSize);

    RelativePtr alloc(const uint32_t sizeBytes);
    void dealloc(const RelativePtr ptr);

  private:
    struct MemoryRegion
    {
        MemoryRegion(const RelativePtr ptr, const uint32_t size) //
            : ptr(ptr),                                          //
              size(size)
        {
        }

        MemoryRegion(const MemoryRegion &region)
            : ptr(region.ptr), //
              size(region.size)
        {
        }

        RelativePtr ptr;
        uint32_t size;
    };

    struct MemoryRegionCompare
    {
        // use the stdlib set is_transparent feature
        using is_transparent = void;

        bool isLess(const uint32_t left, const uint32_t leftSize, const uint32_t right) const
        {
            uint32_t size = leftSize;
            if (size != 0)
            {
                size -= 1;
            }

            return ((left + size) < right);
        }

        bool operator()(const MemoryRegion &left, const MemoryRegion &right) const
        {
            return isLess(left.ptr, left.size, right.ptr);
        }

        bool operator()(const MemoryRegion &left, const uint32_t relativePtr) const
        {
            return isLess(left.ptr, left.size, relativePtr);
        }

        bool operator()(const uint32_t relativePtr, const MemoryRegion &right) const
        {
            return isLess(relativePtr, 0, right.ptr);
        }
    };

    void tryToCoalesce(const MemoryRegion &freedMemRegion);

    std::set<MemoryRegion, MemoryRegionCompare> mRegionsFree;
    std::set<MemoryRegion, MemoryRegionCompare> mRegionsUsed;

    uint8_t *const mSharedMem;
    uint32_t mMemoryRegionSize;
};

} // namespace kiss

#endif // RELATIVEALLOCATOR_HPP
