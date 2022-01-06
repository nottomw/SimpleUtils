#include "RelativeAllocator.hpp"

#include <cassert>
#include <cstdio>
#include <new>

namespace kiss
{

// TODO: array wrapper, similar to std::array<>

RelativeAllocator::MemoryRegion &RelativeAllocator::arrEmplace(MemoryRegion *const arr,                  //
                                                               uint32_t *arrTail,                        //
                                                               const RelativeAllocator::RelativePtr ptr, //
                                                               const uint32_t size)
{
    // TODO: size check
    // TODO: not thread/ipc safe

    arr[*arrTail].ptr = ptr;
    arr[*arrTail].size = size;

    MemoryRegion &emplaced = arr[*arrTail];

    *arrTail += 1U;

    return emplaced;
}

void RelativeAllocator::arrErase(MemoryRegion *const arr, //
                                 uint32_t &arrTail,       //
                                 const MemoryRegion &region)
{
    assert(arrTail != 0);

    for (uint32_t i = 0U; i < arrTail; ++i)
    {
        // size check just to make sure
        if ((arr[i].ptr == region.ptr) && (arr[i].size == region.size))
        {
            const uint32_t lastEntryIdx = arrTail - 1U;

            if (i != lastEntryIdx)
            {
                // replace the removed entry with the last entry
                arr[i].ptr = arr[lastEntryIdx].ptr;
                arr[i].size = arr[lastEntryIdx].size;
            }

            arrTail -= 1;

            break;
        }
    }
}

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
    mRegionsFree = new (freeArrayAddr + sizeof(*mRegionsFreeTail)) MemoryRegion[numberOfBuckets];

    uint8_t *const usedArrayAddr = mSharedMem + (numberOfBuckets * sizeof(MemoryRegion));
    mRegionsUsedTail = new (usedArrayAddr) uint32_t;
    *mRegionsUsedTail = 0U;
    mRegionsUsed = new (usedArrayAddr + sizeof(*mRegionsUsedTail)) MemoryRegion[numberOfBuckets];

    arrEmplace(mRegionsFree, mRegionsFreeTail, 0, memoryRegionSize);
}

RelativeAllocator::RelativePtr RelativeAllocator::alloc(const uint32_t sizeBytes)
{
    bool allocSuccess = false;
    RelativePtr allocAddr = 0;

    for (uint32_t i = 0U; i < *mRegionsFreeTail; ++i)
    {
        auto &it = mRegionsFree[i];

        if (it.size > sizeBytes)
        {
            arrEmplace(mRegionsUsed, mRegionsUsedTail, it.ptr, sizeBytes);
            allocAddr = it.ptr;

            const uint32_t newFreeRegionSize = it.size - sizeBytes;
            if (newFreeRegionSize != 0)
            {
                arrEmplace(mRegionsFree, mRegionsFreeTail, (it.ptr + sizeBytes), newFreeRegionSize);
            }

            arrErase(mRegionsFree, *mRegionsFreeTail, it);

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
        auto &it = mRegionsUsed[i];
        if (it.ptr == ptr)
        {
            found = true;

            arrErase(mRegionsUsed, *mRegionsUsedTail, it);

            auto &newMemRegion = arrEmplace(mRegionsFree, mRegionsFreeTail, it.ptr, it.size);
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
            arrFindPtr(mRegionsFree, *mRegionsFreeTail, memRegionToCoalesce.ptr - 1U);
        if (freeRegionBefore != nullptr)
        {
            arrErase(mRegionsFree, *mRegionsFreeTail, *freeRegionBefore);
            arrErase(mRegionsFree, *mRegionsFreeTail, freedMemRegion);

            const uint32_t newFreeRegionSize = freeRegionBefore->size + memRegionToCoalesce.size;
            memRegionToCoalesce.ptr = freeRegionBefore->ptr;
            memRegionToCoalesce.size = newFreeRegionSize;

            (void)arrEmplace(mRegionsFree, mRegionsFreeTail, freeRegionBefore->ptr, newFreeRegionSize);
        }
    }

    // find free regions after
    if ((memRegionToCoalesce.ptr + memRegionToCoalesce.size + 1) < mMemoryRegionSize)
    {
        MemoryRegion *const freeRegionBefore =
            arrFindPtr(mRegionsFree, *mRegionsFreeTail, memRegionToCoalesce.ptr + memRegionToCoalesce.size + 1);
        if (freeRegionBefore != nullptr)
        {
            arrErase(mRegionsFree, *mRegionsFreeTail, *freeRegionBefore);
            arrErase(mRegionsFree, *mRegionsFreeTail, freedMemRegion);

            const uint32_t newFreeRegionSize = freeRegionBefore->size + memRegionToCoalesce.size;
            memRegionToCoalesce.size = newFreeRegionSize;

            (void)arrEmplace(mRegionsFree, mRegionsFreeTail, memRegionToCoalesce.ptr, newFreeRegionSize);
        }
    }
}

} // namespace kiss
