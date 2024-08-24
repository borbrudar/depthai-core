#include "depthai/pipeline/node/AprilTag.hpp"

#include <opencv2/imgproc.hpp>
#include <thread>

#include "depthai/pipeline/datatype/AprilTags.hpp"
#include "pipeline/datatype/AprilTagConfig.hpp"
#include "properties/AprilTagProperties.hpp"
#include "spdlog/fmt/fmt.h"

extern "C" {
#include "apriltag.h"
#include "common/getopt.h"
#include "tag16h5.h"
#include "tag25h9.h"
#include "tag36h10.h"
#include "tag36h11.h"
#include "tagCircle21h7.h"
#include "tagCircle49h12.h"
#include "tagCustom48h12.h"
#include "tagStandard41h12.h"
#include "tagStandard52h13.h"
}

namespace dai {
namespace node {

AprilTag::AprilTag(std::unique_ptr<Properties> props) : DeviceNodeCRTP<DeviceNode, AprilTag, AprilTagProperties>(std::move(props)) {}

AprilTag::Properties& AprilTag::getProperties() {
    properties.initialConfig = initialConfig;
    return properties;
}

// Node properties configuration
void AprilTag::setWaitForConfigInput(bool wait) {
    properties.inputConfigSync = wait;
}

void AprilTag::setRunOnHost(bool runOnHost) {
    runOnHostVar = runOnHost;
}

bool AprilTag::runOnHost() const {
    return runOnHostVar;
}

void AprilTag::buildInternal() {
    if(device) {
        auto platform = device->getPlatform();
        runOnHostVar = platform == Platform::RVC2 || platform == Platform::RVC3;
    } else {
        // No device, default to host
        runOnHostVar = true;
    }
    logger->info("AprilTag node running on host: {}", runOnHostVar);
}

apriltag_family_t* getAprilTagFamily(dai::AprilTagConfig::Family family) {
    apriltag_family_t* tf = nullptr;
    switch(family) {
        case dai::AprilTagConfig::Family::TAG_36H11:
            tf = tag36h11_create();
            break;
        case dai::AprilTagConfig::Family::TAG_36H10:
            tf = tag36h10_create();
            break;
        case dai::AprilTagConfig::Family::TAG_25H9:
            tf = tag25h9_create();
            break;
        case dai::AprilTagConfig::Family::TAG_16H5:
            tf = tag16h5_create();
            break;
        case dai::AprilTagConfig::Family::TAG_CIR21H7:
            tf = tagCircle21h7_create();
            break;
        case dai::AprilTagConfig::Family::TAG_STAND41H12:
            tf = tagStandard41h12_create();
            break;
        default:
            throw std::runtime_error("Unsupported AprilTag family");
    }
    return tf;
}

void setDetectorConfig(apriltag_detector_t* td, const dai::AprilTagConfig& config) {
    // Set detector family
    apriltag_detector_add_family(td, getAprilTagFamily(config.family));

    // Set detector config
    td->quad_decimate = config.quadDecimate;
    td->quad_sigma = config.quadSigma;
    td->refine_edges = config.refineEdges;
    td->decode_sharpening = config.decodeSharpening;

    // Tresholds
    td->qtp.min_cluster_pixels = config.quadThresholds.minClusterPixels;
    td->qtp.critical_rad = config.quadThresholds.criticalDegree * (3.14159 / 180.0);  // Convert to radians
    td->qtp.max_line_fit_mse = config.quadThresholds.maxLineFitMse;
    td->qtp.deglitch = config.quadThresholds.deglitch;

    // We don't want to debug
    td->debug = 0;
}

void setDetectorProperties(apriltag_detector_t* td, const dai::AprilTagProperties& properties) {
    td->nthreads = properties.numThreads;
}

void AprilTag::run() {
    // Retrieve properties and initial config
    const dai::AprilTagProperties& properties = getProperties();
    dai::AprilTagConfig config = properties.initialConfig;

    // Prepare other variables
    std::shared_ptr<ImgFrame> inFrame = nullptr;
    std::shared_ptr<AprilTagConfig> inConfig = nullptr;

    // Setup april tag detector
    apriltag_detector_t* td = apriltag_detector_create();

    // Set detector properties
    setDetectorProperties(td, properties);

    // Set detector config
    setDetectorConfig(td, config);

    // TODOs:
    // - Handle everything that is settable in properties (family, etc) [DONE]
    // - In the case of a dynamic config setting different family, etc - handle it [DONE]
    // - Handle different input types (right now GRAY and NV12 work, but not the rest - not everything needs to be handled, but types that don't work should
    // error out) [DONE]
    // - Better error handling 
    // - Expose number of CPU threads as a property [DONE]
    while(isRunning()) {
        // Try getting config
        if(properties.inputConfigSync) {
            inConfig = inputConfig.get<AprilTagConfig>();
        } else {
            inConfig = inputConfig.tryGet<AprilTagConfig>();
        }

        // Set config if there is one
        if(inConfig != nullptr) {
            setDetectorConfig(td, *inConfig);
        }

        // Get latest frame
        inFrame = inputImage.get<ImgFrame>();

        cv::Mat img;
        cv::cvtColor(inFrame->getCvFrame(), img, cv::COLOR_BGR2GRAY);

        image_u8_t aprilImg = {.width = img.cols, .height = img.rows, .stride = img.cols, .buf = img.data};

        auto now = std::chrono::system_clock::now();
        zarray_t* detections = apriltag_detector_detect(td, &aprilImg);
        auto end = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsedSeconds = end - now;
        logger->trace("April detections took {} ms", elapsedSeconds.count() / 1000.0);

        std::shared_ptr<dai::AprilTags> aprilTags = std::make_shared<dai::AprilTags>();

        if(detections != nullptr) {
            int numDetections = zarray_size(detections);
            aprilTags->aprilTags.reserve(numDetections);
            for(int i = 0; i < numDetections; i++) {
                apriltag_detection_t* det = nullptr;
                zarray_get(detections, i, &det);
                if(det == nullptr) {
                    continue;
                }
                dai::AprilTag daiDet;
                daiDet.id = det->id;
                daiDet.hamming = det->hamming;
                daiDet.decisionMargin = det->decision_margin;
                dai::Point2f center;
                center.x = static_cast<float>(det->c[0]);
                center.y = static_cast<float>(det->c[1]);

                dai::Point2f topLeft;
                dai::Point2f topRight;
                dai::Point2f bottomRight;
                dai::Point2f bottomLeft;

                topLeft.x = static_cast<float>(det->p[3][0]);
                topLeft.y = static_cast<float>(det->p[3][1]);
                topRight.x = static_cast<float>(det->p[2][0]);
                topRight.y = static_cast<float>(det->p[2][1]);
                bottomRight.x = static_cast<float>(det->p[1][0]);
                bottomRight.y = static_cast<float>(det->p[1][1]);
                bottomLeft.x = static_cast<float>(det->p[0][0]);
                bottomLeft.y = static_cast<float>(det->p[0][1]);

                daiDet.topLeft = topLeft;
                daiDet.topRight = topRight;
                daiDet.bottomRight = bottomRight;
                daiDet.bottomLeft = bottomLeft;

                aprilTags->aprilTags.push_back(daiDet);
            }
        }

        logger->trace("Detected {} april tags", zarray_size(detections));
        out.send(aprilTags);
        passthroughInputImage.send(inFrame);
    }
}

}  // namespace node
}  // namespace dai
