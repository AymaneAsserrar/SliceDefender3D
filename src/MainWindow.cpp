#include "MainWindow.h"
#include "WebcamHandler.h"
#include "OpenGLWidget.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QCloseEvent>
#include <QTimer>
#include <QMessageBox>
#include <QPushButton>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), webcamHandler(new WebcamHandler()) {

    QWidget *centralWidget = new QWidget(this);
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);

    openglWidget = new OpenGLWidget(this);
    openglWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    mainLayout->addWidget(openglWidget, 3); 

    QWidget* rightWidget = new QWidget(this);
    rightWidget->setStyleSheet(
        "QWidget {"
        "   background-image: url(:/new/prefix2/resources/images/wood_text.jpg);"
        "   background-repeat: no-repeat;"
        "   background-position: center;"
        "   background-attachment: fixed;"
        "}"
        );

    QVBoxLayout* rightLayout = new QVBoxLayout(rightWidget);

    cameraLabel = new QLabel("Camera Feed", this);
    cameraLabel->setAlignment(Qt::AlignCenter);
    cameraLabel->setStyleSheet("background-color: black;");
    cameraLabel->setFixedSize(320, 240);

    scoreLabel = new QLabel("Score: 0", this);
    timeLabel = new QLabel("Time: 0s", this);
    scoreLabel->setAlignment(Qt::AlignCenter);
    timeLabel->setAlignment(Qt::AlignCenter);

    scoreLabel->setStyleSheet(
        "QLabel {"
        "   font-size: 20px;"
        "   font-weight: bold;"
        "   color: #2E7D32;"
        "   padding: 10px;"
        "   border: 2px solid #81C784;"
        "   border-radius: 12px;"
        "   background-color: rgba(232, 245, 233, 180);"  
        "}"
        );

    timeLabel->setStyleSheet(
        "QLabel {"
        "   font-size: 18px;"
        "   font-weight: semi-bold;"
        "   color: #1565C0;"
        "   padding: 8px;"
        "   border: 2px solid #64B5F6;"
        "   border-radius: 12px;"
        "   background-color: rgba(227, 242, 253, 180);"  
        "}"
        );

    rightLayout->addWidget(cameraLabel);
    rightLayout->addWidget(scoreLabel);
    rightLayout->addWidget(timeLabel);
    rightLayout->addStretch();  

    mainLayout->addWidget(rightWidget, 1);

    setCentralWidget(centralWidget);

    connect(webcamHandler, &WebcamHandler::frameReady, this, &MainWindow::updateCameraView);
    connect(webcamHandler, &WebcamHandler::handDetected, this, &MainWindow::onHandDetected);

    gameTimer = new QTimer(this);
    connect(gameTimer, &QTimer::timeout, this, &MainWindow::updateGameTime);

    connect(openglWidget, &OpenGLWidget::scoreIncreased, this, &MainWindow::incrementScore);
    connect(openglWidget, &OpenGLWidget::gameOver, this, &MainWindow::endGame);

    score = 0;
    elapsedTime = 0;
    scoreLabel->setText("Score: 0");
    timeLabel->setText("Time: 0s");

    gameTimer->start(1000);

    webcamHandler->startCamera();
}

MainWindow::~MainWindow() {

    gameTimer->stop();

    webcamHandler->stopCamera();
    webcamHandler->thread()->quit();
    webcamHandler->thread()->wait();
    delete webcamHandler;
    delete openglWidget;
}

void MainWindow::closeEvent(QCloseEvent *event) {
    webcamHandler->stopCamera();
    QMainWindow::closeEvent(event);
}

void MainWindow::updateCameraView(const QImage &frame) {
    QPixmap pixmap = QPixmap::fromImage(frame);
    cameraLabel->setPixmap(pixmap.scaled(cameraLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void MainWindow::onHandDetected(const QPoint &center) {
    if (!openglWidget) return;

    QSize camSize = !cameraLabel->pixmap().isNull() ? cameraLabel->pixmap().size() : cameraLabel->size();

    float normX = float(center.x()) / float(camSize.width());
    float normY = float(center.y()) / float(camSize.height());

    openglWidget->setHandPosition(normX, normY);
}

void MainWindow::incrementScore() {
    score += 1;
    scoreLabel->setText(QString("Score: %1").arg(score));
}

void MainWindow::updateGameTime() {
    elapsedTime++;
    timeLabel->setText(QString("Time: %1s").arg(elapsedTime));

    if (elapsedTime >= gameDuration) {
        endGame();
    }
}

void MainWindow::endGame() {

    static bool endGameInProgress = false;
    if (endGameInProgress) return;

    endGameInProgress = true;
    gameTimer->stop();

    QString message = QString(
                          "<div style='text-align: center;'>"
                          "<h1 style='color: #E53935; font-weight: bold;'>Game Over!</h1>"
                          "<p style='font-size: 18px; color: #3949AB;'>"
                          "Your Score: <b>%1</b><br>"
                          "Time Played: <b>%2 seconds</b>"
                          "</p>"
                          "</div>"
                          ).arg(score).arg(elapsedTime);

    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Game Over");
    msgBox.setTextFormat(Qt::RichText);
    msgBox.setText(message);
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);

    QPushButton* okButton = qobject_cast<QPushButton*>(msgBox.button(QMessageBox::Ok));
    if (okButton) {
        okButton->setStyleSheet(
            "QPushButton {"
            "   background-color: #1976D2;"
            "   color: white;"
            "   font-size: 16px;"
            "   font-weight: bold;"
            "   padding: 10px 25px;"
            "   border-radius: 15px;"
            "   min-width: 100px;"
            "}"
            "QPushButton:hover {"
            "   background-color: #1565C0;"
            "}"
            "QPushButton:pressed {"
            "   background-color: #0D47A1;"
            "}"
            );
    }

    msgBox.exec();

    score = 0;
    elapsedTime = 0;
    scoreLabel->setText("Score: 0");
    timeLabel->setText("Time: 0s");

    openglWidget->resetGame();

    QTimer::singleShot(100, [this]() {
        gameTimer->start(1000);
        endGameInProgress = false;
    });
}