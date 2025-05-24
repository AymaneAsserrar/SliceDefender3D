/**
 * @file WebcamHandler.h
 * @brief Gestion de la webcam et détection des mains pour le jeu Slice Defender 3D
 * @author Aymane ASSERRAR + Marieme Benzha
 * @date Mai 2025
 */

#ifndef WEBCAMHANDLER_H
#define WEBCAMHANDLER_H

#include <QObject>
#include <QThread>
#include <QImage>
#include <QPoint>
#include <opencv2/opencv.hpp>

/**
 * @class WebcamHandler
 * @brief Classe pour gérer l'accès à la webcam et la détection des mains
 * 
 * Cette classe capture les images de la webcam, détecte les paumes des mains
 * à l'aide de OpenCV et émet des signaux contenant les images capturées
 * et les positions des mains détectées.
 */
class WebcamHandler : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Constructeur
     * @param parent Pointeur vers l'objet parent (nullptr par défaut)
     * 
     * Initialise les ressources nécessaires, notamment le fichier XML pour
     * la détection des paumes et configure le thread de travail.
     */
    explicit WebcamHandler(QObject *parent = nullptr);
    
    /**
     * @brief Destructeur
     * 
     * Libère les ressources et arrête proprement le thread de travail.
     */
    ~WebcamHandler();

    /**
     * @brief Démarre la capture vidéo
     * 
     * Ouvre la webcam et lance le traitement des images dans un thread séparé.
     */
    void startCamera();
    
    /**
     * @brief Arrête la capture vidéo
     * 
     * Arrête le traitement des images et libère la webcam.
     */
    void stopCamera();

signals:
    /**
     * @brief Signal émis lorsqu'une image est prête
     * @param frame Image capturée et redimensionnée
     */
    void frameReady(const QImage &frame);
    
    /**
     * @brief Signal émis lorsqu'une main est détectée
     * @param center Point central de la main détectée
     */
    void handDetected(const QPoint &center);

private:
    /**
     * @brief Traite les images capturées par la webcam
     * 
     * Cette méthode s'exécute dans un thread séparé et effectue :
     * - Capture d'image depuis la webcam
     * - Conversion en niveaux de gris
     * - Détection des paumes avec OpenCV
     * - Dessin des rectangles autour des paumes détectées
     * - Émission des signaux avec l'image et la position des mains
     */
    void processFrame();
    
    /**
     * @brief Démarre le traitement dans le thread de travail
     */
    void startProcessing();

    QThread workerThread;           ///< Thread séparé pour le traitement des images
    cv::VideoCapture cap;           ///< Objet OpenCV pour la capture vidéo
    cv::CascadeClassifier palmCascade; ///< Classificateur en cascade pour détecter les paumes
    bool running;                   ///< Indicateur d'état de fonctionnement
};

#endif 