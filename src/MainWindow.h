#include <QMainWindow>
#include <QLabel>
#include <QTimer>
#include <QMessageBox>

class WebcamHandler;
class OpenGLWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    
    // Method to increment score
    void incrementScore();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void updateCameraView(const QImage &frame);
    void onHandDetected(const QPoint &center);
    
    // Add new game management slots
    void updateGameTime();
    void endGame();

private:
    WebcamHandler *webcamHandler;
    OpenGLWidget *openglWidget;
    QLabel *cameraLabel;
    QLabel *scoreLabel;
    QLabel *timeLabel;
    
    // Game state variables
    int score = 0;
    int elapsedTime = 0;
    QTimer *gameTimer;
    
    // Game duration in seconds (2 minutes)
    const int gameDuration = 120;
};
