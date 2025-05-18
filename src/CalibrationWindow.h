#pragma once

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include <opencv2/opencv.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/flann.hpp>

class CalibrationWindow : public QDialog {
    Q_OBJECT

public:
    explicit CalibrationWindow(QWidget *parent = nullptr);
    ~CalibrationWindow();

    // Get calibration data
    cv::Rect getPalmRegion() const { return calibratedPalmRegion; }
    std::vector<cv::KeyPoint> getKeypoints() const { return calibrationKeypoints; }
    cv::Mat getDescriptors() const { return calibrationDescriptors; }
    bool isCalibrated() const { return calibrationComplete; }

signals:
    void calibrationFinished(bool success);

private slots:
    void updateFrame();
    void startCalibration();
    void finishCalibration();

private:
    // UI components
    QLabel *videoLabel;
    QPushButton *calibrateButton;
    QPushButton *finishButton;
    QTimer *timer;
    QVBoxLayout *layout;

    // OpenCV components
    cv::VideoCapture capture;
    cv::CascadeClassifier palmCascade;
    cv::Mat currentFrame;

    // FLANN components
    cv::Ptr<cv::FeatureDetector> featureDetector;
    cv::Ptr<cv::DescriptorExtractor> descriptorExtractor;
    cv::Ptr<cv::FlannBasedMatcher> flannMatcher;

    // Calibration data
    cv::Rect calibratedPalmRegion;
    std::vector<cv::KeyPoint> calibrationKeypoints;
    cv::Mat calibrationDescriptors;
    bool calibrationComplete;

    // Helper methods
    bool initializePalmDetection();
    bool detectPalm();
    void extractFeatures();
    QImage matToQImage(const cv::Mat &mat);
};
