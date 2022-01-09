#include "RelativeAllocator.hpp"

#include <cassert>
#include <cstdio>
#include <new>

namespace kiss
{

RelativeAllocator::MemoryRegion *RelativeAllocator::arrFindPtr(MemoryRegion *const arr, //
                                                               uint32_t &arrTail,       //
                                                               const RelativePtr &ptr)
{
    for (uint32_t i = 0U; i < arrTail; ++i)
    {
        auto &it = arr[i];
        const bool ptrInRange = //
            (ptr >= it.ptr) &&  //
            (ptr < (it.ptr + it.size));

        if (ptrInRange)
        {
            return &it;
        }
    }

    return nullptr;
}

RelativeAllocator::RelativeAllocator(uint8_t *const allocatorInternalSharedMem,     //
                                     const uint32_t allocatorInternalSharedMemSize, //
                                     const uint32_t memoryRegionSize)
    : mSharedMem(allocatorInternalSharedMem), //
      mMemoryRegionSize(memoryRegionSize)
{
    // all buckets are divided to two arrays - managing "used" and "free" memory
    constexpr uint32_t bookkeepingArraysCount = 2U;
    const uint32_t tailsSize = bookkeepingArraysCount * sizeof(*mRegionsFreeTail);
    const uint32_t numberOfBuckets =
        (allocatorInternalSharedMemSize - tailsSize) / sizeof(MemoryRegion) / bookkeepingArraysCount;

    // place the memory bookkeeping structures in shared memory
    uint8_t *const freeArrayAddr = mSharedMem;
    mRegionsFreeTail = new (freeArrayAddr) uint32_t;
    *mRegionsFreeTail = 0U;
    mRegionsFreeArr = new (freeArrayAddr + sizeof(*mRegionsFreeTail)) MemoryRegion[numberOfBuckets];
    mRegionsFree.assignArray(mRegionsFreeArr, mRegionsFreeTail);

    uint8_t *const usedArrayAddr = mSharedMem + (numberOfBuckets * sizeof(MemoryRegion));
    mRegionsUsedTail = new (usedArrayAddr) uint32_t;
    *mRegionsUsedTail = 0U;
    mRegionsUsedArr = new (usedArrayAddr + sizeof(*mRegionsUsedTail)) MemoryRegion[numberOfBuckets];
    mRegionsUsed.assignArray(mRegionsUsedArr, mRegionsUsedTail);

    (void)mRegionsFree.emplace(RelativePtr(0), memoryRegionSize);
}

RelativeAllocator::RelativePtr RelativeAllocator::alloc(const uint32_t sizeBytes)
{
    bool allocSuccess = false;
    RelativePtr allocAddr = 0;

    for (uint32_t i = 0U; i < *mRegionsFreeTail; ++i)
    {
        auto &it = mRegionsFreeArr[i];

        if (it.size > sizeBytes)
        {
            (void)mRegionsUsed.emplace(it.ptr, sizeBytes);
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

    return allocAddr;
}

void RelativeAllocator::dealloc(const RelativeAllocator::RelativePtr ptr)
{
    bool found = false;

    for (uint32_t i = 0U; i < *mRegionsUsedTail; ++i)
    {
        auto &it = mRegionsUsedArr[i];

        if (it.ptr == ptr)
        {
            found = true;

            mRegionsUsed.erase(it);

            auto &newMemRegion = mRegionsFree.emplace(it.ptr, it.size);
            tryToCoalesce(newMemRegion);
        }
    }

    if (!found)
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
        MemoryRegion *const freeRegionBefore =
            arrFindPtr(mRegionsFreeArr, *mRegionsFreeTail, memRegionToCoalesce.ptr - 1U);
        if (freeRegionBefore != nullptr)
        {
            const uint32_t newFreeRegionSize = freeRegionBefore->size + memRegionToCoalesce.size;
            memRegionToCoalesce.ptr = freeRegionBefore->ptr;
            memRegionToCoalesce.size = newFreeRegionSize;

            mRegionsFree.erase(*freeRegionBefore);
            mRegionsFree.erase(freedMemRegion);

            (void)mRegionsFree.emplace(memRegionToCoalesce.ptr, memRegionToCoalesce.size);
        }
    }

    // find free regions after
    if ((memRegionToCoalesce.ptr + memRegionToCoalesce.size + 1) < mMemoryRegionSize)
    {
        MemoryRegion *const freeRegionAfter =
            arrFindPtr(mRegionsFreeArr, *mRegionsFreeTail, memRegionToCoalesce.ptr + memRegionToCoalesce.size + 1);
        if (freeRegionAfter != nullptr)
        {
            const uint32_t newFreeRegionSize = freeRegionAfter->size + memRegionToCoalesce.size;
            memRegionToCoalesce.size = newFreeRegionSize;

            mRegionsFree.erase(*freeRegionAfter);
            mRegionsFree.erase(freedMemRegion);

            (void)mRegionsFree.emplace(memRegionToCoalesce.ptr, memRegionToCoalesce.size);
        }
    }
}

} // namespace kiss
