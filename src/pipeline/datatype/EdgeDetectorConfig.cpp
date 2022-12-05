#include "depthai/pipeline/datatype/EdgeDetectorConfig.hpp"

namespace dai {

EdgeDetectorConfig::Serialized EdgeDetectorConfig::serialize() const {
    return {data, raw};
}

EdgeDetectorConfig::EdgeDetectorConfig() : Buffer(std::make_shared<RawEdgeDetectorConfig>()), cfg(*dynamic_cast<RawEdgeDetectorConfig*>(raw.get())) {}
EdgeDetectorConfig::EdgeDetectorConfig(std::shared_ptr<RawEdgeDetectorConfig> ptr)
    : Buffer(std::move(ptr)), cfg(*dynamic_cast<RawEdgeDetectorConfig*>(raw.get())) {}

void EdgeDetectorConfig::setSobelFilterKernels(const std::vector<std::vector<int>>& horizontalKernel, const std::vector<std::vector<int>>& verticalKernel) {
    cfg.config.sobelFilterHorizontalKernel = horizontalKernel;
    cfg.config.sobelFilterVerticalKernel = verticalKernel;
}

EdgeDetectorConfigData EdgeDetectorConfig::getConfigData() const {
    return cfg.config;
}

}  // namespace dai
