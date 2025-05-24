/**
 * @file OpenGLWidget.h
 * @brief Widget OpenGL pour le rendu 3D du jeu Slice Defender
 * @author Aymane ASSERRAR + Marieme Benzha
 * @date Mai 2025
 */

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QVector>
#include <QVector3D>
#include <QElapsedTimer>
#include "Projectile.h"

#include <opencv2/opencv.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/flann.hpp>

/**
 * @class OpenGLWidget
 * @brief Widget OpenGL qui gère le rendu 3D du jeu
 * 
 * Cette classe est responsable du rendu 3D, de la physique du jeu,
 * de la génération des projectiles, de la détection des collisions
 * et de la gestion des entrées utilisateur via la webcam.
 */
class OpenGLWidget : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

public:
    /**
     * @brief Constructeur
     * @param parent Pointeur vers l'objet parent (nullptr par défaut)
     * 
     * Initialise les ressources OpenGL et les variables du jeu
     */
    OpenGLWidget(QWidget *parent = nullptr);
    
    /**
     * @brief Destructeur
     * 
     * Libère les ressources OpenGL et les textures
     */
    ~OpenGLWidget();

    /**
     * @brief Définit la position de la main en coordonnées normalisées
     * @param normX Coordonnée X normalisée (entre 0 et 1)
     * @param normY Coordonnée Y normalisée (entre 0 et 1)
     * 
     * Convertit les coordonnées normalisées en position 3D
     */
    void setHandPosition(float normX, float normY);
    
    /**
     * @brief Définit la position de la main en 3D
     * @param position Vecteur position 3D
     */
    void setHandPosition(const QVector3D& position);
    
    /**
     * @brief Réinitialise l'état du jeu
     * 
     * Remet à zéro le score, supprime les projectiles et réinitialise le temps
     */
    void resetGame(); 

    /**
     * @brief Calibre la détection de la paume
     * @param frame Image de la webcam pour la calibration
     * @return true si la calibration a réussi, false sinon
     * 
     * Détecte la paume dans l'image et extrait ses caractéristiques pour le suivi
     */
    bool calibratePalmDetection(const cv::Mat& frame);
    
    /**
     * @brief Traite la détection de la paume en continu
     * @param frame Image actuelle de la webcam
     * @return true si la paume a été détectée, false sinon
     * 
     * Utilise les données de calibration pour suivre la paume dans l'image
     */
    bool processPalmDetection(const cv::Mat& frame);

    /**
     * @brief Réinitialise la caméra virtuelle
     * 
     * Remet la caméra dans sa position et orientation initiales
     */
    void resetCamera();

signals:
    /**
     * @brief Signal émis lorsque le score augmente
     * 
     * Émis quand un projectile est tranché avec succès
     */
    void scoreIncreased(); 
    
    /**
     * @brief Signal émis à la fin de la calibration
     * @param success Indique si la calibration a réussi
     */
    void calibrationComplete(bool success); 
    
    /**
     * @brief Signal émis lorsque la partie est terminée
     * 
     * Émis quand un cube en bois touche l'épée ou quand le temps est écoulé
     */
    void gameOver(); 

protected:
    /**
     * @brief Initialise le contexte OpenGL
     * 
     * Configure les shaders, les buffers, les textures et l'état OpenGL
     */
    void initializeGL() override;
    
    /**
     * @brief Gère le redimensionnement du widget
     * @param w Nouvelle largeur
     * @param h Nouvelle hauteur
     * 
     * Met à jour la matrice de projection
     */
    void resizeGL(int w, int h) override;
    
    /**
     * @brief Effectue le rendu de la scène
     * 
     * Dessine tous les éléments du jeu (épée, projectiles, environnement)
     */
    void paintGL() override;
    
    /**
     * @brief Gère les événements de minuterie
     * @param event Événement de minuterie
     * 
     * Met à jour la physique et la logique du jeu à intervalles réguliers
     */
    void timerEvent(QTimerEvent* event) override;
    
    /**
     * @brief Gère les événements d'appui sur les touches
     * @param event Événement de clavier
     * 
     * Permet le contrôle de la caméra via le clavier
     */
    void keyPressEvent(QKeyEvent* event) override;
    
    /**
     * @brief Gère les événements de relâchement des touches
     * @param event Événement de clavier
     */
    void keyReleaseEvent(QKeyEvent* event) override;

private:
    // Position et état de la main/épée
    float handX = 0.0f, handY = 0.0f, handZ = 0.0f;  ///< Position de la main en 3D
    float normHandX = 0.0f, normHandY = 0.0f;        ///< Position normalisée de la main
    QVector3D handPosition;                          ///< Position vectorielle de la main
    float cylinderRadius = 1.5f;                     ///< Rayon du cylindre (épée)
    float cylinderHeight = 2.0f;                     ///< Hauteur du cylindre (épée)
    bool handSet = false;                            ///< Indique si la position de la main est définie

    // Ressources OpenGL
    QOpenGLShaderProgram *shaderProgram;            ///< Programme shader pour le rendu
    QOpenGLBuffer vbo;                              ///< Vertex Buffer Object pour les données de géométrie

    // Gestion des projectiles
    QVector<Projectile> projectiles;                ///< Projectiles actifs
    QVector<Projectile> pendingProjectiles;         ///< Projectiles en attente d'ajout
    
    /**
     * @brief Génère un nouveau projectile
     * 
     * Crée un projectile avec une position et une vitesse aléatoires
     */
    void spawnProjectile();
    
    /**
     * @brief Met à jour la position et l'état des projectiles
     * @param deltaTime Temps écoulé depuis la dernière mise à jour
     * 
     * Applique la physique et supprime les projectiles inactifs
     */
    void updateProjectiles(float deltaTime);
    
    /**
     * @brief Vérifie les collisions entre l'épée et les projectiles
     * 
     * Détecte si l'épée a touché un projectile et le découpe si c'est le cas
     */
    void checkCollisions();
    
    /**
     * @brief Dessine la zone de génération des projectiles
     * 
     * Représentation visuelle de la zone d'où proviennent les projectiles
     */
    void drawSpawningZone(); 

    // Gestion du temps et de l'état du jeu
    int timerId;                                   ///< ID du timer pour les mises à jour régulières
    QElapsedTimer elapsedTimer;                    ///< Chronomètre pour mesurer le temps écoulé
    float gameTime = 0.0f;                         ///< Temps total de jeu en secondes
    float deltaTime = 0.0f;                        ///< Temps écoulé entre deux frames
    float lastSpawnTime = 0.0f;                    ///< Moment du dernier spawn de projectile
    float spawnInterval = 2.0f;                    ///< Intervalle entre deux spawns de projectiles
    int score = 0;                                 ///< Score actuel du joueur

    bool isGameRunning = true;                     ///< Indique si le jeu est en cours
    bool gameOverEffect = false;                   ///< Indique si l'effet de fin de jeu est actif
    float gameOverEffectTime = 0.0f;               ///< Temps écoulé depuis le début de l'effet de fin de jeu

    // Méthodes de rendu
    /**
     * @brief Dessine un cylindre (l'épée)
     * 
     * Crée la géométrie d'un cylindre et le rend avec les textures appropriées
     */
    void drawCylinder();
    
    /**
     * @brief Dessine une sphère
     * @param radius Rayon de la sphère
     * @param lats Nombre de latitudes
     * @param longs Nombre de longitudes
     * 
     * Crée la géométrie d'une sphère avec le niveau de détail spécifié
     */
    void drawSphere(float radius, int lats, int longs);
    
    /**
     * @brief Dessine l'épée
     * 
     * Rend l'épée complète (lame et poignée)
     */
    void drawSword(); 
    
    /**
     * @brief Dessine l'ombre de l'épée
     * 
     * Projette une ombre de l'épée sur le sol
     */
    void drawSwordShadow(); 
    
    /**
     * @brief Dessine le sol
     * 
     * Rend le sol avec sa texture
     */
    void drawGround();  
    
    /**
     * @brief Dessine les murs
     * 
     * Rend les murs avec leurs textures
     */
    void drawWalls();  
    
    /**
     * @brief Dessine le plafond
     * 
     * Rend le plafond avec sa texture
     */
    void drawRoof();  
    
    /**
     * @brief Dessine une source de lumière
     * @param position Position de la source de lumière
     * 
     * Représentation visuelle d'une source de lumière dans la scène
     */
    void drawLightSource(const QVector3D& position); 

    // Textures
    QOpenGLTexture* bladeTexture = nullptr;        ///< Texture pour la lame de l'épée
    QOpenGLTexture* handleTexture = nullptr;       ///< Texture pour la poignée de l'épée
    QOpenGLTexture* groundTexture = nullptr;       ///< Texture pour le sol
    QOpenGLTexture* wallTexture = nullptr;         ///< Texture pour les murs
    QOpenGLTexture* backWallTexture = nullptr;     ///< Texture pour le mur du fond
    QOpenGLTexture* roofTexture = nullptr;         ///< Texture pour le plafond

    // Matrices de transformation
    QMatrix4x4 projection;                         ///< Matrice de projection
    QMatrix4x4 view;                               ///< Matrice de vue

    // Détection et suivi de la paume
    cv::CascadeClassifier palmCascade;             ///< Classificateur en cascade pour la détection des paumes
    bool palmCascadeLoaded = false;                ///< Indique si le classificateur a été chargé
    bool isCalibrated = false;                     ///< Indique si la calibration a été effectuée
    cv::Rect calibratedPalmRegion;                 ///< Région contenant la paume calibrée

    cv::Ptr<cv::FeatureDetector> featureDetector;         ///< Détecteur de points caractéristiques
    cv::Ptr<cv::DescriptorExtractor> descriptorExtractor; ///< Extracteur de descripteurs
    cv::Ptr<cv::FlannBasedMatcher> flannMatcher;          ///< Algorithme de mise en correspondance
    std::vector<cv::KeyPoint> calibrationKeypoints;       ///< Points caractéristiques de référence
    cv::Mat calibrationDescriptors;                       ///< Descripteurs des points caractéristiques de référence

    /**
     * @brief Initialise les détecteurs de paume
     * @return true si l'initialisation a réussi, false sinon
     * 
     * Charge le classificateur en cascade et initialise les détecteurs de caractéristiques
     */
    bool initializePalmDetection();
    
    /**
     * @brief Suit le mouvement de la paume
     * @param frame Image actuelle de la webcam
     * @return Position 2D de la paume
     * 
     * Utilise l'algorithme de suivi des caractéristiques pour localiser la paume
     */
    cv::Point2f trackPalmMovement(const cv::Mat& frame);
    
    /**
     * @brief Convertit la position de la paume en position de la main dans le jeu
     * @param palmPosition Position 2D de la paume
     * @param frameSize Dimensions de l'image
     * 
     * Transforme les coordonnées 2D en position 3D pour l'épée
     */
    void convertToHandPosition(const cv::Point2f& palmPosition, const cv::Size& frameSize);

    // Camera
    QVector3D cameraPosition;                      ///< Position de la caméra
    float cameraYaw = 0.0f;                        ///< Rotation horizontale de la caméra
    float cameraPitch = 0.0f;                      ///< Rotation verticale de la caméra
    float cameraDistance = 5.0f;                   ///< Distance de la caméra au centre de la scène
    bool keysPressed[4] = {false};                 ///< État des touches de contrôle de la caméra

    float cameraRotationSpeed = 70.0f;             ///< Vitesse de rotation de la caméra
    float cameraMoveSpeed = 2.0f;                  ///< Vitesse de déplacement de la caméra

    /**
     * @brief Met à jour la position et l'orientation de la caméra
     * 
     * Applique les mouvements de caméra en fonction des touches pressées
     */
    void updateCamera();

    QOpenGLBuffer zoneVBO;                         ///< Buffer pour la zone de génération des projectiles
};