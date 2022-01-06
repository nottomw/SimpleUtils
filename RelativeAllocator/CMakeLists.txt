cmake_minimum_required(VERSION 2.8)

# Relative allocator working on shared memory region, no
# dynamic allocations.
add_library(RelativeAllocator STATIC
    RelativeAllocator.cpp
)

add_library(utils::RelativeAllocator ALIAS RelativeAllocator)

# Relative allocator implementation variant with std::set (heap used).
add_library(RelativeAllocatorMem STATIC
    RelativeAllocatorMem.cpp
)

add_library(utils::RelativeAllocatorMem ALIAS RelativeAllocatorMem)

