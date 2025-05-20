#include <QApplication>
#include "MainWindow.h"
#include "CalibrationWindow.h"
#include "PalmTracker.h"
#include <opencv2/core.hpp>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Skip calibration: create PalmTracker with dummy data
    PalmTracker* palmTracker = new PalmTracker();

    // Provide dummy or default data (you can customize as needed)
    cv::Rect dummyPalmRegion(100, 100, 200, 200);
    std::vector<cv::KeyPoint> dummyKeypoints;
    cv::Mat dummyDescriptors;

    // Optional: fill dummyKeypoints and dummyDescriptors with mock values
    // For example:
    dummyKeypoints.push_back(cv::KeyPoint(150.0f, 150.0f, 10.0f));
    dummyDescriptors = cv::Mat::zeros(1, 32, CV_8U);  // Assuming ORB-like descriptor

    palmTracker->setCalibrationData(dummyPalmRegion, dummyKeypoints, dummyDescriptors);

    // Create and show MainWindow
    MainWindow* mainWindow = new MainWindow();

    // Optionally: pass palmTracker to mainWindow or link WebcamHandler here
    // e.g., mainWindow->setPalmTracker(palmTracker);

    mainWindow->show();

    return app.exec();
}
