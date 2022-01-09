#ifndef RINGBUFFER_HPP
#define RINGBUFFER_HPP

#include <array>
#include <cstring>

namespace kiss
{

enum class RingBufferResult
{
    OK,
    ERR_FULL,
    ERR_EMPTY
};

template <typename T, size_t Size> //
class RingBuffer
{
    // One entry wasted as "buffer full" marker.
    static constexpr size_t SizeWithPadding = Size + 1;

  public:
    RingBuffer()
    {
        mHead = 0U;
        mTail = 0U;
    }

    RingBufferResult push(const T *data, const uint32_t count)
    {
        const uint32_t sizeLeftCur = sizeLeft();
        if (sizeLeftCur < count)
        {
            return RingBufferResult::ERR_FULL;
        }

        const uint32_t copyBytesCount = count * sizeof(T);

        if ((mHead + copyBytesCount) < SizeWithPadding)
        {
            (void)memcpy(&ringBuffer[mHead], data, copyBytesCount);
        }
        else
        {
            const uint32_t firstCopySize = SizeWithPadding - mHead;
            const uint32_t secondCopySize = copyBytesCount - firstCopySize;

            (void)memcpy(&ringBuffer[mHead], data, firstCopySize);
            (void)memcpy(&ringBuffer[0], data + firstCopySize, secondCopySize);
        }

        mHead = (mHead + copyBytesCount) % SizeWithPadding;

        return RingBufferResult::OK;
    }

    RingBufferResult push(const T *data)
    {
        return push(data, 1);
    }

    RingBufferResult pop(T *data, const uint32_t count)
    {
        if (isEmpty())
        {
            return RingBufferResult::ERR_EMPTY;
        }

        const uint32_t copyBytesCount = count * sizeof(T);

        if ((mTail + copyBytesCount) < SizeWithPadding)
        {
            (void)memcpy(data, &ringBuffer[mTail], copyBytesCount);
        }
        else
        {
            const uint32_t firstCopySize = SizeWithPadding - mTail;
            const uint32_t secondCopySize = copyBytesCount - firstCopySize;

            (void)memcpy(data, &ringBuffer[mTail], firstCopySize);
            (void)memcpy(data + firstCopySize, &ringBuffer[0], secondCopySize);
        }

        mTail = (mTail + copyBytesCount) % SizeWithPadding;

        return RingBufferResult::OK;
    }

    RingBufferResult pop(T *data)
    {
        return pop(data, 1);
    }

  private:
    bool isFull(void)
    {
        return ((mHead + 1U) == mTail);
    }
    bool isEmpty(void)
    {
        return (mHead == mTail);
    }

    uint32_t sizeLeft(void)
    {
        if (isFull())
        {
            return 0U;
        }

        if (mHead > mTail)
        {
            return (mHead - mTail - 1);
        }
        else
        {
            return (SizeWithPadding + mHead - mTail - 1);
        }
    }

    uint32_t mHead;
    uint32_t mTail;

    std::array<T, SizeWithPadding> ringBuffer;
};

} // namespace kiss

#endif // RINGBUFFER_HPP
