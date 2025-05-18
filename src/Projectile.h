#ifndef PROJECTILE_H
#define PROJECTILE_H

#include <QOpenGLFunctions>
#include <QOpenGLExtraFunctions>
#include <QVector3D>
#include <QMatrix4x4>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <vector>

// Forward declarations
class QOpenGLShaderProgram;

class Projectile : public QOpenGLFunctions {
public:
    // Projectile types
    enum class Type {
        BANANA,
        APPLE,
        ANANAS  // Pineapple
    };

    // Constructor
    Projectile(Type type, const QVector3D& position, const QVector3D& velocity);
    ~Projectile();

    // Properties
    QVector3D position() const { return m_position; }
    QVector3D velocity() const { return m_velocity; }
    bool isActive() const { return m_active; }
    Type type() const { return m_type; }
    float rotationAngle() const { return m_rotationAngle; }

    // Main functionality
    void update(float deltaTime);
    void render(QOpenGLShaderProgram* shaderProgram, const QMatrix4x4& projection, const QMatrix4x4& view);
    bool checkCollisionWithCylinder(float radius, float height, const QVector3D& cylinderPosition);
    std::vector<Projectile> slice();

    // Initialize OpenGL resources
    void initializeGL();

    // Add method to check if this is a fragment
    bool isFragment() const { return m_isFragment; }

    void applyGravity(float deltaTime) {
        // Apply gravity directly to the velocity vector
        m_velocity.setY(m_velocity.y() - 2.5f * deltaTime); // Gravity constant
    }

private:
    // Rendering helpers
    void renderBanana(QOpenGLShaderProgram* shaderProgram);
    void renderApple(QOpenGLShaderProgram* shaderProgram);
    void renderAnanas(QOpenGLShaderProgram* shaderProgram);  // Add this line

    // Properties
    Type m_type;
    QVector3D m_position;
    QVector3D m_velocity;
    float m_rotationAngle;
    QVector3D m_rotationAxis;
    bool m_active;
    float m_scale;
    
    // Reorder these declarations to match initialization order
    GLuint m_vao;
    GLuint m_vbo;
    GLuint m_ebo;
    bool m_initialized;
    bool m_isFragment = false;  // Flag to track if this is a fragment
    
    // Fragment-specific properties
    int m_fragmentSide = 0;         // 1 for positive half, -1 for negative half
    QVector3D m_sliceNormal;        // Normal vector of the slice plane
    
    // Helper for fragment rendering
    void applyFragmentCutPlane(std::vector<GLfloat>& vertices, std::vector<GLuint>& indices);
    
    // Physics constants
    static constexpr float GRAVITY = 9.8f;

    // Collision helpers
    bool checkPointInCylinder(const QVector3D& point, float radius, float height, const QVector3D& cylinderPosition);
};

#endif // PROJECTILE_H
