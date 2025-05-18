#ifndef WEBCAMHANDLER_H
#define WEBCAMHANDLER_H

#include <QObject>
#include <QThread>
#include <QImage>
#include <QPoint>
#include <opencv2/opencv.hpp>

class WebcamHandler : public QObject {
    Q_OBJECT

public:
    explicit WebcamHandler(QObject *parent = nullptr);
    ~WebcamHandler();

    void startCamera();
    void stopCamera();

signals:
    void frameReady(const QImage &frame);
    void handDetected(const QPoint &center);

private:
    void processFrame();
    void startProcessing();

    QThread workerThread;
    cv::VideoCapture cap;
    cv::CascadeClassifier palmCascade;
    bool running;
};

#endif // WEBCAMHANDLER_H
