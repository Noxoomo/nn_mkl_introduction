#pragma once

#include <core/object.h>
#include <vector>

class ArrayVec  : public Object {
public:
    using DataContainer = std::vector<float>;

    ArrayVec(int64_t size)
    : data_(std::make_shared<DataContainer>())
    , offset_(0) {
        data_->resize(size);
    }

    ArrayVec(std::shared_ptr<DataContainer> ptr, int64_t offset)
    : data_(ptr)
    , offset_(offset) {

    }

    const float* data() const {
        return data_->data();
    }
    float* data() {
        return data_->data();
    }
    int64_t dim() const {
        assert(offset_ <= data_->size());
        return data_->size() - offset_;
    }
private:
    std::shared_ptr<DataContainer> data_;
    int64_t offset_;
};


//#include <cstdint>
//#include <memory>
//
//class VecData : public Object {
//public:
//
//    virtual ~VecData() = default;
//
//    virtual double get(int64_t idx) const = 0;
//
//    virtual void set(int64_t idx, double value) = 0;
//
//    virtual int64_t dim() = 0;
//
//    virtual VecDataPtr slice(int64_t from, int64_t to) = 0;
//
//    virtual VecDataPtr instance(int64_t size, bool fillZero) = 0;
//};
//
//using VecDataPtr = VecData::VecDataPtr;
//
//namespace Impl {
//
//
//
//    class ArrayVec : public VecData {
//    public:
//
//    protected:
//        ArrayVecData(ui64 size);
//        ArrayVecData(ArrayVecData data, ui64 offset, ui64 size);
//    private:
//        std::shared_ptr<T> data_;
//        ui64 size_;
//        ui64 offset_;
//    };
//
//
//
//}