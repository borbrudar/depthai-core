#pragma once

#include "depthai/pipeline/datatype/ImgFrame.hpp"


class DepthFrame : public ImgFrame {
   public:
    /**
     * Construct DepthFrame message.
     */
    DepthFrame() = default;
    virtual ~DepthFrame() = default;


    void getPointcloud();
    void save();
    void getMedianValue();
    void getMinValue();
    void getMaxValue();
    void getUnitType();
    
    /*
    getPointcloud()
    save()
    median/min/max values
    unit type
    source (tof, stereo..)
        if stereo: max disparity value, metadata of both left/right frames 
        if tof: phase unwrapping level (not sure if relevant)
    */
};