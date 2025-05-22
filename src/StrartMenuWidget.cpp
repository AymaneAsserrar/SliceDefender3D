#include "StartMenuWidget.h"
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QFont>

#include <QUrl>


StartMenuWidget::StartMenuWidget(QWidget* parent) : QWidget(parent) {
    setupUi();
}

void StartMenuWidget::setupUi() {
    // Layout vertical principal
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(30);
    mainLayout->setContentsMargins(50, 50, 50, 50);

    // Label titre avec style simple
    titleLabel = new QLabel(tr("Mon Super Jeu"), this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(36);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);

    // Bouton contrôle main
    handButton = new QPushButton(tr("Contrôle avec la main"), this);
    handButton->setFixedHeight(50);
    handButton->setStyleSheet("font-size: 18px;");

    // Bouton contrôle souris
    mouseButton = new QPushButton(tr("Contrôle avec la souris"), this);
    mouseButton->setFixedHeight(50);
    mouseButton->setStyleSheet("font-size: 18px;");

    // Ajouter widgets au layout
    mainLayout->addWidget(titleLabel);
    mainLayout->addStretch();
    mainLayout->addWidget(handButton);
    mainLayout->addWidget(mouseButton);
    mainLayout->addStretch();

    // Connexions signaux -> slots
    connect(handButton, &QPushButton::clicked, this, &StartMenuWidget::onHandButtonClicked);
    connect(mouseButton, &QPushButton::clicked, this, &StartMenuWidget::onMouseButtonClicked);

    // Style de base du widget (couleur fond)
    setStyleSheet("background-color: qlineargradient(x1:0, y1:0, x2:1, y2:1,"
                  "stop:0 #4caf50, stop:1 #087f23);"
                  "color: white;");
}

void StartMenuWidget::onHandButtonClicked() {
    emit controlModeSelected(true);
}

void StartMenuWidget::onMouseButtonClicked() {
    emit controlModeSelected(false);
}
