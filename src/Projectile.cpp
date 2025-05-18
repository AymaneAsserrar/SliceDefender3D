#include "Projectile.h"
#include <QtMath>
#include <QOpenGLContext>
#include <QRandomGenerator>

Projectile::Projectile(Type type, const QVector3D& position, const QVector3D& velocity)
    : m_type(type), 
      m_position(position), 
      m_velocity(velocity),
      m_rotationAngle(0.0f),
      m_rotationAxis(0.0f, 1.0f, 0.0f),
      m_active(true),
      m_scale(1.0f),
      m_vao(0),
      m_vbo(0),
      m_ebo(0),
      m_initialized(false),
      m_isFragment(false)
{
    // Random initial rotation
    m_rotationAngle = QRandomGenerator::global()->bounded(360);
    // Convert integer bounds to float with proper scaling
    m_rotationAxis = QVector3D(
        QRandomGenerator::global()->bounded(-100, 100) / 100.0f,
        QRandomGenerator::global()->bounded(-100, 100) / 100.0f,
        QRandomGenerator::global()->bounded(-100, 100) / 100.0f
    ).normalized();
}

Projectile::~Projectile() {
    if (m_initialized) {
        this->glDeleteBuffers(1, &m_vbo);
        this->glDeleteBuffers(1, &m_ebo);
        // Use QOpenGLExtraFunctions for this call
        if (QOpenGLContext::currentContext()) {
            QOpenGLContext::currentContext()->extraFunctions()->glDeleteVertexArrays(1, &m_vao);
        }
    }
}

void Projectile::initializeGL() {
    if (m_initialized) return;
    
    initializeOpenGLFunctions();
    
    // Create VAO and VBO
    // Use QOpenGLExtraFunctions for this call
    QOpenGLContext::currentContext()->extraFunctions()->glGenVertexArrays(1, &m_vao);
    this->glGenBuffers(1, &m_vbo);
    this->glGenBuffers(1, &m_ebo);
    
    m_initialized = true;
}

void Projectile::update(float deltaTime) {
    if (!m_active) return;
    
    // Update position based on velocity
    m_position += m_velocity * deltaTime;
    
    // Apply gravity
    m_velocity.setY(m_velocity.y() - GRAVITY * deltaTime);
    
    // Update rotation
    m_rotationAngle += 90.0f * deltaTime; // Rotate 90 degrees per second
    if (m_rotationAngle > 360.0f) {
        m_rotationAngle -= 360.0f;
    }
    
    // Deactivate if it falls below a certain threshold
    if (m_position.y() < -10.0f) {
        m_active = false;
    }
}

void Projectile::render(QOpenGLShaderProgram* shaderProgram, const QMatrix4x4& projection, const QMatrix4x4& view) {
    if (!m_active || !m_initialized) return;
    
    // Calculate model matrix
    QMatrix4x4 model;
    model.translate(m_position);
    model.rotate(m_rotationAngle, m_rotationAxis);
    model.scale(m_scale);
    
    // Set shader uniforms
    shaderProgram->setUniformValue("mvpMatrix", projection * view * model);
    shaderProgram->setUniformValue("modelMatrix", model);
    shaderProgram->setUniformValue("normalMatrix", model.normalMatrix());
    
    // Set color based on type
    QVector4D color;
    switch (m_type) {
        case Type::CUBE:
            color = QVector4D(1.0f, 0.0f, 0.0f, 1.0f); // Red
            break;
        case Type::PRISM:
            color = QVector4D(0.0f, 1.0f, 0.0f, 1.0f); // Green
            break;
        case Type::SPHERE:
            color = QVector4D(0.0f, 0.0f, 1.0f, 1.0f); // Blue
            break;
        case Type::CONE:
            color = QVector4D(1.0f, 1.0f, 0.0f, 1.0f); // Yellow
            break;
    }
    shaderProgram->setUniformValue("color", color);
    
    // Render based on projectile type
    switch (m_type) {
        case Type::CUBE:
            renderCube(shaderProgram);
            break;
        case Type::PRISM:
            renderPrism(shaderProgram);
            break;
        case Type::SPHERE:
            renderSphere(shaderProgram);
            break;
        case Type::CONE:
            renderCone(shaderProgram);
            break;
    }
}

bool Projectile::checkCollisionWithCylinder(float radius, float height, const QVector3D& cylinderPosition) {
    if (!m_active) return false;
    
    // Simple bounding sphere collision check
    float projectileRadius = 0.5f * m_scale;
    QVector3D cylinderCenter = cylinderPosition + QVector3D(0, 0, 0);
    
    // Check if projectile bounding sphere intersects with cylinder
    float distanceXZ = qSqrt(qPow(m_position.x() - cylinderCenter.x(), 2) + 
                            qPow(m_position.z() - cylinderCenter.z(), 2));
    
    // First, check if we're in the right X-Z radius
    if (distanceXZ > (radius + projectileRadius)) {
        return false;
    }
    
    // Now check height (Y) intersection
    float halfHeight = height / 2.0f;
    float cylinderTop = cylinderCenter.y() + halfHeight;
    float cylinderBottom = cylinderCenter.y() - halfHeight;
    
    if (m_position.y() + projectileRadius < cylinderBottom || 
        m_position.y() - projectileRadius > cylinderTop) {
        return false;
    }
    
    return true;
}

std::vector<Projectile> Projectile::slice() {
    if (!m_active) return {};
    
    m_active = false; // Deactivate the original projectile
    
    std::vector<Projectile> fragments;
    
    // Create 2-3 fragments with diverging velocities
    int fragmentCount = QRandomGenerator::global()->bounded(2, 4); // 2 or 3 fragments
    
    for (int i = 0; i < fragmentCount; ++i) {
        // Create fragment with slightly different position (convert to scaled float)
        QVector3D fragmentPos = m_position + QVector3D(
            QRandomGenerator::global()->bounded(-10, 10) / 100.0f,
            QRandomGenerator::global()->bounded(-10, 10) / 100.0f,
            QRandomGenerator::global()->bounded(-10, 10) / 100.0f
        );
        
        // Create diverging velocity
        float velocityFactor = 0.7f + QRandomGenerator::global()->bounded(60) / 100.0f; // 0.7 to 1.3
        QVector3D fragmentVel = m_velocity * velocityFactor;
        
        // Add some random divergence
        fragmentVel += QVector3D(
            QRandomGenerator::global()->bounded(-300, 300) / 100.0f,
            QRandomGenerator::global()->bounded(100, 500) / 100.0f, // Upward bias
            QRandomGenerator::global()->bounded(-300, 300) / 100.0f
        );
        
        // Create the fragment with same type but smaller scale
        Projectile fragment(m_type, fragmentPos, fragmentVel);
        fragment.m_scale = m_scale * 0.5f; // Make fragments smaller
        fragment.m_isFragment = true;  // Mark as a fragment
        fragment.initializeGL();
        
        fragments.push_back(fragment);
    }
    
    return fragments;
}

// Rendering implementations for different shapes

void Projectile::renderCube(QOpenGLShaderProgram* /* shaderProgram */) {
    // Define cube vertices
    static const GLfloat vertices[] = {
        // positions          // normals
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f
    };

    static const GLuint indices[] = {
        0, 1, 2, 2, 3, 0,       // Front face
        4, 5, 6, 6, 7, 4,       // Back face
        8, 9, 10, 10, 11, 8,    // Left face
        12, 13, 14, 14, 15, 12, // Right face
        16, 17, 18, 18, 19, 16, // Bottom face
        20, 21, 22, 22, 23, 20  // Top face
    };

    // Bind VAO and load data
    QOpenGLContext::currentContext()->extraFunctions()->glBindVertexArray(m_vao);
    
    this->glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    this->glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    this->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    this->glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    
    // Position attribute
    this->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    this->glEnableVertexAttribArray(0);
    
    // Normal attribute
    this->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    this->glEnableVertexAttribArray(1);
    
    // Draw the cube
    this->glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    
    // Unbind
    QOpenGLContext::currentContext()->extraFunctions()->glBindVertexArray(0);
}

void Projectile::renderPrism(QOpenGLShaderProgram* /* shaderProgram */) {
    // Define triangular prism vertices
    static const GLfloat vertices[] = {
        // Positions          // Normals
        // Base triangle 1
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f, 0.0f,
         0.0f, -0.5f,  0.5f,  0.0f, -1.0f, 0.0f,
        
        // Base triangle 2
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f, 0.0f,
         0.0f,  0.5f,  0.5f,  0.0f,  1.0f, 0.0f,
        
        // Side 1
        -0.5f, -0.5f, -0.5f, -0.5f,  0.0f, -0.866f,
        -0.5f,  0.5f, -0.5f, -0.5f,  0.0f, -0.866f,
         0.5f,  0.5f, -0.5f, -0.5f,  0.0f, -0.866f,
         0.5f, -0.5f, -0.5f, -0.5f,  0.0f, -0.866f,
        
        // Side 2
        -0.5f, -0.5f, -0.5f, -0.866f, 0.0f,  0.5f,
        -0.5f,  0.5f, -0.5f, -0.866f, 0.0f,  0.5f,
         0.0f,  0.5f,  0.5f, -0.866f, 0.0f,  0.5f,
         0.0f, -0.5f,  0.5f, -0.866f, 0.0f,  0.5f,
        
        // Side 3
         0.5f, -0.5f, -0.5f,  0.866f, 0.0f,  0.5f,
         0.5f,  0.5f, -0.5f,  0.866f, 0.0f,  0.5f,
         0.0f,  0.5f,  0.5f,  0.866f, 0.0f,  0.5f,
         0.0f, -0.5f,  0.5f,  0.866f, 0.0f,  0.5f
    };

    static const GLuint indices[] = {
        0, 1, 2,          // Base triangle 1
        3, 5, 4,          // Base triangle 2 (reversed for winding)
        6, 7, 8, 8, 9, 6, // Side 1
        10, 11, 12, 12, 13, 10, // Side 2
        14, 15, 16, 16, 17, 14  // Side 3
    };

    // Bind VAO and load data
    QOpenGLContext::currentContext()->extraFunctions()->glBindVertexArray(m_vao);
    
    this->glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    this->glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    this->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    this->glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    
    // Position attribute
    this->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    this->glEnableVertexAttribArray(0);
    
    // Normal attribute
    this->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    this->glEnableVertexAttribArray(1);
    
    // Draw the prism
    this->glDrawElements(GL_TRIANGLES, 24, GL_UNSIGNED_INT, 0);
    
    // Unbind
    QOpenGLContext::currentContext()->extraFunctions()->glBindVertexArray(0);
}

void Projectile::renderSphere(QOpenGLShaderProgram* /* shaderProgram */) {
    // Using GL_TRIANGLE_STRIP to render the sphere
    const int stacks = 16;
    const int slices = 32;
    const float radius = 0.5f;
    
    std::vector<GLfloat> vertices;
    std::vector<GLuint> indices;
    
    // Generate sphere vertices
    for (int i = 0; i <= stacks; ++i) {
        float phi = M_PI * float(i) / float(stacks);
        float sinPhi = std::sin(phi);
        float cosPhi = std::cos(phi);
        
        for (int j = 0; j <= slices; ++j) {
            float theta = 2.0f * M_PI * float(j) / float(slices);
            float sinTheta = std::sin(theta);
            float cosTheta = std::cos(theta);
            
            // Position
            float x = radius * sinPhi * cosTheta;
            float y = radius * cosPhi;
            float z = radius * sinPhi * sinTheta;
            
            // Normal
            float nx = sinPhi * cosTheta;
            float ny = cosPhi;
            float nz = sinPhi * sinTheta;
            
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
            vertices.push_back(nx);
            vertices.push_back(ny);
            vertices.push_back(nz);
        }
    }
    
    // Generate indices
    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < slices; ++j) {
            int first = i * (slices + 1) + j;
            int second = first + slices + 1;
            
            indices.push_back(first);
            indices.push_back(second);
            indices.push_back(first + 1);
            
            indices.push_back(second);
            indices.push_back(second + 1);
            indices.push_back(first + 1);
        }
    }
    
    // Bind VAO and load data
    QOpenGLContext::currentContext()->extraFunctions()->glBindVertexArray(m_vao);
    
    this->glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    this->glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);
    
    this->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    this->glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);
    
    // Position attribute
    this->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    this->glEnableVertexAttribArray(0);
    
    // Normal attribute
    this->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    this->glEnableVertexAttribArray(1);
    
    // Draw the sphere
    this->glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    
    // Unbind
    QOpenGLContext::currentContext()->extraFunctions()->glBindVertexArray(0);
}

void Projectile::renderCone(QOpenGLShaderProgram* /* shaderProgram */) {
    const int slices = 32;
    const float height = 1.0f;
    const float radius = 0.5f;
    
    std::vector<GLfloat> vertices;
    std::vector<GLuint> indices;
    
    // Apex vertex
    vertices.push_back(0.0f);           // x
    vertices.push_back(height * 0.5f);  // y
    vertices.push_back(0.0f);           // z
    vertices.push_back(0.0f);           // nx
    vertices.push_back(1.0f);           // ny
    vertices.push_back(0.0f);           // nz
    
    // Base center vertex
    vertices.push_back(0.0f);           // x
    vertices.push_back(-height * 0.5f); // y
    vertices.push_back(0.0f);           // z
    vertices.push_back(0.0f);           // nx
    vertices.push_back(-1.0f);          // ny
    vertices.push_back(0.0f);           // nz
    
    // Base vertices
    for (int i = 0; i <= slices; ++i) {
        float theta = 2.0f * M_PI * float(i) / float(slices);
        float cosTheta = std::cos(theta);
        float sinTheta = std::sin(theta);
        
        // Base vertex
        float x = radius * cosTheta;
        float y = -height * 0.5f;
        float z = radius * sinTheta;
        
        // Normal for sides (approximated)
        float sideNormalY = 0.5f;  // Approximate value
        float normalLength = std::sqrt(1.0f + sideNormalY * sideNormalY);
        float nx = cosTheta / normalLength;
        float ny = sideNormalY / normalLength;
        float nz = sinTheta / normalLength;
        
        // Add base vertex
        vertices.push_back(x);
        vertices.push_back(y);
        vertices.push_back(z);
        vertices.push_back(0.0f);  // Base normal
        vertices.push_back(-1.0f);
        vertices.push_back(0.0f);
        
        // Add side vertex (same position as base vertex)
        vertices.push_back(x);
        vertices.push_back(y);
        vertices.push_back(z);
        vertices.push_back(nx);  // Side normal
        vertices.push_back(ny);
        vertices.push_back(nz);
    }
    
    // Base triangle indices
    for (int i = 0; i < slices; ++i) {
        indices.push_back(1);  // Base center
        indices.push_back(2 + i * 2);
        indices.push_back(2 + ((i + 1) % slices) * 2);
    }
    
    // Side triangle indices
    for (int i = 0; i < slices; ++i) {
        indices.push_back(0);  // Apex
        indices.push_back(3 + i * 2);
        indices.push_back(3 + ((i + 1) % slices) * 2);
    }
    
    // Bind VAO and load data
    QOpenGLContext::currentContext()->extraFunctions()->glBindVertexArray(m_vao);
    
    this->glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    this->glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);
    
    this->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    this->glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);
    
    // Position attribute
    this->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    this->glEnableVertexAttribArray(0);
    
    // Normal attribute
    this->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    this->glEnableVertexAttribArray(1);
    
    // Draw the cone
    this->glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    
    // Unbind
    QOpenGLContext::currentContext()->extraFunctions()->glBindVertexArray(0);
}
