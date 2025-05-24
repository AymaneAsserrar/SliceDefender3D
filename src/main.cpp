#include <QApplication>
#include "MainWindow.h"
#include "PalmTracker.h"
#include <opencv2/core.hpp>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    PalmTracker* palmTracker = new PalmTracker();

    cv::Rect dummyPalmRegion(100, 100, 200, 200);
    std::vector<cv::KeyPoint> dummyKeypoints;
    cv::Mat dummyDescriptors;

    dummyKeypoints.push_back(cv::KeyPoint(150.0f, 150.0f, 10.0f));
    dummyDescriptors = cv::Mat::zeros(1, 32, CV_8U);  

    palmTracker->setCalibrationData(dummyPalmRegion, dummyKeypoints, dummyDescriptors);

    MainWindow* mainWindow = new MainWindow();

    mainWindow->showMaximized();

    return app.exec();
}