#include <QApplication>
#include "MainWindow.h"
#include "CalibrationWindow.h"
#include "PalmTracker.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    // First show the calibration window
    CalibrationWindow calibrationWindow;
    
    // If calibration is successful, open the main window
    MainWindow* mainWindow = nullptr;
    PalmTracker* palmTracker = nullptr;
    
    QObject::connect(&calibrationWindow, &CalibrationWindow::calibrationFinished, 
                   [&](bool success) {
        if (success) {
            // Create palm tracker with calibration data
            palmTracker = new PalmTracker();
            palmTracker->setCalibrationData(
                calibrationWindow.getPalmRegion(),
                calibrationWindow.getKeypoints(),
                calibrationWindow.getDescriptors()
            );
            
            // Create and show main window
            mainWindow = new MainWindow();
            
            // Connect the WebcamHandler in MainWindow to the PalmTracker
            // You'll need to expose WebcamHandler in MainWindow and add a method
            // to pass frames to PalmTracker. This part depends on your MainWindow
            // implementation.
            
            mainWindow->show();
        } else {
            // Exit application if calibration failed
            app.quit();
        }
    });
    
    // Execute the calibration window
    if (calibrationWindow.exec() != QDialog::Accepted) {
        // User closed or canceled calibration
        return 0;
    }
    
    return app.exec();
}
