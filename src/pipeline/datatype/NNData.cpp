#include "depthai/pipeline/datatype/NNData.hpp"

#include <cassert>
#include <limits>
#include <unordered_map>
#include <vector>

#include "depthai-shared/datatype/RawNNData.hpp"
#include "depthai/pipeline/datatype/ADatatype.hpp"
#include "depthai/utility/VectorMemory.hpp"
#include "fp16/fp16.h"

namespace dai {

NNData::NNData() : Buffer(std::make_shared<RawNNData>()), rawNn(*dynamic_cast<RawNNData*>(raw.get())) {}
NNData::NNData(std::shared_ptr<RawNNData> ptr) : Buffer(ptr), rawNn(*ptr.get()) {}

static std::size_t sizeofTensorInfoDataType(TensorInfo::DataType type) {
    switch(type) {
        case TensorInfo::DataType::FP16:
            return sizeof(uint16_t);
        case TensorInfo::DataType::FP32:
            return sizeof(float);
        case TensorInfo::DataType::I8:
            return sizeof(int8_t);
        case TensorInfo::DataType::INT:
            return sizeof(int32_t);
        case TensorInfo::DataType::U8F:
            return sizeof(uint8_t);
        default:
            // invalid type, shouldn't happen
            assert(0);
            return 0;
    }
}

static std::size_t getTensorDataSize(const TensorInfo& tensor) {
    uint32_t i;

    // Use the first non zero stride
    for(i = 0; i < tensor.strides.size(); i++) {
        if(tensor.strides[i] > 0) {
            break;
        }
    }
    return tensor.dims[i] * tensor.strides[i];
}

NNData::Serialized NNData::serialize() const {
    // get data from u8Data and fp16Data and place properly into the underlying raw buffer
    rawNn.tensors = {};
    auto mem = std::make_shared<VectorMemory>();
    std::vector<uint8_t>& temporary = *mem;
    // data = mem;

    // U8 tensors
    for(const auto& kv : u8Data) {
        const auto dataType = TensorInfo::DataType::U8F;
        const auto dataSize = kv.second.size() * sizeofTensorInfoDataType(dataType);

        // First add any required padding bytes
        // calculate how many alignment bytes to add for next tensor
        size_t remainder = (temporary.end() - temporary.begin()) % DATA_ALIGNMENT;
        if(remainder > 0) {
            temporary.insert(temporary.end(), DATA_ALIGNMENT - remainder, 0);
        }

        // Then get offset to beginning of data
        size_t offset = temporary.end() - temporary.begin();

        const auto* data = reinterpret_cast<const std::uint8_t*>(kv.second.data());
        temporary.insert(temporary.end(), data, data + dataSize);

        // Add entry in tensors
        // TODO(themarpe) - refactor with proper way off specifying tensors
        TensorInfo info;
        info.dataType = dataType;
        info.numDimensions = 1;
        info.dims.push_back(kv.second.size());
        info.strides.push_back(sizeofTensorInfoDataType(dataType));
        info.name = kv.first;
        info.offset = static_cast<unsigned int>(offset);
        rawNn.tensors.push_back(info);
    }

    // FP16 tensors
    for(const auto& kv : fp16Data) {
        const auto dataType = TensorInfo::DataType::FP16;
        const auto dataSize = kv.second.size() * sizeofTensorInfoDataType(dataType);

        // First add any required padding bytes
        // calculate how many alignment bytes to add for next tensor
        size_t remainder = (temporary.end() - temporary.begin()) % DATA_ALIGNMENT;
        if(remainder > 0) {
            temporary.insert(temporary.end(), DATA_ALIGNMENT - remainder, 0);
        }

        // Then get offset to beginning of data
        size_t offset = temporary.end() - temporary.begin();

        const auto* data = reinterpret_cast<const std::uint8_t*>(kv.second.data());
        temporary.insert(temporary.end(), data, data + dataSize);

        // Add entry in tensors
        // TODO(themarpe) - refactor with proper way off specifying tensors
        TensorInfo info;
        info.dataType = dataType;
        info.numDimensions = 1;
        info.dims.push_back(kv.second.size());
        info.strides.push_back(sizeofTensorInfoDataType(dataType));
        info.name = kv.first;
        info.offset = static_cast<unsigned int>(offset);
        rawNn.tensors.push_back(info);
    }

    return {mem, raw};
}

uint16_t NNData::fp32_to_fp16(float value) {
    return fp16_ieee_from_fp32_value(value);
};

float NNData::fp16_to_fp32(uint16_t value) {
    return fp16_ieee_to_fp32_value(value);
};

// setters
// uint8_t
NNData& NNData::setLayer(const std::string& name, std::vector<std::uint8_t> data) {
    u8Data[name] = std::move(data);
    return *this;
}
NNData& NNData::setLayer(const std::string& name, const std::vector<int>& data) {
    u8Data[name] = std::vector<std::uint8_t>(data.size());
    for(unsigned i = 0; i < data.size(); i++) {
        u8Data[name][i] = static_cast<std::uint8_t>(data[i]);
    }
    return *this;
}

// fp16
NNData& NNData::setLayer(const std::string& name, std::vector<float> data) {
    fp16Data[name] = std::vector<std::uint16_t>(data.size());
    for(unsigned i = 0; i < data.size(); i++) {
        fp16Data[name][i] = fp16_ieee_from_fp32_value(data[i]);
    }
    return *this;
}
NNData& NNData::setLayer(const std::string& name, std::vector<double> data) {
    fp16Data[name] = std::vector<std::uint16_t>(data.size());
    for(unsigned i = 0; i < data.size(); i++) {
        fp16Data[name][i] = fp16_ieee_from_fp32_value(static_cast<float>(data[i]));
    }
    return *this;
}

// getters
std::vector<std::string> NNData::getAllLayerNames() const {
    std::vector<std::string> names;
    for(const auto& t : rawNn.tensors) {
        names.push_back(t.name);
    }
    return names;
}

std::vector<TensorInfo> NNData::getAllLayers() const {
    return rawNn.tensors;
}

bool NNData::getLayer(const std::string& name, TensorInfo& tensor) const {
    for(const auto& t : rawNn.tensors) {
        if(t.name == name) {
            tensor = t;
            return true;
        }
    }
    return false;
}

bool NNData::hasLayer(const std::string& name) const {
    for(const auto& tensor : rawNn.tensors) {
        if(tensor.name == name) return true;
    }
    return false;
}

bool NNData::getLayerDatatype(const std::string& name, TensorInfo::DataType& datatype) const {
    TensorInfo tensor;
    if(getLayer(name, tensor)) {
        datatype = tensor.dataType;
        return true;
    }
    return false;
}

// uint8
std::vector<std::uint8_t> NNData::getLayerUInt8(const std::string& name) const {
    // std::vector<std::uint8_t> data;
    // find layer name and its offset
    TensorInfo tensor;
    if(getLayer(name, tensor)) {
        if(tensor.dataType == TensorInfo::DataType::U8F) {
            // Total data size = last dimension * last stride
            if(tensor.numDimensions > 0) {
                size_t size = getTensorDataSize(tensor);
                auto beg = data->getData().begin() + tensor.offset;
                auto end = beg + size;
                return {beg, end};
            }
        }
    }
    return {};
}

// int32_t
std::vector<std::int32_t> NNData::getLayerInt32(const std::string& name) const {
    // find layer name and its offset
    TensorInfo tensor;
    if(getLayer(name, tensor)) {
        if(tensor.dataType == TensorInfo::DataType::INT) {
            // Total data size = last dimension * last stride
            if(tensor.numDimensions > 0) {
                size_t size = getTensorDataSize(tensor);
                std::size_t numElements = size / sizeof(std::int32_t);  // FP16

                std::vector<std::int32_t> data;
                data.reserve(numElements);
                auto* pInt32Data = reinterpret_cast<std::int32_t*>(&this->data->getData()[tensor.offset]);
                for(std::size_t i = 0; i < numElements; i++) {
                    data.push_back(pInt32Data[i]);
                }
                return data;
            }
        }
    }
    return {};
}

// fp16
std::vector<float> NNData::getLayerFp16(const std::string& name) const {
    // find layer name and its offset
    TensorInfo tensor;
    if(getLayer(name, tensor)) {
        if(tensor.dataType == TensorInfo::DataType::FP16) {
            // Total data size = last dimension * last stride
            if(tensor.numDimensions > 0) {
                std::size_t size = getTensorDataSize(tensor);
                std::size_t numElements = size / 2;  // FP16

                std::vector<float> data;
                data.reserve(numElements);
                auto* pFp16Data = reinterpret_cast<std::uint16_t*>(&this->data->getData()[tensor.offset]);
                for(std::size_t i = 0; i < numElements; i++) {
                    data.push_back(fp16_ieee_to_fp32_value(pFp16Data[i]));
                }
                return data;
            }
        }
    }
    return {};
}

// uint8
std::vector<std::uint8_t> NNData::getFirstLayerUInt8() const {
    // find layer name and its offset
    if(!rawNn.tensors.empty()) {
        return getLayerUInt8(rawNn.tensors[0].name);
    }

    return {};
}

// fp16
std::vector<float> NNData::getFirstLayerFp16() const {
    // find layer name and its offset
    if(!rawNn.tensors.empty()) {
        return getLayerFp16(rawNn.tensors[0].name);
    }
    return {};
}

// int32
std::vector<std::int32_t> NNData::getFirstLayerInt32() const {
    // find layer name and its offset
    if(!rawNn.tensors.empty()) {
        return getLayerInt32(rawNn.tensors[0].name);
    }
    return {};
}

// getters
std::chrono::time_point<std::chrono::steady_clock, std::chrono::steady_clock::duration> NNData::getTimestamp() const {
    using namespace std::chrono;
    return time_point<steady_clock, steady_clock::duration>{seconds(rawNn.ts.sec) + nanoseconds(rawNn.ts.nsec)};
}
std::chrono::time_point<std::chrono::steady_clock, std::chrono::steady_clock::duration> NNData::getTimestampDevice() const {
    using namespace std::chrono;
    return time_point<steady_clock, steady_clock::duration>{seconds(rawNn.tsDevice.sec) + nanoseconds(rawNn.tsDevice.nsec)};
}
int64_t NNData::getSequenceNum() const {
    return rawNn.sequenceNum;
}

// setters
NNData& NNData::setTimestamp(std::chrono::time_point<std::chrono::steady_clock, std::chrono::steady_clock::duration> tp) {
    // Set timestamp from timepoint
    using namespace std::chrono;
    auto ts = tp.time_since_epoch();
    rawNn.ts.sec = duration_cast<seconds>(ts).count();
    rawNn.ts.nsec = duration_cast<nanoseconds>(ts).count() % 1000000000;
    return *this;
}
NNData& NNData::setTimestampDevice(std::chrono::time_point<std::chrono::steady_clock, std::chrono::steady_clock::duration> tp) {
    // Set timestamp from timepoint
    using namespace std::chrono;
    auto ts = tp.time_since_epoch();
    rawNn.tsDevice.sec = duration_cast<seconds>(ts).count();
    rawNn.tsDevice.nsec = duration_cast<nanoseconds>(ts).count() % 1000000000;
    return *this;
}
NNData& NNData::setSequenceNum(int64_t sequenceNum) {
    rawNn.sequenceNum = sequenceNum;
    return *this;
}

TensorInfo::DataType NNData::getTensorDatatype(const std::string& name) {
    const auto it = std::find_if(rawNn.tensors.begin(), rawNn.tensors.end(), [&name](const TensorInfo& ti) { return ti.name == name; });

    if(it == rawNn.tensors.end()) throw std::runtime_error("Tensor does not exist");

    return it->dataType;
};
}  // namespace dai
