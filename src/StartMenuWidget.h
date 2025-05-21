#ifndef STARTMENUWIDGET_H
#define STARTMENUWIDGET_H

#include <QWidget>

class QPushButton;
class QLabel;

class StartMenuWidget : public QWidget {
    Q_OBJECT
public:
    explicit StartMenuWidget(QWidget* parent = nullptr);

signals:
    // true = contrôle main, false = contrôle souris
    void controlModeSelected(bool useHandControl);

private slots:
    void onHandButtonClicked();
    void onMouseButtonClicked();

private:
    QLabel* titleLabel;
    QPushButton* handButton;
    QPushButton* mouseButton;

    void setupUi();
};

#endif // STARTMENUWIDGET_H
