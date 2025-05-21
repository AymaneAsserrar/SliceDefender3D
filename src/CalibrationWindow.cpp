#include "CalibrationWindow.h"
#include <QMessageBox>
#include <QDir>
#include <QDebug>
#include <QCoreApplication>

CalibrationWindow::CalibrationWindow(QWidget *parent) : QDialog(parent) {
    // Set up window properties
    setWindowTitle("Palm Calibration");
    setMinimumSize(640, 520);
    setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);

    // Create UI elements
    videoLabel = new QLabel(this);
    videoLabel->setAlignment(Qt::AlignCenter);
    videoLabel->setMinimumSize(640, 480);

    calibrateButton = new QPushButton("Calibrate Palm", this);
    finishButton = new QPushButton("Finish Calibration", this);
    finishButton->setEnabled(false);

    // Create layout
    layout = new QVBoxLayout(this);
    layout->addWidget(videoLabel);
    layout->addWidget(calibrateButton);
    layout->addWidget(finishButton);
    setLayout(layout);

    // Initialize OpenCV components
    if (!initializePalmDetection()) {
        QMessageBox::critical(this, "Error", "Failed to initialize palm detection!");
    }

    // Open webcam
    capture.open(0);
    if (!capture.isOpened()) {
        QMessageBox::critical(this, "Error", "Failed to open webcam!");
    }

    // Initialize timer for video feed
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &CalibrationWindow::updateFrame);
    timer->start(30); // ~30fps

    // Connect signals and slots
    connect(calibrateButton, &QPushButton::clicked, this, &CalibrationWindow::startCalibration);
    connect(finishButton, &QPushButton::clicked, this, &CalibrationWindow::finishCalibration);
    
    // Initialize calibration status
    calibrationComplete = false;
}

CalibrationWindow::~CalibrationWindow() {
    if (timer->isActive()) {
        timer->stop();
    }
    if (capture.isOpened()) {
        capture.release();
    }
}

bool CalibrationWindow::initializePalmDetection() {
    // Try multiple possible locations for the cascade files
    QStringList possiblePaths;
    
    // Get application directory path
    QString appDir = QCoreApplication::applicationDirPath();
    QString projectDir = appDir;
    
    // Add possible locations to search
    possiblePaths << appDir + "/palm.xml"                         // In build/exe directory
                  << appDir + "/haarcascade_hand.xml"             // Fallback in build/exe dir
                  << appDir + "/../palm.xml"                      // One level up
                  << appDir + "/../haarcascade_hand.xml"          // Fallback one level up
                  << "C:/Users/pc/slicedefender3d/palm.xml";       // Absolute path to project root
    
    // Try to find the cascade file in any of the possible locations
    bool palmCascadeLoaded = false;
    
    for (const QString& path : possiblePaths) {
        qDebug() << "Trying to load cascade from:" << path;
        palmCascadeLoaded = palmCascade.load(path.toStdString());
        if (palmCascadeLoaded) {
            qDebug() << "Successfully loaded cascade from:" << path;
            break;
        }
    }
    
    if (!palmCascadeLoaded) {
        qDebug() << "Failed to load any cascade file after trying all possible paths";
        return false;
    }
    
    // Initialize FLANN components
    featureDetector = cv::ORB::create();
    descriptorExtractor = cv::ORB::create();
    flannMatcher = cv::FlannBasedMatcher::create();
    
    return true;
}

void CalibrationWindow::updateFrame() {
    if (!capture.isOpened()) {
        return;
    }

    // Read frame from webcam
    if (!capture.read(currentFrame)) {
        return;
    }

    // Flip the frame horizontally for a more intuitive view
    cv::flip(currentFrame, currentFrame, 1);

    // Draw a guide box in the center for palm placement
    cv::Size frameSize = currentFrame.size();
    cv::Rect guideRect(frameSize.width/2 - 100, frameSize.height/2 - 100, 200, 200);
    cv::rectangle(currentFrame, guideRect, cv::Scalar(0, 255, 0), 2);

    // If palm detection is active, try to detect the palm
    if (calibrationComplete) {
        cv::rectangle(currentFrame, calibratedPalmRegion, cv::Scalar(0, 0, 255), 2);
        for (const auto& kp : calibrationKeypoints) {
            cv::circle(currentFrame, 
                       cv::Point(kp.pt.x + calibratedPalmRegion.x, kp.pt.y + calibratedPalmRegion.y), 
                       3, cv::Scalar(255, 0, 0), -1);
        }
    }

    // Convert cv::Mat to QImage and display it
    QImage image = matToQImage(currentFrame);
    videoLabel->setPixmap(QPixmap::fromImage(image).scaled(videoLabel->size(), 
                                                         Qt::KeepAspectRatio, 
                                                         Qt::SmoothTransformation));
}

void CalibrationWindow::startCalibration() {
    // Prompt the user to place their palm
    QMessageBox::information(this, "Palm Calibration", 
                          "Place your palm in the green box and hold steady.");

    // Detect palm in current frame
    if (detectPalm()) {
        extractFeatures();
        calibrationComplete = true;
        finishButton->setEnabled(true);
        QMessageBox::information(this, "Calibration Complete", 
                              "Palm calibration successful! You can now finish calibration.");
    } else {
        QMessageBox::warning(this, "Calibration Failed", 
                          "Failed to detect palm. Please try again.");
    }
}

void CalibrationWindow::finishCalibration() {
    if (calibrationComplete) {
        emit calibrationFinished(true);
        accept(); // Close the dialog with acceptance
    } else {
        QMessageBox::warning(this, "Calibration Required", 
                          "Please complete palm calibration first.");
    }
}

bool CalibrationWindow::detectPalm() {
    // Convert to grayscale
    cv::Mat grayFrame;
    cv::cvtColor(currentFrame, grayFrame, cv::COLOR_BGR2GRAY);
    
    // Detect palms
    std::vector<cv::Rect> palms;
    palmCascade.detectMultiScale(grayFrame, palms, 1.1, 3, 0, cv::Size(30, 30));
    
    if (palms.empty()) {
        qDebug() << "No palm detected during calibration";
        return false;
    }
    
    // Use the largest detected palm region
    calibratedPalmRegion = *std::max_element(palms.begin(), palms.end(), 
        [](const cv::Rect& a, const cv::Rect& b) { return a.area() < b.area(); });
    
    return true;
}

void CalibrationWindow::extractFeatures() {
    // Convert to grayscale
    cv::Mat grayFrame;
    cv::cvtColor(currentFrame, grayFrame, cv::COLOR_BGR2GRAY);
    
    // Extract region of interest (ROI)
    cv::Mat palmROI = grayFrame(calibratedPalmRegion);
    
    // Extract keypoints and descriptors for tracking
    featureDetector->detect(palmROI, calibrationKeypoints);
    descriptorExtractor->compute(palmROI, calibrationKeypoints, calibrationDescriptors);
}

QImage CalibrationWindow::matToQImage(const cv::Mat &mat) {
    // Convert OpenCV Mat to QImage
    if (mat.type() == CV_8UC3) {
        // BGR to RGB
        cv::Mat rgbMat;
        cv::cvtColor(mat, rgbMat, cv::COLOR_BGR2RGB);
        return QImage((const uchar*)rgbMat.data, rgbMat.cols, rgbMat.rows, 
                     rgbMat.step, QImage::Format_RGB888).copy();
    } else if (mat.type() == CV_8UC1) {
        // Grayscale
        return QImage((const uchar*)mat.data, mat.cols, mat.rows, 
                     mat.step, QImage::Format_Grayscale8).copy();
    }
    
    return QImage();
}
