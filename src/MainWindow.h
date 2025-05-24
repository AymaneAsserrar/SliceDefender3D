/**
 * @file MainWindow.h
 * @brief Fenêtre principale de l'application Slice Defender 3D
 * @author Aymane ASSERRAR + Marieme Benzha
 * @date Mai 2025
 */

#include <QMainWindow>
#include <QLabel>
#include <QTimer>
#include <QMessageBox>

class WebcamHandler;
class OpenGLWidget;

/**
 * @class MainWindow
 * @brief Classe principale qui gère l'interface utilisateur du jeu
 * 
 * Cette classe coordonne l'affichage de la webcam, la zone de jeu OpenGL,
 * le score et le temps restant. Elle établit également les connexions
 * entre les différents composants du jeu.
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    /**
     * @brief Constructeur
     * @param parent Pointeur vers l'objet parent (nullptr par défaut)
     * 
     * Initialise l'interface utilisateur, le gestionnaire de webcam et le widget OpenGL.
     * Configure également les connexions entre les signaux et les slots.
     */
    explicit MainWindow(QWidget *parent = nullptr);
    
    /**
     * @brief Destructeur
     * 
     * Libère les ressources allouées
     */
    ~MainWindow();

    /**
     * @brief Incrémente le score du joueur
     * 
     * Est appelée lorsqu'un projectile est tranché avec succès
     */
    void incrementScore();

protected:
    /**
     * @brief Gère l'événement de fermeture de la fenêtre
     * @param event Événement de fermeture
     * 
     * S'assure que les ressources sont correctement libérées lors de la fermeture
     */
    void closeEvent(QCloseEvent *event) override;

private slots:
    /**
     * @brief Met à jour l'affichage de la webcam
     * @param frame Image capturée par la webcam
     */
    void updateCameraView(const QImage &frame);
    
    /**
     * @brief Gère la détection d'une main dans l'image
     * @param center Position centrale de la main détectée
     */
    void onHandDetected(const QPoint &center);

    /**
     * @brief Met à jour l'affichage du temps de jeu
     * 
     * Décompte le temps restant et met à jour l'interface
     */
    void updateGameTime();
    
    /**
     * @brief Termine la partie
     * 
     * Affiche le score final et met le jeu en pause
     */
    void endGame();

private:
    WebcamHandler *webcamHandler;  ///< Gestionnaire de la webcam
    OpenGLWidget *openglWidget;    ///< Widget OpenGL pour le rendu du jeu
    QLabel *cameraLabel;           ///< Étiquette pour l'affichage de la webcam
    QLabel *scoreLabel;            ///< Étiquette pour l'affichage du score
    QLabel *timeLabel;             ///< Étiquette pour l'affichage du temps

    int score = 0;                 ///< Score actuel du joueur
    int elapsedTime = 0;           ///< Temps écoulé depuis le début de la partie
    QTimer *gameTimer;             ///< Minuteur pour le décompte du temps

    const int gameDuration = 120;  ///< Durée totale d'une partie en secondes
};