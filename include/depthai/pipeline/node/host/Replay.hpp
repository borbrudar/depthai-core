#pragma once

#include <cstdint>
#include <depthai/pipeline/ThreadedNode.hpp>
#include <memory>

// shared
#include <depthai/properties/XLinkOutProperties.hpp>

// project
#include <depthai/pipeline/datatype/Buffer.hpp>
#include <depthai/utility/RecordReplay.hpp>

#include "depthai/pipeline/ThreadedHostNode.hpp"
#include "depthai/pipeline/datatype/ImgFrame.hpp"

namespace dai {
namespace node {

/**
 * @brief Replay node, used to replay a file to a source node
 */
class Replay : public NodeCRTP<ThreadedHostNode, Replay> {
   private:
    std::optional<std::tuple<int, int>> size;
    std::optional<float> fps;
    std::string replayVideo;
    std::string replayFile;
    ImgFrame::Type outFrameType = ImgFrame::Type::YUV420p;

    std::shared_ptr<Buffer> getMessage(utility::RecordType type, const nlohmann::json& metadata, std::vector<uint8_t>& frame);

   public:
    constexpr static const char* NAME = "Replay";

    /**
     * Output for any type of messages to be transferred over XLink stream
     *
     * Default queue is blocking with size 8
     */
    Output out{*this, {.name = "out", .types = {{DatatypeEnum::Buffer, true}}}};

    void run() override;

    Replay& setReplayFile(const std::string& replayFile);
    Replay& setReplayVideo(const std::string& replayVideo);
    Replay& setOutFrameType(ImgFrame::Type outFrameType);
    Replay& setSize(std::tuple<int, int> size);
    Replay& setSize(int width, int height);
    Replay& setFps(float fps);
};

}  // namespace node
}  // namespace dai
