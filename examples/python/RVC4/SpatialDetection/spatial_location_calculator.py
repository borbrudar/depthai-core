#!/usr/bin/env python3

import cv2
import depthai as dai
import numpy as np
from pathlib import Path

# Temporary toy example until the RVC4 devices are calibrated
examplePath = (Path(__file__).parent / ".." / "..").resolve().absolute()
calibJsonFile = examplePath / "models" / "depthai_calib.json"
# Check if file exists otherwise provoke the user to run `python3 examples/python/install_requirements.py`
if not calibJsonFile.exists():
    import sys

    print(
        f"Calibration file not found at: {calibJsonFile}. Please run {sys.executable} {examplePath}/install_requirements.py to get it downloaded."
    )
    exit(1)


calibData = dai.CalibrationHandler(calibJsonFile)

color = (255, 255, 255)

# Create pipeline
pipeline = dai.Pipeline()
pipeline.setCalibrationData(calibData)

# Config
topLeft = dai.Point2f(0.4, 0.4)
bottomRight = dai.Point2f(0.6, 0.6)

# Define sources and outputs
monoLeft = pipeline.create(dai.node.Camera)
monoRight = pipeline.create(dai.node.Camera)
stereo = pipeline.create(dai.node.StereoDepth)
spatialLocationCalculator = pipeline.create(dai.node.SpatialLocationCalculator)

# Define sources and outputs
monoLeft.setBoardSocket(dai.CameraBoardSocket.CAM_A)
monoRight.setBoardSocket(dai.CameraBoardSocket.CAM_C)

# Linking
monoLeftOut = monoLeft.requestOutput((640, 400), type=dai.ImgFrame.Type.NV12)
monoRightOut = monoRight.requestOutput((640, 400), type=dai.ImgFrame.Type.NV12)
monoLeftOut.link(stereo.left)
monoRightOut.link(stereo.right)

stereo.setInputResolution(640, 400)
stereo.setRectification(True)
stereo.setExtendedDisparity(True)
stereo.setNumFramesPool(10)
syncedLeftQueue = stereo.syncedLeft.createOutputQueue()
syncedRightQueue = stereo.syncedRight.createOutputQueue()
# disparityQueue = stereo.disparity.createOutputQueue()

stepSize = 0.05

config = dai.SpatialLocationCalculatorConfigData()
config.depthThresholds.lowerThreshold = 10
config.depthThresholds.upperThreshold = 10000
calculationAlgorithm = dai.SpatialLocationCalculatorAlgorithm.MEDIAN
config.roi = dai.Rect(topLeft, bottomRight)

spatialLocationCalculator.inputConfig.setWaitForMessage(False)
spatialLocationCalculator.initialConfig.addROI(config)


xoutSpatialQueue = spatialLocationCalculator.out.createOutputQueue()
outputDepthQueue = spatialLocationCalculator.passthroughDepth.createOutputQueue()


# spatialLocationCalculator.passthroughDepth.link(xoutDepth.input)
stereo.depth.link(spatialLocationCalculator.inputDepth)


inputConfigQueue = spatialLocationCalculator.inputConfig.createInputQueue()

pipeline.start()
while pipeline.isRunning():
    spatialData = xoutSpatialQueue.get().getSpatialLocations()

    print("Use WASD keys to move ROI!")
    outputDepthIMage : dai.ImgFrame = outputDepthQueue.get()

    frameDepth = outputDepthIMage.getCvFrame()

    depthFrameColor = cv2.normalize(frameDepth, None, 255, 0, cv2.NORM_INF, cv2.CV_8UC1)
    depthFrameColor = cv2.equalizeHist(depthFrameColor)
    depthFrameColor = cv2.applyColorMap(depthFrameColor, cv2.COLORMAP_HOT)
    for depthData in spatialData:
        roi = depthData.config.roi
        roi = roi.denormalize(width=depthFrameColor.shape[1], height=depthFrameColor.shape[0])
        xmin = int(roi.topLeft().x)
        ymin = int(roi.topLeft().y)
        xmax = int(roi.bottomRight().x)
        ymax = int(roi.bottomRight().y)

        depthMin = depthData.depthMin
        depthMax = depthData.depthMax

        fontType = cv2.FONT_HERSHEY_TRIPLEX
        cv2.rectangle(depthFrameColor, (xmin, ymin), (xmax, ymax), color, cv2.FONT_HERSHEY_SCRIPT_SIMPLEX)
        cv2.putText(depthFrameColor, f"X: {int(depthData.spatialCoordinates.x)} mm", (xmin + 10, ymin + 20), fontType, 0.5, color)
        cv2.putText(depthFrameColor, f"Y: {int(depthData.spatialCoordinates.y)} mm", (xmin + 10, ymin + 35), fontType, 0.5, color)
        cv2.putText(depthFrameColor, f"Z: {int(depthData.spatialCoordinates.z)} mm", (xmin + 10, ymin + 50), fontType, 0.5, color)
    # Show the frame
    cv2.imshow("depth", depthFrameColor)

    key = cv2.waitKey(1)
    if key == ord('q'):
        pipeline.stop()
        break

    stepSize = 0.05

    newConfig = False

    key = cv2.waitKey(1000)
    if key == ord('q'):
        break
    elif key == ord('w'):
        if topLeft.y - stepSize >= 0:
            topLeft.y -= stepSize
            bottomRight.y -= stepSize
            newConfig = True
    elif key == ord('a'):
        if topLeft.x - stepSize >= 0:
            topLeft.x -= stepSize
            bottomRight.x -= stepSize
            newConfig = True
    elif key == ord('s'):
        if bottomRight.y + stepSize <= 1:
            topLeft.y += stepSize
            bottomRight.y += stepSize
            newConfig = True
    elif key == ord('d'):
        if bottomRight.x + stepSize <= 1:
            topLeft.x += stepSize
            bottomRight.x += stepSize
            newConfig = True
    elif key == ord('1'):
        calculationAlgorithm = dai.SpatialLocationCalculatorAlgorithm.MEAN
        print('Switching calculation algorithm to MEAN!')
        newConfig = True
    elif key == ord('2'):
        calculationAlgorithm = dai.SpatialLocationCalculatorAlgorithm.MIN
        print('Switching calculation algorithm to MIN!')
        newConfig = True
    elif key == ord('3'):
        calculationAlgorithm = dai.SpatialLocationCalculatorAlgorithm.MAX
        print('Switching calculation algorithm to MAX!')
        newConfig = True
    elif key == ord('4'):
        calculationAlgorithm = dai.SpatialLocationCalculatorAlgorithm.MODE
        print('Switching calculation algorithm to MODE!')
        newConfig = True
    elif key == ord('5'):
        calculationAlgorithm = dai.SpatialLocationCalculatorAlgorithm.MEDIAN
        print('Switching calculation algorithm to MEDIAN!')
        newConfig = True

    if newConfig:
        config.roi = dai.Rect(topLeft, bottomRight)
        config.calculationAlgorithm = calculationAlgorithm
        cfg = dai.SpatialLocationCalculatorConfig()
        cfg.addROI(config)
        inputConfigQueue.send(cfg)
        newConfig = False
