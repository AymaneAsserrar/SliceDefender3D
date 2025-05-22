#ifndef PROJECTILE_H
#define PROJECTILE_H

#include <QOpenGLFunctions>
#include <QOpenGLExtraFunctions>
#include <QVector3D>
#include <QMatrix4x4>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <vector>

class Projectile : public QOpenGLFunctions {
public:
    enum class Type {
        BANANA,
        APPLE,
        ANANAS,
        FRAISE
    };

    Projectile(Type type, const QVector3D& position, const QVector3D& velocity);
    ~Projectile();

    QVector3D position() const { return m_position; }
    QVector3D velocity() const { return m_velocity; }
    bool isActive() const { return m_active; }
    Type type() const { return m_type; }
    float rotationAngle() const { return m_rotationAngle; }

    void update(float deltaTime);
    void render(QOpenGLShaderProgram* shaderProgram,
                const QMatrix4x4& projection,
                const QMatrix4x4& view,
                const QVector3D& cameraPosition);

    bool checkCollisionWithCylinder(float radius, float height, const QVector3D& cylinderPosition);
    std::vector<Projectile> slice();

    void initializeGL();

    bool isFragment() const { return m_isFragment; }

    void applyGravity(float deltaTime) {
        m_velocity.setY(m_velocity.y() - GRAVITY * deltaTime);
    }

private:
    // Rendering helpers with lighting parameters where needed
    void renderBanana(QOpenGLShaderProgram* shaderProgram,
                      const QMatrix4x4& model,
                      const QMatrix4x4& view,
                      const QMatrix4x4& projection,
                      const QVector3D& lightPos,
                      const QVector3D& viewPos,
                      const QVector3D& lightColor);
    void renderApple(QOpenGLShaderProgram* shaderProgram,
                     const QMatrix4x4& model,
                     const QMatrix4x4& view,
                     const QMatrix4x4& projection,
                     const QVector3D& lightPos,
                     const QVector3D& viewPos,
                     const QVector3D& lightColor);
    void renderAnanas(QOpenGLShaderProgram* shaderProgram,
                      const QMatrix4x4& model,
                      const QMatrix4x4& view,
                      const QMatrix4x4& projection,
                      const QVector3D& lightPos,
                      const QVector3D& viewPos,
                      const QVector3D& lightColor);
    void renderFraise(QOpenGLShaderProgram* shaderProgram,
                      const QMatrix4x4& model,
                      const QMatrix4x4& view,
                      const QMatrix4x4& projection,
                      const QVector3D& lightPos,
                      const QVector3D& viewPos,
                      const QVector3D& lightColor);

    // Internal render overload for simpler calls (if needed)
    void renderSimple(QOpenGLShaderProgram* shaderProgram,
                      const QMatrix4x4& projection,
                      const QMatrix4x4& view);

    // Helper to apply cut plane on fragments
    void applyFragmentCutPlane(std::vector<GLfloat>& vertices, std::vector<GLuint>& indices);

    // Members
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

    // Fragment-specific
    int m_fragmentSide;
    QVector3D m_sliceNormal;

    static constexpr float GRAVITY = 9.8f;

    bool checkPointInCylinder(const QVector3D& point, float radius, float height, const QVector3D& cylinderPosition);
};

#endif // PROJECTILE_H
