#include "MainWindow.h"
#include "WebcamHandler.h"
#include "OpenGLWidget.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QCloseEvent>
#include <QTimer>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), webcamHandler(new WebcamHandler()) {
    // Create UI elements
    QWidget *centralWidget = new QWidget(this);
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);

    // Left: OpenGLWidget
    openglWidget = new OpenGLWidget(this);
    openglWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    mainLayout->addWidget(openglWidget, 3); // Stretch factor 3 for OpenGLWidget

    // Right: Vertical column
    QVBoxLayout *rightLayout = new QVBoxLayout();

    // Webcam feed
    cameraLabel = new QLabel("Camera Feed", this);
    cameraLabel->setAlignment(Qt::AlignCenter);
    cameraLabel->setStyleSheet("background-color: black;");
    cameraLabel->setFixedSize(320, 240); // Fixed size for the webcam feed (red rectangle)

    // Score and Time labels
    scoreLabel = new QLabel("Score: 0", this);
    timeLabel = new QLabel("Time: 0", this);
    scoreLabel->setAlignment(Qt::AlignCenter);
    timeLabel->setAlignment(Qt::AlignCenter);

    // Add widgets to the right layout
    rightLayout->addWidget(cameraLabel);
    rightLayout->addWidget(scoreLabel);
    rightLayout->addWidget(timeLabel);
    rightLayout->addStretch(); // Add stretch to push labels to the top

    mainLayout->addLayout(rightLayout, 1); // Stretch factor 1 for the right layout

    setCentralWidget(centralWidget);

    // Connect WebcamHandler signals to slots
    connect(webcamHandler, &WebcamHandler::frameReady, this, &MainWindow::updateCameraView);
    connect(webcamHandler, &WebcamHandler::handDetected, this, &MainWindow::onHandDetected);

    // Initialize game timer
    gameTimer = new QTimer(this);
    connect(gameTimer, &QTimer::timeout, this, &MainWindow::updateGameTime);

    // Connect signals from OpenGLWidget
    connect(openglWidget, &OpenGLWidget::scoreIncreased, this, &MainWindow::incrementScore);

    // Reset game state
    score = 0;
    elapsedTime = 0;
    scoreLabel->setText("Score: 0");
    timeLabel->setText("Time: 0s");

    // Start the game timer
    gameTimer->start(1000); // 1-second updates

    // Start the webcam
    webcamHandler->startCamera();
}

MainWindow::~MainWindow() {
    // Stop the timer
    gameTimer->stop();

    // Ensure the webcam is stopped and delete the handler
    webcamHandler->stopCamera();
    webcamHandler->thread()->quit(); // Ensure the thread is stopped
    webcamHandler->thread()->wait(); // Wait for the thread to finish
    delete webcamHandler;
    delete openglWidget; // Explicitly delete OpenGLWidget if necessary
}

void MainWindow::closeEvent(QCloseEvent *event) {
    // Stop the webcam when the window is closed
    webcamHandler->stopCamera();
    QMainWindow::closeEvent(event);
}

void MainWindow::updateCameraView(const QImage &frame) {
    // Convert QImage to QPixmap and display it in the cameraLabel
    QPixmap pixmap = QPixmap::fromImage(frame);
    cameraLabel->setPixmap(pixmap.scaled(cameraLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void MainWindow::onHandDetected(const QPoint &center) {
    if (!openglWidget) return;

    QSize camSize = !cameraLabel->pixmap().isNull() ? cameraLabel->pixmap().size() : cameraLabel->size();

    // Convert to normalized coordinates (0.0 to 1.0)
    float normX = float(center.x()) / float(camSize.width());
    float normY = float(center.y()) / float(camSize.height());

    // Pass to OpenGLWidget for 3D positioning
    openglWidget->setHandPosition(normX, normY);

    // Update score display (if score is available from OpenGLWidget)
    // scoreLabel->setText(QString("Score: %1").arg(openglWidget->getScore()));
}

void MainWindow::incrementScore() {
    // Increase score by 1 point for each original projectile
    score += 1;
    scoreLabel->setText(QString("Score: %1").arg(score));
}

void MainWindow::updateGameTime() {
    elapsedTime++;
    timeLabel->setText(QString("Time: %1s").arg(elapsedTime));

    // End game after 2 minutes (120 seconds)
    if (elapsedTime >= gameDuration) {
        endGame();
    }
}

void MainWindow::endGame() {
    // Stop timer
    gameTimer->stop();

    // Show game over message
    QMessageBox::information(this, "Game Over",
                          QString("Game Over!\nYour Score: %1\nTime: %2 seconds")
                          .arg(score).arg(elapsedTime));

    // Reset for new game
    score = 0;
    elapsedTime = 0;
    scoreLabel->setText("Score: 0");
    timeLabel->setText("Time: 0s");

    // Reset the OpenGL scene
    openglWidget->resetGame();

    // Restart timer
    gameTimer->start(1000);
}
