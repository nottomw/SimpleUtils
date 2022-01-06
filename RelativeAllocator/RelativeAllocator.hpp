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

    RelativeAllocator(uint8_t *const allocatorInternalSharedMem,     //
                      const uint32_t allocatorInternalSharedMemSize, //
                      const uint32_t memoryRegionSize);

    RelativePtr alloc(const uint32_t sizeBytes);
    void dealloc(const RelativePtr ptr);

  private:
    struct MemoryRegion
    {
        MemoryRegion()
            : ptr(0), //
              size(0)

        {
        }

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

    void tryToCoalesce(const MemoryRegion &freedMemRegion);

    MemoryRegion &arrEmplace(MemoryRegion *const arr,                  //
                             uint32_t *arrTail,                        //
                             const RelativeAllocator::RelativePtr ptr, //
                             const uint32_t size);

    void arrErase(MemoryRegion *const arr, //
                  uint32_t &arrTail,       //
                  const MemoryRegion &region);

    MemoryRegion *arrFindPtr(MemoryRegion *const arr, //
                             uint32_t &arrTail,       //
                             const RelativePtr &ptr);

    MemoryRegion *mRegionsFree; // raw array
    uint32_t *mRegionsFreeTail;

    MemoryRegion *mRegionsUsed; // raw array
    uint32_t *mRegionsUsedTail;

    uint8_t *const mSharedMem;
    uint32_t mMemoryRegionSize;
};

} // namespace kiss

#endif // RELATIVEALLOCATOR_HPP
