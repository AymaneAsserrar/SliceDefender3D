QT += core gui widgets openglwidgets concurrent

CONFIG += c++17
SOURCES += \
    src/StrartMenuWidget.cpp \
    src/main.cpp \
    src/MainWindow.cpp \
    src/OpenGLWidget.cpp \
    src/WebcamHandler.cpp \
    src/Projectile.cpp \
    src/PalmDetection.cpp \
    src/CalibrationWindow.cpp \
    src/PalmTracker.cpp

HEADERS += \
    src/MainWindow.h \
    src/OpenGLWidget.h \
    src/StartMenuWidget.h \
    src/WebcamHandler.h \
    src/Projectile.h \
    src/CalibrationWindow.h \
    src/PalmTracker.h

# OpenCV

#INCLUDEPATH += $$(OPENCV_DIR)/../../include



INCLUDEPATH += C:/opencv/build/install/include


LIBS += -L"C:/opencv/build/install/x64/mingw/lib" \
    -lopencv_core490 \
    -lopencv_highgui490 \
    -lopencv_imgproc490 \
    -lopencv_imgcodecs490 \
    -lopencv_videoio490 \
    -lopencv_features2d490 \
    -lopencv_calib3d490 \
    -lopencv_objdetect490 \
    -lopencv_video490 \
    -lopencv_flann490


# RESOURCES += resources.qrc # Comment out or remove if resources.qrc is not used or does not exist

RESOURCES += \
    resources.qrc

DISTFILES += \
    resources/.qtcreator/project.json \
    resources/images/Banana_texture.png \
    resources/images/apple_texture.jpg
