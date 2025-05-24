#include "WebcamHandler.h"
#include <QtConcurrent>
#include <QDebug>
#include <QThreadPool>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QTemporaryFile>

WebcamHandler::WebcamHandler(QObject *parent) : QObject(parent), running(false) {

    QString resourcePath = ":/new/prefix3/resources/hand/palm.xml";
    QFile resourceFile(resourcePath);

    if (resourceFile.exists()) {
        qDebug() << "Resource file exists:" << resourcePath;

        QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir appDir(appDataPath);
        if (!appDir.exists()) {
            appDir.mkpath(".");
        }

        QString extractedFilePath = appDir.filePath("palm.xml");
        QFile extractedFile(extractedFilePath);

        if (resourceFile.open(QIODevice::ReadOnly)) {
            QByteArray fileData = resourceFile.readAll();
            resourceFile.close();

            if (extractedFile.open(QIODevice::WriteOnly)) {
                extractedFile.write(fileData);
                extractedFile.close();

                qDebug() << "Using extracted file path:" << extractedFilePath;

                if (!palmCascade.load(extractedFilePath.toStdString())) {
                    qWarning() << "Failed to load palm.xml from extracted file";

                    QString fallbackPath = "resources/hand/palm.xml";
                    if (!palmCascade.load(fallbackPath.toStdString())) {
                        qWarning() << "Failed to load palm.xml from fallback path";
                    } else {
                        qDebug() << "Successfully loaded palm.xml from fallback path";
                    }
                } else {
                    qDebug() << "Successfully loaded palm.xml from extracted file";
                }
            } else {
                qWarning() << "Could not create extracted file:" << extractedFilePath;
            }
        } else {
            qWarning() << "Could not open resource file for reading";
        }
    } else {
        qWarning() << "Resource file does not exist:" << resourcePath;

        QString fallbackPath = "resources/hand/palm.xml";
        if (!palmCascade.load(fallbackPath.toStdString())) {
            qWarning() << "Failed to load palm.xml from fallback path";
        } else {
            qDebug() << "Successfully loaded palm.xml from fallback path";
        }
    }

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

        cv::Mat gray;
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
        cv::equalizeHist(gray, gray);

        std::vector<cv::Rect> palms;
        palmCascade.detectMultiScale(gray, palms);

        for (const auto &palm : palms) {
            cv::rectangle(frame, palm, cv::Scalar(0, 255, 0), 2);

            QPoint center(palm.x + palm.width / 2, palm.y + palm.height / 2);
            emit handDetected(center);
        }

        QImage qFrame(frame.data, frame.cols, frame.rows, frame.step, QImage::Format_BGR888);

        QSize targetSize(640, 360); 
        QImage resizedFrame = qFrame.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

        emit frameReady(resizedFrame);
    }
}