#include "WebcamHandler.h"
#include <QtConcurrent>
#include <QDebug>
#include <QThreadPool>

WebcamHandler::WebcamHandler(QObject *parent) : QObject(parent), running(false) {
    // Load the palm.xml Haar cascade file
    if (!palmCascade.load("C:/Users/pc/slicedefender3d/palm.xml")) {
        qWarning() << "Failed to load palm.xml";
    }

    // Move this object to a separate thread for processing
    moveToThread(&workerThread);
    connect(&workerThread, &QThread::started, this, &WebcamHandler::startProcessing);
    connect(&workerThread, &QThread::finished, this, &QObject::deleteLater);
}

WebcamHandler::~WebcamHandler() {
    stopCamera();
    workerThread.quit();
    workerThread.wait();
}

void WebcamHandler::startCamera() {
    if (!cap.open(0)) {
        qWarning() << "Failed to open webcam";
        return;
    }
    running = true;
    QThreadPool::globalInstance()->start([this]() { processFrame(); });
}

void WebcamHandler::stopCamera() {
    running = false;
    if (cap.isOpened()) {
        cap.release();
    }
    workerThread.quit();
    workerThread.wait();
}

void WebcamHandler::startProcessing() {
    QThreadPool::globalInstance()->start([this]() { processFrame(); });
}

void WebcamHandler::processFrame() {
    while (running) {
        cv::Mat frame;
        if (!cap.read(frame)) {
            qWarning() << "Failed to capture frame";
            continue;
        }

        // Convert to grayscale for detection
        cv::Mat gray;
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
        cv::equalizeHist(gray, gray);

        // Detect palms
        std::vector<cv::Rect> palms;
        palmCascade.detectMultiScale(gray, palms);

        // Draw rectangles around detected palms
        for (const auto &palm : palms) {
            cv::rectangle(frame, palm, cv::Scalar(0, 255, 0), 2);

            // Emit the center of the detected palm
            QPoint center(palm.x + palm.width / 2, palm.y + palm.height / 2);
            emit handDetected(center);
        }

        // Convert the frame to QImage
        QImage qFrame(frame.data, frame.cols, frame.rows, frame.step, QImage::Format_BGR888);

        // Resize the frame to make it larger and rectangular
        QSize targetSize(640, 360); // Example size: 640x360 (16:9 aspect ratio)
        QImage resizedFrame = qFrame.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

        // Emit the resized frame
        emit frameReady(resizedFrame);
    }
}
