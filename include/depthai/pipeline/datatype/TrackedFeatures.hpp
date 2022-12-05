#pragma once

#include <unordered_map>
#include <vector>

#include "depthai-shared/datatype/RawTrackedFeatures.hpp"
#include "depthai/pipeline/datatype/Buffer.hpp"

namespace dai {

/**
 * TrackedFeatures message. Carries position (X, Y) of tracked features and their ID.
 */
class TrackedFeatures : public Buffer {
    Serialized serialize() const override;
    RawTrackedFeatures& rawdata;

   public:
    /**
     * Construct TrackedFeatures message.
     */
    TrackedFeatures();
    explicit TrackedFeatures(std::shared_ptr<RawTrackedFeatures> ptr);
    virtual ~TrackedFeatures() = default;

    std::vector<TrackedFeature>& trackedFeatures;

    /**
     * Retrieves image timestamp related to dai::Clock::now()
     */
    std::chrono::time_point<std::chrono::steady_clock, std::chrono::steady_clock::duration> getTimestamp() const;

    /**
     * Retrieves image timestamp directly captured from device's monotonic clock,
     * not synchronized to host time. Used mostly for debugging
     */
    std::chrono::time_point<std::chrono::steady_clock, std::chrono::steady_clock::duration> getTimestampDevice() const;

    /**
     * Retrieves image sequence number
     */
    int64_t getSequenceNum() const;

    /**
     * Sets image timestamp related to dai::Clock::now()
     */
    TrackedFeatures& setTimestamp(std::chrono::time_point<std::chrono::steady_clock, std::chrono::steady_clock::duration> timestamp);

    /**
     * Sets image timestamp related to dai::Clock::now()
     */
    TrackedFeatures& setTimestampDevice(std::chrono::time_point<std::chrono::steady_clock, std::chrono::steady_clock::duration> timestamp);

    /**
     * Retrieves image sequence number
     */
    TrackedFeatures& setSequenceNum(int64_t sequenceNum);
};

}  // namespace dai
