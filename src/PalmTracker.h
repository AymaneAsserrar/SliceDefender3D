/**
 * @file PalmTracker.h
 * @brief Suivi des mouvements de la paume de la main
 * @author Aymane ASSERRAR + Marieme Benzha
 * @date Mai 2025
 */

#pragma once

#include <QObject>
#include <QPointF>  
#include <opencv2/opencv.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/flann.hpp>

/**
 * @class PalmTracker
 * @brief Classe pour le suivi des mouvements de la paume de la main
 * 
 * Cette classe utilise la détection et le suivi de points caractéristiques (features)
 * d'OpenCV pour suivre les mouvements de la paume de la main dans des images successives.
 */
class PalmTracker : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Constructeur
     * @param parent Pointeur vers l'objet parent (nullptr par défaut)
     * 
     * Initialise les détecteurs de caractéristiques et les extracteurs de descripteurs.
     */
    explicit PalmTracker(QObject *parent = nullptr);

    /**
     * @brief Définit les données de calibration pour le suivi
     * @param region Région d'intérêt contenant la paume
     * @param keypoints Points caractéristiques détectés dans la région
     * @param descriptors Descripteurs des points caractéristiques
     * 
     * Cette méthode enregistre les informations de référence qui seront utilisées
     * pour le suivi de la paume dans les images ultérieures.
     */
    void setCalibrationData(const cv::Rect& region, 
                           const std::vector<cv::KeyPoint>& keypoints,
                           const cv::Mat& descriptors);

    /**
     * @brief Tente de localiser la paume dans une image
     * @param frame Image dans laquelle chercher la paume
     * @return true si la paume a été localisée avec succès, false sinon
     * 
     * Algorithme:
     * 1. Détecte les points caractéristiques dans l'image
     * 2. Extrait leurs descripteurs
     * 3. Fait correspondre ces descripteurs avec ceux de calibration
     * 4. Estime la position actuelle de la paume
     * 5. Met à jour la position normalisée
     */
    bool trackPalm(const cv::Mat& frame);

    /**
     * @brief Renvoie la position normalisée actuelle de la paume
     * @return Position normalisée (x,y dans l'intervalle [0,1])
     */
    QPointF getNormalizedPosition() const;

signals:
    /**
     * @brief Signal émis lorsque la position de la paume change
     * @param normX Coordonnée X normalisée (entre 0 et 1)
     * @param normY Coordonnée Y normalisée (entre 0 et 1)
     */
    void palmPositionChanged(float normX, float normY);

private:
    cv::Rect palmRegion;            ///< Région d'intérêt contenant la paume
    std::vector<cv::KeyPoint> calibrationKeypoints;  ///< Points caractéristiques de référence
    cv::Mat calibrationDescriptors; ///< Descripteurs des points caractéristiques de référence
    bool isInitialized = false;     ///< Indicateur de l'état d'initialisation

    cv::Ptr<cv::FeatureDetector> featureDetector;        ///< Détecteur de points caractéristiques
    cv::Ptr<cv::DescriptorExtractor> descriptorExtractor; ///< Extracteur de descripteurs
    cv::Ptr<cv::FlannBasedMatcher> flannMatcher;         ///< Algorithme de mise en correspondance

    cv::Point2f currentPosition;    ///< Position actuelle en pixels
    QPointF normalizedPosition;     ///< Position normalisée entre 0 et 1
    cv::Size lastFrameSize;         ///< Taille de la dernière image traitée
};