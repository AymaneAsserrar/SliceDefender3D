/**
 * @file Projectile.h
 * @brief Définition de la classe Projectile pour les objets lancés dans le jeu
 * @author Aymane ASSERRAR + Marieme Benzha
 * @date Mai 2025
 */

#ifndef PROJECTILE_H
#define PROJECTILE_H

#include <QOpenGLFunctions>
#include <QOpenGLExtraFunctions>
#include <QVector3D>
#include <QMatrix4x4>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <vector>

/// Vitesse horizontale maximale par défaut
const float DEFAULT_MAX_HORIZONTAL_VELOCITY = 10.0f;  
/// Vitesse verticale maximale par défaut
const float DEFAULT_MAX_VERTICAL_VELOCITY = 12.0f;    
/// Vitesse horizontale maximale des fragments après découpe
const float FRAGMENT_MAX_HORIZONTAL_VELOCITY = 8.0f;  
/// Vitesse verticale maximale des fragments après découpe
const float FRAGMENT_MAX_VERTICAL_VELOCITY = 10.0f;   

class QOpenGLShaderProgram;

/**
 * @class Projectile
 * @brief Classe représentant un objet en mouvement dans le jeu
 * 
 * Gère le rendu, la physique et les collisions des objets lancés
 * vers le joueur (fruits et autres projectiles).
 */
class Projectile : public QOpenGLFunctions {
public:
    /**
     * @brief Types de projectiles disponibles
     */
    enum class Type {
        BANANA,  ///< Banane
        APPLE,   ///< Pomme
        ANANAS,  ///< Ananas
        FRAISE,  ///< Fraise
        WOOD_CUBE ///< Cube en bois
    };

    /**
     * @brief Constructeur
     * @param type Type du projectile
     * @param position Position initiale en 3D
     * @param velocity Vecteur vitesse initial
     * 
     * Initialise un nouveau projectile avec sa position et sa vitesse
     */
    Projectile(Type type, const QVector3D& position, const QVector3D& velocity);
    
    /**
     * @brief Destructeur
     * 
     * Libère les ressources OpenGL associées
     */
    ~Projectile();

    /**
     * @brief Obtient la position du projectile
     * @return Position actuelle en 3D
     */
    QVector3D position() const { return m_position; }
    
    /**
     * @brief Obtient la vitesse du projectile
     * @return Vecteur vitesse actuel
     */
    QVector3D velocity() const { return m_velocity; }
    
    /**
     * @brief Vérifie si le projectile est actif
     * @return true si le projectile est actif, false sinon
     */
    bool isActive() const { return m_active; }
    
    /**
     * @brief Obtient le type du projectile
     * @return Type du projectile
     */
    Type type() const { return m_type; }
    
    /**
     * @brief Obtient l'angle de rotation actuel
     * @return Angle de rotation en degrés
     */
    float rotationAngle() const { return m_rotationAngle; }

    /**
     * @brief Met à jour la position et la rotation du projectile
     * @param deltaTime Temps écoulé depuis la dernière mise à jour en secondes
     * 
     * Applique la physique (vitesse, gravité) et calcule la nouvelle position
     */
    void update(float deltaTime);
    
    /**
     * @brief Dessine le projectile
     * @param shaderProgram Programme shader à utiliser
     * @param projection Matrice de projection
     * @param view Matrice de vue
     * 
     * Effectue le rendu du projectile avec sa texture appropriée
     */
    void render(QOpenGLShaderProgram* shaderProgram, const QMatrix4x4& projection, const QMatrix4x4& view);
    
    /**
     * @brief Dessine l'ombre du projectile
     * @param shaderProgram Programme shader à utiliser
     * @param projection Matrice de projection
     * @param view Matrice de vue
     * @param groundLevel Hauteur du sol
     */
    void renderShadow(QOpenGLShaderProgram* shaderProgram, const QMatrix4x4& projection, const QMatrix4x4& view, float groundLevel);
    
    /**
     * @brief Vérifie la collision avec un cylindre (épée)
     * @param radius Rayon du cylindre
     * @param height Hauteur du cylindre
     * @param cylinderPosition Position du cylindre
     * @return true s'il y a collision, false sinon
     */
    bool checkCollisionWithCylinder(float radius, float height, const QVector3D& cylinderPosition);
    
    /**
     * @brief Divise le projectile en fragments
     * @return Vecteur contenant les fragments résultants
     * 
     * Simule la découpe du projectile en plusieurs morceaux
     */
    std::vector<Projectile> slice();

    void initializeGL();

    bool isFragment() const { return m_isFragment; }

    void markForGameOver() { m_causedGameOver = true; }
    bool causedGameOver() const { return m_causedGameOver; }

    void applyGravity(float deltaTime) {

        m_velocity.setY(m_velocity.y() - 2.0f * deltaTime); 

        limitVelocity();
    }

    void limitVelocity(float maxHorizontal = DEFAULT_MAX_HORIZONTAL_VELOCITY, 
                       float maxVertical = DEFAULT_MAX_VERTICAL_VELOCITY) {

        float horizontalSpeed = std::sqrt(m_velocity.x() * m_velocity.x() + m_velocity.z() * m_velocity.z());
        if (horizontalSpeed > maxHorizontal) {
            float scaleFactor = maxHorizontal / horizontalSpeed;
            m_velocity.setX(m_velocity.x() * scaleFactor);
            m_velocity.setZ(m_velocity.z() * scaleFactor);
        }

        if (std::abs(m_velocity.y()) > maxVertical) {
            m_velocity.setY(m_velocity.y() > 0 ? maxVertical : -maxVertical);
        }
    }

private:

    void renderBanana(QOpenGLShaderProgram* shaderProgram);
    void renderApple(QOpenGLShaderProgram* shaderProgram);
    void renderAnanas(QOpenGLShaderProgram* shaderProgram);
    void renderFraise(QOpenGLShaderProgram* shaderProgram);
    void renderWoodCube(QOpenGLShaderProgram* shaderProgram);

    void renderBananaShadow(QOpenGLShaderProgram* shaderProgram);
    void renderAppleShadow(QOpenGLShaderProgram* shaderProgram);
    void renderAnanasShadow(QOpenGLShaderProgram* shaderProgram);
    void renderFraiseShadow(QOpenGLShaderProgram* shaderProgram);
    void renderWoodCubeShadow(QOpenGLShaderProgram* shaderProgram);

    void generateCutSurface(const QVector3D& sliceNormal, float direction);

    Type m_type;
    QVector3D m_position;
    QVector3D m_velocity;
    float m_rotationAngle;
    QVector3D m_rotationAxis;
    bool m_active;
    float m_scale;

    QOpenGLTexture* m_texture;
    bool m_hasTexture;
    GLuint m_vao;
    GLuint m_vbo;
    GLuint m_ebo;
    bool m_initialized;
    bool m_isFragment;  

    int m_fragmentSide = 0;         
    QVector3D m_sliceNormal;        

    std::vector<QVector3D> m_cutVertices;
    QVector3D m_cutSurfaceColor;

    bool m_causedGameOver = false;  

    void applyFragmentCutPlane(std::vector<GLfloat>& vertices, std::vector<GLuint>& indices);

    static constexpr float GRAVITY = 8.5f; 

    bool checkPointInCylinder(const QVector3D& point, float radius, float height, const QVector3D& cylinderPosition);
};

#endif 