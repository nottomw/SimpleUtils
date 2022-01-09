#ifndef RAWARRAYWRAPPER_HPP
#define RAWARRAYWRAPPER_HPP

#include <cassert>
#include <cstdint>
#include <utility>

namespace kiss
{

template <typename T> //
class RawArrayWrapper
{
  public:
    RawArrayWrapper()    //
        : mArr(nullptr), //
          mArrTail(0U)
    {
    }

    RawArrayWrapper(T *arr, uint32_t *arrTail) //
        : mArr(arr),                           //
          mArrTail(arrTail)
    {
    }

    void assignArray(T *arr, uint32_t *arrTail)
    {
        mArr = arr;
        mArrTail = arrTail;
    }

    template <typename... Args> //
    T &emplace(Args... args)
    {
        assert(mArr != nullptr);
        assert(mArrTail != nullptr);

        mArr[*mArrTail] = std::move(T{args...});

        T &emplaced = mArr[*mArrTail];

        *mArrTail += 1U;

        return emplaced;
    }

    void erase(const T &elem)
    {
        assert(mArr != nullptr);
        assert(mArrTail != nullptr);
        assert(*mArrTail != 0);

        for (uint32_t i = 0U; i < *mArrTail; ++i)
        {
            if (mArr[i] == elem)
            {
                const uint32_t lastEntryIdx = *mArrTail - 1U;

                if (i != lastEntryIdx)
                {
                    // replace the removed entry with the last entry
                    mArr[i] = std::move(mArr[lastEntryIdx]);
                }

                *mArrTail -= 1;

                break;
            }
        }
    }

  private:
    T *mArr;
    uint32_t *mArrTail;
};

} // namespace kiss

#endif // RAWARRAYWRAPPER_HPP