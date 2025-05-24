#include "PalmTracker.h"
#include <QDebug>

PalmTracker::PalmTracker(QObject *parent) : QObject(parent) {

    featureDetector = cv::ORB::create();
    descriptorExtractor = cv::ORB::create();
    flannMatcher = cv::FlannBasedMatcher::create();
}

void PalmTracker::setCalibrationData(const cv::Rect& region, 
                                    const std::vector<cv::KeyPoint>& keypoints,
                                    const cv::Mat& descriptors) {
    palmRegion = region;
    calibrationKeypoints = keypoints;
    calibrationDescriptors = descriptors.clone();
    isInitialized = !keypoints.empty() && !descriptors.empty();
}

bool PalmTracker::trackPalm(const cv::Mat& frame) {
    if (!isInitialized) {
        return false;
    }

    lastFrameSize = frame.size();

    cv::Mat grayFrame;
    cv::cvtColor(frame, grayFrame, cv::COLOR_BGR2GRAY);

    std::vector<cv::KeyPoint> currentKeypoints;
    cv::Mat currentDescriptors;
    featureDetector->detect(grayFrame, currentKeypoints);
    if (currentKeypoints.empty()) {
        return false;
    }

    descriptorExtractor->compute(grayFrame, currentKeypoints, currentDescriptors);
    if (currentDescriptors.empty()) {
        return false;
    }

    std::vector<cv::DMatch> matches;
    flannMatcher->match(calibrationDescriptors, currentDescriptors, matches);

    if (matches.empty()) {
        return false;
    }

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
        return false;
    }

    cv::Point2f center(0, 0);
    for (const auto& match : goodMatches) {
        center += currentKeypoints[match.trainIdx].pt;
    }
    center.x /= goodMatches.size();
    center.y /= goodMatches.size();

    currentPosition = center;

    normalizedPosition.setX(currentPosition.x / lastFrameSize.width);
    normalizedPosition.setY(currentPosition.y / lastFrameSize.height);

    emit palmPositionChanged(normalizedPosition.x(), normalizedPosition.y());
    return true;
}

QPointF PalmTracker::getNormalizedPosition() const {
    return normalizedPosition;
}