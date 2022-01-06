#include "RelativeAllocator.hpp"

#include <cassert>
#include <cstdio>

namespace kiss
{

RelativeAllocator::RelativeAllocator(uint8_t *const allocatorInternalSharedMem, //
                                     const uint32_t memoryRegionSize)
    : mSharedMem(allocatorInternalSharedMem), //
      mMemoryRegionSize(memoryRegionSize)
{
    // place the memory bookkeeping structures in shared memory

    mRegionsFree.emplace(0, memoryRegionSize);
}

RelativeAllocator::RelativePtr RelativeAllocator::alloc(const uint32_t sizeBytes)
{
    bool allocSuccess = false;
    RelativePtr allocAddr = 0;

    for (auto &it : mRegionsFree)
    {
        if (it.size > sizeBytes)
        {
            printf("ALLOC: new ptr = %d, size = \n", it.ptr, sizeBytes);

            mRegionsUsed.emplace(it.ptr, sizeBytes);
            allocAddr = it.ptr;

            const uint32_t newFreeRegionSize = it.size - sizeBytes;
            if (newFreeRegionSize != 0)
            {
                mRegionsFree.emplace((it.ptr + sizeBytes), newFreeRegionSize);
            }

            mRegionsFree.erase(it);

            allocSuccess = true;

            break;
        }
    }

    // TODO: return error, assert for now...
    assert(allocSuccess);

    printf("Allocated: %d\n", allocAddr);

    return allocAddr;
}

void RelativeAllocator::dealloc(const RelativeAllocator::RelativePtr ptr)
{
    printf("Dealloc: %d\n", ptr);

    auto it = mRegionsUsed.find(ptr);
    if (it != mRegionsUsed.end())
    {
        mRegionsUsed.erase(it);
        auto newMemRegion = mRegionsFree.emplace(it->ptr, it->size).first;

        tryToCoalesce(*newMemRegion);
    }
    else
    {
        assert("tried to free invalid ptr" == nullptr);
    }
}

void RelativeAllocator::tryToCoalesce(const MemoryRegion &freedMemRegion)
{
    // if possible coalesce the freed MemRegion with adjacent free memory

    MemoryRegion memRegionToCoalesce{freedMemRegion};

    // find free regions before
    if (memRegionToCoalesce.ptr != 0)
    {
        auto freeRegionBefore = mRegionsFree.find(memRegionToCoalesce.ptr - 1U);
        if (freeRegionBefore != mRegionsFree.end())
        {
            mRegionsFree.erase(freeRegionBefore);

            const uint32_t newFreeRegionSize = freeRegionBefore->size + memRegionToCoalesce.size;
            memRegionToCoalesce.ptr = freeRegionBefore->ptr;
            memRegionToCoalesce.size = newFreeRegionSize;

            mRegionsFree.emplace(memRegionToCoalesce);
        }
    }

    // find free regions after
    if ((memRegionToCoalesce.ptr + memRegionToCoalesce.size + 1) < mMemoryRegionSize)
    {
        auto freeRegionAfter = mRegionsFree.find(memRegionToCoalesce.ptr + memRegionToCoalesce.size + 1);
        if (freeRegionAfter != mRegionsFree.end())
        {
            mRegionsFree.erase(freeRegionAfter);

            const uint32_t newFreeRegionSize = freeRegionAfter->size + memRegionToCoalesce.size;
            memRegionToCoalesce.size = newFreeRegionSize;

            mRegionsFree.emplace(memRegionToCoalesce);
        }
    }
}

} // namespace kiss
