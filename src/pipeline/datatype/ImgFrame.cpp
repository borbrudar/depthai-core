#define _USE_MATH_DEFINES
#include "depthai/pipeline/datatype/ImgFrame.hpp"

#include "spdlog/fmt/fmt.h"
namespace dai {

ImgFrame::Serialized ImgFrame::serialize() const {
    return {data, raw};
}

ImgFrame::ImgFrame() : Buffer(std::make_shared<RawImgFrame>()), img(*dynamic_cast<RawImgFrame*>(raw.get())), transformations(img.transformations) {
    // set timestamp to now
    setTimestamp(std::chrono::steady_clock::now());
}

ImgFrame::ImgFrame(size_t size) : ImgFrame() {
    auto mem = std::make_shared<VectorMemory>();
    mem->resize(size);
    data = mem;
}

ImgFrame::ImgFrame(std::shared_ptr<RawImgFrame> ptr)
    : Buffer(std::move(ptr)), img(*dynamic_cast<RawImgFrame*>(raw.get())), transformations(img.transformations) {}

// helpers

// getters
std::chrono::time_point<std::chrono::steady_clock, std::chrono::steady_clock::duration> ImgFrame::getTimestamp() const {
    using namespace std::chrono;
    return time_point<steady_clock, steady_clock::duration>{seconds(img.ts.sec) + nanoseconds(img.ts.nsec)};
}
std::chrono::time_point<std::chrono::steady_clock, std::chrono::steady_clock::duration> ImgFrame::getTimestampDevice() const {
    using namespace std::chrono;
    return time_point<steady_clock, steady_clock::duration>{seconds(img.tsDevice.sec) + nanoseconds(img.tsDevice.nsec)};
}
unsigned int ImgFrame::getInstanceNum() const {
    return img.instanceNum;
}
unsigned int ImgFrame::getCategory() const {
    return img.category;
}
int64_t ImgFrame::getSequenceNum() const {
    return img.sequenceNum;
}
unsigned int ImgFrame::getWidth() const {
    return img.fb.width;
}
unsigned int ImgFrame::getStride() const {
    if(img.fb.stride == 0) return getWidth();
    return img.fb.stride;
}
unsigned int ImgFrame::getPlaneStride(int planeIndex) const {
    int planeStride = 0;
    switch(planeIndex) {
        case 0:
            planeStride = img.fb.p2Offset - img.fb.p1Offset;
            break;
        case 1:
            planeStride = img.fb.p3Offset - img.fb.p2Offset;
            break;
    }
    if(planeStride <= 0) planeStride = getStride() * getHeight();
    return planeStride;
}
unsigned int ImgFrame::getHeight() const {
    return img.fb.height;
}
unsigned int ImgFrame::getPlaneHeight() const {
    return getPlaneStride() / getStride();
}
RawImgFrame::Type ImgFrame::getType() const {
    return img.fb.type;
}
float ImgFrame::getBytesPerPixel() const {
    return img.typeToBpp(getType());
}
std::chrono::microseconds ImgFrame::getExposureTime() const {
    return std::chrono::microseconds(img.cam.exposureTimeUs);
}
int ImgFrame::getSensitivity() const {
    return img.cam.sensitivityIso;
}
int ImgFrame::getColorTemperature() const {
    return img.cam.wbColorTemp;
}
int ImgFrame::getLensPosition() const {
    return img.cam.lensPosition;
}

unsigned int ImgFrame::getSourceWidth() const {
    return img.sourceFb.width;
}
unsigned int ImgFrame::getSourceHeight() const {
    return img.sourceFb.height;
}

RawImgFrame ImgFrame::get() const {
    return img;
}

// setters
ImgFrame& ImgFrame::setTimestamp(std::chrono::time_point<std::chrono::steady_clock, std::chrono::steady_clock::duration> tp) {
    // Set timestamp from timepoint
    using namespace std::chrono;
    auto ts = tp.time_since_epoch();
    img.ts.sec = duration_cast<seconds>(ts).count();
    img.ts.nsec = duration_cast<nanoseconds>(ts).count() % 1000000000;
    return *this;
}
ImgFrame& ImgFrame::setTimestampDevice(std::chrono::time_point<std::chrono::steady_clock, std::chrono::steady_clock::duration> tp) {
    // Set timestamp from timepoint
    using namespace std::chrono;
    auto ts = tp.time_since_epoch();
    img.tsDevice.sec = duration_cast<seconds>(ts).count();
    img.tsDevice.nsec = duration_cast<nanoseconds>(ts).count() % 1000000000;
    return *this;
}
ImgFrame& ImgFrame::setInstanceNum(unsigned int instanceNum) {
    img.instanceNum = instanceNum;
    return *this;
}
ImgFrame& ImgFrame::setCategory(unsigned int category) {
    img.category = category;
    return *this;
}
ImgFrame& ImgFrame::setSequenceNum(int64_t sequenceNum) {
    img.sequenceNum = sequenceNum;
    return *this;
}
ImgFrame& ImgFrame::setWidth(unsigned int width) {
    img.fb.width = width;
    img.fb.stride = width;
    return *this;
}
ImgFrame& ImgFrame::setHeight(unsigned int height) {
    img.fb.height = height;
    return *this;
}
ImgFrame& ImgFrame::setSize(unsigned int width, unsigned int height) {
    setWidth(width);
    setHeight(height);
    return *this;
}
ImgFrame& ImgFrame::setSize(std::tuple<unsigned int, unsigned int> size) {
    setSize(std::get<0>(size), std::get<1>(size));
    return *this;
}

ImgFrame& ImgFrame::setSourceSize(unsigned int width, unsigned int height) {
    img.sourceFb.width = width;
    img.sourceFb.stride = width;
    img.sourceFb.height = height;
    transformations.setInitTransformation(width, height);
    return *this;
}

ImgFrame& ImgFrame::setSourceSize(std::tuple<unsigned int, unsigned int> size) {
    setSourceSize(std::get<0>(size), std::get<1>(size));
    return *this;
}
ImgFrame& ImgFrame::setType(RawImgFrame::Type type) {
    img.fb.type = type;
    img.fb.bytesPP = RawImgFrame::typeToBpp(img.fb.type);
    return *this;
}

void ImgFrame::set(RawImgFrame rawImgFrame) {
    img = rawImgFrame;
}

ImgFrame& ImgFrame::initMetadata(std::shared_ptr<dai::ImgFrame> sourceFrame) {
    set(sourceFrame->get());
    return *this;
}

dai::Point2f ImgFrame::remapPointFromSource(dai::Point2f point) const {
    if(point.isNormalized()) {
        throw std::runtime_error("Point must be denormalized");
    }
    dai::Point2f transformedPoint = point;
    bool isClipped = false;
    for(auto& transformation : transformations.transformations) {
        transformedPoint = transformations.transformPoint(transformation, transformedPoint, isClipped);
    }
    return transformedPoint;
}

dai::Point2f ImgFrame::remapPointToSource(dai::Point2f point) const {
    if(point.isNormalized()) {
        throw std::runtime_error("Point must be denormalized");
    }
    dai::Point2f transformedPoint = point;
    bool isClipped = false;
    // Do the loop in reverse order
    for(auto it = transformations.transformations.rbegin(); it != transformations.transformations.rend(); ++it) {
        transformedPoint = transformations.invTransformPoint(*it, transformedPoint, isClipped);
    }
    return transformedPoint;
}

dai::Rect ImgFrame::remapRectFromSource(dai::Rect rect) const {
    bool isNormalized = rect.isNormalized();
    if(isNormalized) {
        rect = rect.denormalize(getSourceWidth(), getSourceHeight());
    }
    auto topLeftTransformed = remapPointFromSource(rect.topLeft());
    auto bottomRightTransformed = remapPointFromSource(rect.bottomRight());
    dai::Rect returnRect(topLeftTransformed, bottomRightTransformed);
    if(isNormalized) {
        returnRect = returnRect.normalize(getWidth(), getHeight());
    }
    return returnRect;
}

dai::Rect ImgFrame::remapRectToSource(dai::Rect rect) const {
    bool isNormalized = rect.isNormalized();
    if(isNormalized) {
        rect = rect.denormalize(getWidth(), getHeight());
    }
    auto topLeftTransformed = remapPointToSource(rect.topLeft());
    auto bottomRightTransformed = remapPointToSource(rect.bottomRight());

    dai::Rect returnRect(topLeftTransformed, bottomRightTransformed);
    if(isNormalized) {
        returnRect = returnRect.normalize(getSourceWidth(), getSourceHeight());
    }
    return returnRect;
}

ImgFrame& ImgFrame::setSourceHFov(float degrees) {
    img.HFovDegrees = degrees;
    return *this;
}

float ImgFrame::getSourceHFov() {
    return img.HFovDegrees;
}

float ImgFrame::getSourceDFov() {
    // TODO only works rectlinear lenses (rectified frames).
    // Calculate the vertical FOV from the source dimensions and the source DFov
    float sourceWidth = getSourceWidth();
    float sourceHeight = getSourceHeight();

    if(sourceHeight <= 0) {
        throw std::runtime_error(fmt::format("Source height is invalid. Height: {}", sourceHeight));
    }
    if(sourceWidth <= 0) {
        throw std::runtime_error(fmt::format("Source width is invalid. Width: {}", sourceWidth));
    }
    float HFovDegrees = getSourceHFov();

    // Calculate the diagonal ratio (DR)
    float dr = std::sqrt(std::pow(sourceWidth, 2) + std::pow(sourceHeight, 2));

    // Validate the horizontal FOV
    if(HFovDegrees <= 0 || HFovDegrees >= 180) {
        throw std::runtime_error(fmt::format("Horizontal FOV is invalid. Horizontal FOV: {}", HFovDegrees));
    }

    float HFovRadians = HFovDegrees * (static_cast<float>(M_PI) / 180.0f);

    // Calculate the tangent of half of the HFOV
    float tanHFovHalf = std::tan(HFovRadians / 2);

    // Calculate the tangent of half of the VFOV
    float tanDiagonalFovHalf = (dr / sourceWidth) * tanHFovHalf;

    // Calculate the VFOV in radians
    float diagonalFovRadians = 2 * std::atan(tanDiagonalFovHalf);

    // Convert VFOV to degrees
    float diagonalFovDegrees = diagonalFovRadians * (180.0f / static_cast<float>(M_PI));
    return diagonalFovDegrees;
}

float ImgFrame::getSourceVFov() {
    // TODO only works rectlinear lenses (rectified frames).
    // Calculate the vertical FOV from the source dimensions and the source DFov
    float sourceWidth = getSourceWidth();
    float sourceHeight = getSourceHeight();

    if(sourceHeight <= 0) {
        throw std::runtime_error(fmt::format("Source height is invalid. Height: {}", sourceHeight));
    }
    if(sourceWidth <= 0) {
        throw std::runtime_error(fmt::format("Source width is invalid. Width: {}", sourceWidth));
    }
    float HFovDegrees = getSourceHFov();

    // Validate the horizontal FOV
    if(HFovDegrees <= 0 || HFovDegrees >= 180) {
        throw std::runtime_error(fmt::format("Horizontal FOV is invalid. Horizontal FOV: {}", HFovDegrees));
    }

    float HFovRadians = HFovDegrees * (static_cast<float>(M_PI) / 180.0f);

    // Calculate the tangent of half of the HFOV
    float tanHFovHalf = std::tan(HFovRadians / 2);

    // Calculate the tangent of half of the VFOV
    float tanVerticalFovHalf = (sourceHeight / sourceWidth) * tanHFovHalf;

    // Calculate the VFOV in radians
    float verticalFovRadians = 2 * std::atan(tanVerticalFovHalf);

    // Convert VFOV to degrees
    float verticalFovDegrees = verticalFovRadians * (180.0f / static_cast<float>(M_PI));
    return verticalFovDegrees;
}

bool ImgFrame::validateTransformations() const {
    // TODO Implementation
    return true;
}

dai::Point2f ImgFrame::remapPointBetweenSourceFrames(dai::Point2f point, std::shared_ptr<dai::ImgFrame> sourceImage, std::shared_ptr<dai::ImgFrame> destImage) {
    auto hFovDegreeDest = destImage->getSourceHFov();
    auto vFovDegreeDest = destImage->getSourceVFov();

    auto hFovDegreeOrigin = sourceImage->getSourceHFov();
    auto vFovDegreeOrigin = sourceImage->getSourceVFov();
    // TODO Implementation
    return point;
}

dai::Point2f ImgFrame::remapPointBetweenFrames(dai::Point2f originPoint, std::shared_ptr<dai::ImgFrame> originFrame, std::shared_ptr<dai::ImgFrame> destFrame) {
    // TODO Implementation
    return originPoint;
}

dai::Rect ImgFrame::remapRectangleBetweenFrames(dai::Rect originRect, std::shared_ptr<dai::ImgFrame> originFrame, std::shared_ptr<dai::ImgFrame> destFrame) {
    // TODO Implementation
    return originRect;
}

}  // namespace dai
