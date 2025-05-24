#include "OpenGLWidget.h"
#include <QDebug>
#include <QDir>

bool OpenGLWidget::initializePalmDetection() {

    QString cascadePath = QDir::currentPath() + "/palm.xml";
    palmCascadeLoaded = palmCascade.load(cascadePath.toStdString());

    if (!palmCascadeLoaded) {
        qDebug() << "Failed to load palm cascade from:" << cascadePath;
        return false;
    }

    featureDetector = cv::ORB::create();
    descriptorExtractor = cv::ORB::create();
    flannMatcher = cv::FlannBasedMatcher::create();

    return true;
}

bool OpenGLWidget::calibratePalmDetection(const cv::Mat& frame) {
    if (!palmCascadeLoaded && !initializePalmDetection()) {
        return false;
    }

    cv::Mat grayFrame;
    cv::cvtColor(frame, grayFrame, cv::COLOR_BGR2GRAY);

    std::vector<cv::Rect> palms;
    palmCascade.detectMultiScale(grayFrame, palms, 1.1, 3, 0, cv::Size(30, 30));

    if (palms.empty()) {
        qDebug() << "No palm detected during calibration";
        return false;
    }

    calibratedPalmRegion = *std::max_element(palms.begin(), palms.end(), 
        [](const cv::Rect& a, const cv::Rect& b) { return a.area() < b.area(); });

    cv::Mat palmROI = grayFrame(calibratedPalmRegion);

    featureDetector->detect(palmROI, calibrationKeypoints);
    descriptorExtractor->compute(palmROI, calibrationKeypoints, calibrationDescriptors);

    if (calibrationKeypoints.empty()) {
        qDebug() << "No keypoints found in palm region";
        return false;
    }

    isCalibrated = true;
    emit calibrationComplete(true);
    return true;
}

bool OpenGLWidget::processPalmDetection(const cv::Mat& frame) {
    if (!isCalibrated) {
        return false;
    }

    cv::Point2f palmPosition = trackPalmMovement(frame);
    if (palmPosition.x < 0 || palmPosition.y < 0) {
        return false;
    }

    convertToHandPosition(palmPosition, frame.size());
    return true;
}

cv::Point2f OpenGLWidget::trackPalmMovement(const cv::Mat& frame) {

    cv::Mat grayFrame;
    cv::cvtColor(frame, grayFrame, cv::COLOR_BGR2GRAY);

    std::vector<cv::KeyPoint> currentKeypoints;
    cv::Mat currentDescriptors;
    featureDetector->detect(grayFrame, currentKeypoints);
    descriptorExtractor->compute(grayFrame, currentKeypoints, currentDescriptors);

    if (currentKeypoints.empty() || currentDescriptors.empty()) {
        return cv::Point2f(-1, -1);
    }

    std::vector<cv::DMatch> matches;
    flannMatcher->match(calibrationDescriptors, currentDescriptors, matches);

    double maxDist = 0;
    double minDist = 100;
    for (const auto& match : matches) {
        double dist = match.distance;
        if (dist < minDist) minDist = dist;
        if (dist > maxDist) maxDist = dist;
    }

    std::vector<cv::DMatch> goodMatches;
    for (const auto& match : matches) {
        if (match.distance < std::max(2*minDist, 0.02)) {
            goodMatches.push_back(match);
        }
    }

    if (goodMatches.empty()) {
        return cv::Point2f(-1, -1);
    }

    cv::Point2f center(0, 0);
    for (const auto& match : goodMatches) {
        center += currentKeypoints[match.trainIdx].pt;
    }
    center.x /= goodMatches.size();
    center.y /= goodMatches.size();

    return center;
}

void OpenGLWidget::convertToHandPosition(const cv::Point2f& palmPosition, const cv::Size& frameSize) {

    float normX = palmPosition.x / frameSize.width;
    float normY = palmPosition.y / frameSize.height;

    setHandPosition(normX, normY);
}