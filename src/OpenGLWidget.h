#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QVector>
#include <QVector3D>
#include <QElapsedTimer>
#include "Projectile.h"

// Add OpenCV includes
#include <opencv2/opencv.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/flann.hpp>

class OpenGLWidget : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

public:
    OpenGLWidget(QWidget *parent = nullptr);
    ~OpenGLWidget();

    // Update to accept 3D position directly
    void setHandPosition(float normX, float normY);
    void setHandPosition(const QVector3D& position);
    void resetGame(); // Add method to reset game state

    // Add palm detection and calibration methods
    bool calibratePalmDetection(const cv::Mat& frame);
    bool processPalmDetection(const cv::Mat& frame);




    //camera control methods
    void resetCamera();

signals:
    void scoreIncreased(); // Add signal for score increment
    void calibrationComplete(bool success); // Add signal for calibration status

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    void timerEvent(QTimerEvent* /*event*/) override;

    // Add keyboard event handlers
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

private:
    // Hand tracking
    float handX = 0.0f, handY = 0.0f, handZ = 0.0f;
    float normHandX = 0.0f, normHandY = 0.0f;
    QVector3D handPosition;
    float cylinderRadius = 1.5f; // Increased from 1.0f to 1.5f to make cylinder bigger
    float cylinderHeight = 2.0f;
    bool handSet = false;

    // Shader program
    QOpenGLShaderProgram *shaderProgram;
    QOpenGLBuffer vbo;

    // Projectile management
    QVector<Projectile> projectiles;
    QVector<Projectile> pendingProjectiles;
    void spawnProjectile();
    void updateProjectiles(float deltaTime);
    void checkCollisions();
    void drawSpawningZone(); // Add method to visualize spawn zone

    // Game timer and state
    int timerId;
    QElapsedTimer elapsedTimer;
    float gameTime = 0.0f;
    float deltaTime = 0.0f;
    float lastSpawnTime = 0.0f;
    float spawnInterval = 2.0f; // Seconds between projectile spawns
    int score = 0;

    // Drawing helpers
    void drawCylinder();
    void drawSphere(float radius, int lats, int longs);
    void drawSword(); // Declaration of drawSword method

    //texture
    QOpenGLTexture* bladeTexture = nullptr;
    QOpenGLTexture* handleTexture = nullptr;


    // Calculate matrix data
    QMatrix4x4 projection;
    QMatrix4x4 view;

    // Palm detection members
    cv::CascadeClassifier palmCascade;
    bool palmCascadeLoaded = false;
    bool isCalibrated = false;
    cv::Rect calibratedPalmRegion;

    // FLANN-based tracking
    cv::Ptr<cv::FeatureDetector> featureDetector;
    cv::Ptr<cv::DescriptorExtractor> descriptorExtractor;
    cv::Ptr<cv::FlannBasedMatcher> flannMatcher;
    std::vector<cv::KeyPoint> calibrationKeypoints;
    cv::Mat calibrationDescriptors;

    // Helper methods for palm tracking
    bool initializePalmDetection();
    cv::Point2f trackPalmMovement(const cv::Mat& frame);
    void convertToHandPosition(const cv::Point2f& palmPosition, const cv::Size& frameSize);

    // Camera control variables
    QVector3D cameraPosition;
    float cameraYaw = 0.0f;   // Horizontal rotation (left/right)
    float cameraPitch = 0.0f; // Vertical rotation (up/down)
    float cameraDistance = 5.0f;
    bool keysPressed[4] = {false}; // Up, Down, Left, Right

    // Camera movement speed
    float cameraRotationSpeed = 70.0f; // Degrees per second
    float cameraMoveSpeed = 2.0f;      // Units per second

    // Camera update method
    void updateCamera();
};
