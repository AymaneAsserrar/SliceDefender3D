QT += core gui widgets openglwidgets concurrent

CONFIG += c++17
SOURCES += \
    src/main.cpp \
    src/MainWindow.cpp \
    src/OpenGLWidget.cpp \
    src/WebcamHandler.cpp \
    src/Projectile.cpp \
    src/PalmDetection.cpp \
    src/PalmTracker.cpp

HEADERS += \
    src/MainWindow.h \
    src/OpenGLWidget.h \
    src/WebcamHandler.h \
    src/Projectile.h \
    src/PalmTracker.h

# OpenCV

INCLUDEPATH += $$(OPENCV_DIR)/../../include

LIBS += -L$$(OPENCV_DIR)/lib \
    -lopencv_core4110 \
    -lopencv_highgui4110 \
    -lopencv_imgproc4110 \
    -lopencv_imgcodecs4110 \
    -lopencv_videoio4110 \
    -lopencv_features2d4110 \
    -lopencv_calib3d4110 \
    -lopencv_objdetect4110 \
    -lopencv_video4110 \
    -lopencv_flann4110


RESOURCES += resources.qrc # Comment out or remove if resources.qrc is not used or does not exist
