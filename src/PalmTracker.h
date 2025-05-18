#pragma once

#include <QObject>
#include <QPointF>  // Add this include for complete QPointF definition
#include <opencv2/opencv.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/flann.hpp>

class PalmTracker : public QObject {
    Q_OBJECT

public:
    explicit PalmTracker(QObject *parent = nullptr);
    
    // Initialize with calibration data
    void setCalibrationData(const cv::Rect& region, 
                           const std::vector<cv::KeyPoint>& keypoints,
                           const cv::Mat& descriptors);
    
    // Track palm in a new frame
    bool trackPalm(const cv::Mat& frame);
    
    // Get normalized palm position (0.0-1.0)
    QPointF getNormalizedPosition() const;

signals:
    void palmPositionChanged(float normX, float normY);

private:
    // Calibration data
    cv::Rect palmRegion;
    std::vector<cv::KeyPoint> calibrationKeypoints;
    cv::Mat calibrationDescriptors;
    bool isInitialized = false;
    
    // FLANN tracking
    cv::Ptr<cv::FeatureDetector> featureDetector;
    cv::Ptr<cv::DescriptorExtractor> descriptorExtractor;
    cv::Ptr<cv::FlannBasedMatcher> flannMatcher;
    
    // Current tracking position
    cv::Point2f currentPosition;
    QPointF normalizedPosition;
    cv::Size lastFrameSize;
};
