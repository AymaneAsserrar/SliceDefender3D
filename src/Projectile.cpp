#include "Projectile.h"
#include <QtMath>
#include <QOpenGLContext>
#include <QRandomGenerator>
#include <set>

Projectile::Projectile(Type type, const QVector3D& position, const QVector3D& velocity)
    : m_type(type), 
      m_position(position), 
      m_velocity(velocity),
      m_rotationAngle(0.0f),
      m_rotationAxis(0.0f, 1.0f, 0.0f),
      m_active(true),
      m_scale(1.0f),
      m_texture(nullptr),
      m_hasTexture(false),
      m_vao(0),
      m_vbo(0),
      m_ebo(0),
      m_initialized(false),
      m_isFragment(false),
      m_causedGameOver(false)
{

    m_rotationAngle = QRandomGenerator::global()->bounded(360);

    m_rotationAxis = QVector3D(
        QRandomGenerator::global()->bounded(-100, 100) / 100.0f,
        QRandomGenerator::global()->bounded(-100, 100) / 100.0f,
        QRandomGenerator::global()->bounded(-100, 100) / 100.0f
    ).normalized();

    limitVelocity();
}

Projectile::~Projectile() {
    if (m_initialized) {
        this->glDeleteBuffers(1, &m_vbo);
        this->glDeleteBuffers(1, &m_ebo);

        if (QOpenGLContext::currentContext()) {
            QOpenGLContext::currentContext()->extraFunctions()->glDeleteVertexArrays(1, &m_vao);
        }
    }
}

void Projectile::initializeGL() {

    if (m_initialized) return;

    initializeOpenGLFunctions();

    QOpenGLContext::currentContext()->extraFunctions()->glGenVertexArrays(1, &m_vao);
    this->glGenBuffers(1, &m_vbo);
    this->glGenBuffers(1, &m_ebo);

    if (m_type == Type::APPLE) {   
        if (m_texture) delete m_texture; 

        m_texture = new QOpenGLTexture(QImage(":/new/prefix2/resources/images/apple_texture.jpg").flipped());
        m_texture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
        m_texture->setMagnificationFilter(QOpenGLTexture::Linear);
        m_texture->setWrapMode(QOpenGLTexture::Repeat);
        m_hasTexture = true;
    }

    else if (m_type == Type::BANANA) {
        if (m_texture) delete m_texture;
        m_texture = new QOpenGLTexture(QImage(":/new/prefix2/resources/images/banana4_texture.jpg").flipped());
        m_texture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
        m_texture->setMagnificationFilter(QOpenGLTexture::Linear);
        m_texture->setWrapMode(QOpenGLTexture::Repeat);
        m_hasTexture= true;
    }

    else if (m_type == Type::FRAISE) {
        if (m_texture) delete m_texture;
        m_texture = new QOpenGLTexture(QImage(":/new/prefix2/resources/images/Fraise_texture.jpg").flipped());
        m_texture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
        m_texture->setMagnificationFilter(QOpenGLTexture::Linear);
        m_texture->setWrapMode(QOpenGLTexture::Repeat);
        m_hasTexture = true;
    }

    else if (m_type == Type::ANANAS) {
        if (m_texture) delete m_texture;
        m_texture = new QOpenGLTexture(QImage(":/new/prefix2/resources/images/ananas2_texture.jpg").flipped());
        m_texture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
        m_texture->setMagnificationFilter(QOpenGLTexture::Linear);
        m_texture->setWrapMode(QOpenGLTexture::Repeat);
        m_hasTexture = true;
    }

    else if (m_type == Type::WOOD_CUBE) {
        if (m_texture) delete m_texture;
        m_texture = new QOpenGLTexture(QImage(":/new/prefix2/resources/images/wood_texture.jpg").flipped());
        m_texture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
        m_texture->setMagnificationFilter(QOpenGLTexture::Linear);
        m_texture->setWrapMode(QOpenGLTexture::Repeat);
        m_hasTexture = true;
    }

    m_initialized = true;
}

void Projectile::update(float deltaTime) {
    if (!m_active) return;

    m_position += m_velocity * deltaTime;

    m_velocity.setY(m_velocity.y() - GRAVITY * deltaTime);

    limitVelocity();

    m_rotationAngle += 90.0f * deltaTime; 
    if (m_rotationAngle > 360.0f) {
        m_rotationAngle -= 360.0f;
    }

    if (m_position.y() < -15.0f || m_position.z() > 5.0f) {
        m_active = false;
    }
}

void Projectile::render(QOpenGLShaderProgram* shaderProgram, const QMatrix4x4& projection, const QMatrix4x4& view) {
    if (!m_active || !m_initialized) return;

    shaderProgram->setUniformValue("useTexture", false);
    glBindTexture(GL_TEXTURE_2D, 0);

    QMatrix4x4 model;
    model.translate(m_position);
    model.rotate(m_rotationAngle, m_rotationAxis);
    model.scale(m_scale);

    shaderProgram->setUniformValue("mvpMatrix", projection * view * model);
    shaderProgram->setUniformValue("modelMatrix", model);
    shaderProgram->setUniformValue("viewMatrix", view);
    shaderProgram->setUniformValue("normalMatrix", model.normalMatrix());

    shaderProgram->setUniformValue("useLighting", true);
    shaderProgram->setUniformValue("ambientStrength", 0.3f);
    shaderProgram->setUniformValue("specularStrength", 0.7f); 
    shaderProgram->setUniformValue("shininess", 64.0f);       

    QVector4D color;

    if (m_causedGameOver) {
        color = QVector4D(1.0f, 0.0f, 0.0f, 1.0f); 

        model.scale(1.2f);

        shaderProgram->setUniformValue("mvpMatrix", projection * view * model);
        shaderProgram->setUniformValue("modelMatrix", model);
        shaderProgram->setUniformValue("normalMatrix", model.normalMatrix());
    } else {

        switch (m_type) {
        case Type::BANANA:
            color = m_isFragment ? QVector4D(1.0f, 0.98f, 0.8f, 1.0f) : QVector4D(1.0f, 0.9f, 0.0f, 1.0f);
            break;
        case Type::APPLE:
            color = m_isFragment ? QVector4D(0.98f, 0.98f, 0.95f, 1.0f) : QVector4D(0.4f, 0.8f, 0.2f, 1.0f);
            break;
        case Type::ANANAS:
            if (!m_hasTexture)
                color = m_isFragment ? QVector4D(0.98f, 0.93f, 0.7f, 1.0f) : QVector4D(0.9f, 0.7f, 0.1f, 1.0f);
            break;
        case Type::WOOD_CUBE:
            color = m_isFragment ? QVector4D(0.8f, 0.6f, 0.4f, 1.0f) : QVector4D(0.8f, 0.6f, 0.4f, 1.0f);
            break;
        case Type::FRAISE:
            shaderProgram->setUniformValue("useTexture", true);
            renderFraise(shaderProgram);
            shaderProgram->setUniformValue("useTexture", false);
            glBindTexture(GL_TEXTURE_2D, 0);
            break;
        }
    }
    shaderProgram->setUniformValue("color", color);

    if (m_isFragment) {
        shaderProgram->setUniformValue("isFragment", true);
        shaderProgram->setUniformValue("sliceNormal", m_sliceNormal);
        shaderProgram->setUniformValue("fragmentSide", m_fragmentSide);

        shaderProgram->setUniformValue("cutSurfaceColor", QVector4D(m_cutSurfaceColor, 1.0f));
    } else {
        shaderProgram->setUniformValue("isFragment", false);
    }

    switch (m_type) {
    case Type::BANANA:
        shaderProgram->setUniformValue("useTexture", true);
        renderBanana(shaderProgram);
        shaderProgram->setUniformValue("useTexture", false);
        glBindTexture(GL_TEXTURE_2D, 0);
        break;
    case Type::APPLE:
        shaderProgram->setUniformValue("useTexture", true);
        renderApple(shaderProgram);
        shaderProgram->setUniformValue("useTexture", false);
        glBindTexture(GL_TEXTURE_2D, 0);
        break;
    case Type::ANANAS:
        shaderProgram->setUniformValue("useTexture", true);
        renderAnanas(shaderProgram);
        shaderProgram->setUniformValue("useTexture", false);
        glBindTexture(GL_TEXTURE_2D, 0);
        break;
    case Type::WOOD_CUBE:
        shaderProgram->setUniformValue("useTexture", true);
        renderWoodCube(shaderProgram);
        shaderProgram->setUniformValue("useTexture", false);
        glBindTexture(GL_TEXTURE_2D, 0);
        break;
    case Type::FRAISE:
        shaderProgram->setUniformValue("useTexture", true);
        renderFraise(shaderProgram);
        shaderProgram->setUniformValue("useTexture", false);
        glBindTexture(GL_TEXTURE_2D, 0);
        break;
    }
}

bool Projectile::checkCollisionWithCylinder(float radius, float height, const QVector3D& cylinderPosition) {
    if (!m_active) return false;

    float projectileRadius = 0.5f * m_scale;
    QVector3D cylinderCenter = cylinderPosition + QVector3D(0, 0, 0);

    float distanceXZ = qSqrt(qPow(m_position.x() - cylinderCenter.x(), 2) + 
                            qPow(m_position.z() - cylinderCenter.z(), 2));

    if (distanceXZ > (radius + projectileRadius)) {
        return false;
    }

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

    m_active = false; 

    std::vector<Projectile> fragments;

    QVector3D velocity = m_velocity.normalized();
    QVector3D randomVec = QVector3D(
        QRandomGenerator::global()->bounded(-100, 100) / 100.0f,
        QRandomGenerator::global()->bounded(-100, 100) / 100.0f,
        QRandomGenerator::global()->bounded(-100, 100) / 100.0f
    );

    QVector3D sliceNormal = QVector3D::crossProduct(velocity, randomVec).normalized();
    if (sliceNormal.length() < 0.1f) {
        sliceNormal = QVector3D(1.0f, 0.0f, 0.0f); 
    }

    for (int i = 0; i < 2; ++i) {
        float direction = (i == 0) ? 1.0f : -1.0f;
        QVector3D halfOffset = sliceNormal * direction * 0.1f;
        QVector3D fragmentPos = m_position + halfOffset;

        QVector3D fragmentVel = m_velocity;

        fragmentVel += QVector3D(
            QRandomGenerator::global()->bounded(-50, 50) / 100.0f,
            QRandomGenerator::global()->bounded(0, 50) / 100.0f,
            QRandomGenerator::global()->bounded(-50, 50) / 100.0f
        );

        Projectile fragment(m_type, fragmentPos, fragmentVel);

        fragment.limitVelocity(FRAGMENT_MAX_HORIZONTAL_VELOCITY, FRAGMENT_MAX_VERTICAL_VELOCITY);
        fragment.m_scale = m_scale * 0.9f; 
        fragment.m_isFragment = true;
        fragment.m_sliceNormal = sliceNormal;
        fragment.m_fragmentSide = direction > 0 ? 1 : -1;

        fragment.initializeGL();

        fragment.generateCutSurface(sliceNormal, direction);

        fragments.push_back(fragment);
    }

    return fragments;
}

void Projectile::generateCutSurface(const QVector3D& sliceNormal, float ) {

    m_cutVertices.clear();

    QVector3D cutCenter = m_position; 
    float radius = m_scale * 0.5f;    

    QVector3D normal = sliceNormal.normalized();

    QVector3D tangent1;
    if (std::abs(normal.y()) < 0.9f) {
        tangent1 = QVector3D::crossProduct(normal, QVector3D(0, 1, 0)).normalized();
    } else {
        tangent1 = QVector3D::crossProduct(normal, QVector3D(1, 0, 0)).normalized();
    }
    QVector3D tangent2 = QVector3D::crossProduct(normal, tangent1).normalized();

    const int segments = 36;
    for (int i = 0; i < segments; ++i) {
        float angle = (2.0f * M_PI * i) / segments;
        float cosA = std::cos(angle);
        float sinA = std::sin(angle);

        QVector3D offset = (tangent1 * cosA + tangent2 * sinA) * radius;
        QVector3D vertex = cutCenter + offset;

        m_cutVertices.push_back(vertex);
    }

    m_cutVertices.insert(m_cutVertices.begin(), cutCenter);

    switch (m_type) {
        case Type::APPLE:
            m_cutSurfaceColor = QVector3D(1.0f, 0.95f, 0.85f); 
            break;
        case Type::BANANA:
            m_cutSurfaceColor = QVector3D(1.0f, 0.98f, 0.8f);  
            break;
        case Type::ANANAS:
            m_cutSurfaceColor = QVector3D(1.0f, 0.95f, 0.7f);  
            break;
        case Type::WOOD_CUBE:
            m_cutSurfaceColor = QVector3D(0.85f, 0.65f, 0.45f);  
            break;
        case Type::FRAISE:
            m_cutSurfaceColor = QVector3D(1.0f, 0.8f, 0.85f);  
            break;
    }
}

void Projectile::renderBanana(QOpenGLShaderProgram* shaderProgram) {
    const int segments = 12;
    const float baseRadius = 0.08f;
    const float length = 0.7f;

    std::vector<GLfloat> vertices;
    std::vector<GLuint> indices;

    for (int i = 0; i <= segments; ++i) {
        float t = float(i) / float(segments);  
        float angle = t * M_PI * 0.75f;

        float centerX = length * 0.5f * std::sin(angle);
        float centerY = length * 0.5f * (1.0f - std::cos(angle));
        float centerZ = 0.0f;

        float radiusFactor = std::sin(t * M_PI);
        float currentRadius = baseRadius * radiusFactor;

        for (int j = 0; j <= 16; ++j) {
            float circleAngle = 2.0f * M_PI * float(j) / 16.0f;
            float ovalFactor = 0.8f + 0.2f * std::cos(circleAngle);

            float x = centerX + currentRadius * ovalFactor * std::cos(circleAngle);
            float y = centerY;
            float z = centerZ + currentRadius * std::sin(circleAngle);

            float nx = std::cos(circleAngle);
            float nz = std::sin(circleAngle);

            float normalAngle = angle + M_PI_2;
            float normalFactorX = std::cos(normalAngle);
            float normalFactorY = std::sin(normalAngle);

            float adjustedNx = nx * normalFactorX - normalFactorY;
            float adjustedNy = nx * normalFactorY + normalFactorX;

            float len = std::sqrt(adjustedNx * adjustedNx + adjustedNy * adjustedNy + nz * nz);

            float u = float(j) / 16.0f;
            float v = t;

            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
            vertices.push_back(adjustedNx / len);
            vertices.push_back(adjustedNy / len);
            vertices.push_back(nz / len);
            vertices.push_back(u);
            vertices.push_back(v);
        }
    }

    const int verticesPerRing = 17;
    for (int i = 0; i < segments; ++i) {
        for (int j = 0; j < 16; ++j) {
            int current = i * verticesPerRing + j;
            int next = current + verticesPerRing;

            indices.push_back(current);
            indices.push_back(next);
            indices.push_back(current + 1);

            indices.push_back(current + 1);
            indices.push_back(next);
            indices.push_back(next + 1);
        }
    }

    if (m_isFragment) {
        applyFragmentCutPlane(vertices, indices);
    }

    QOpenGLContext::currentContext()->extraFunctions()->glBindVertexArray(m_vao);

    this->glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    this->glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);

    this->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    this->glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

    this->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);                
    this->glEnableVertexAttribArray(0);

    this->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float))); 
    this->glEnableVertexAttribArray(1);

    this->glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float))); 
    this->glEnableVertexAttribArray(2);

    if (m_hasTexture && m_texture) {
        shaderProgram->setUniformValue("useTexture", true);
        m_texture->bind(0);
        shaderProgram->setUniformValue("appleTexture", 0); 
    } else {
        shaderProgram->setUniformValue("useTexture", false);
    }

    this->glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

    if (m_hasTexture && m_texture) {
        m_texture->release();
    }

    QOpenGLContext::currentContext()->extraFunctions()->glBindVertexArray(0);
}

void Projectile::renderApple(QOpenGLShaderProgram* shaderProgram) {
    const int stacks = 24;
    const int slices = 36;
    const float radius = 0.45f;
    const float heightFactor = 1.1f;

    std::vector<GLfloat> verticesBody;
    std::vector<GLuint> indicesBody;
    std::vector<GLfloat> verticesLeaves;
    std::vector<GLuint> indicesLeaves;

    for (int i = 0; i <= stacks; ++i) {
        float v = float(i) / float(stacks);
        float phi = M_PI * v;
        float r = radius;

        if (v < 0.2f)
            r = radius * (0.9f + 0.1f * (v / 0.2f));
        else if (v > 0.8f)
            r = radius * (0.98f - 0.08f * (v - 0.8f) / 0.2f);

        if (v >= 0.3f && v <= 0.7f)
            r *= (1.0f + 0.08f * std::sin((v - 0.3f) / 0.4f * M_PI));

        float sinPhi = std::sin(phi);
        float cosPhi = std::cos(phi);

        for (int j = 0; j <= slices; ++j) {
            float u = float(j) / float(slices);
            float theta = 2.0f * M_PI * u;
            float sinTheta = std::sin(theta);
            float cosTheta = std::cos(theta);

            float x = r * sinPhi * cosTheta;
            float y = r * cosPhi * heightFactor;
            float z = r * sinPhi * sinTheta;

            float nx = sinPhi * cosTheta;
            float ny = cosPhi;
            float nz = sinPhi * sinTheta;
            float len = std::sqrt(nx * nx + ny * ny + nz * nz);

            verticesBody.push_back(x);
            verticesBody.push_back(y);
            verticesBody.push_back(z);
            verticesBody.push_back(nx / len);
            verticesBody.push_back(ny / len);
            verticesBody.push_back(nz / len);
            verticesBody.push_back(u);
            verticesBody.push_back(v);
        }
    }

    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < slices; ++j) {
            int first = i * (slices + 1) + j;
            int second = first + slices + 1;

            indicesBody.push_back(first);
            indicesBody.push_back(second);
            indicesBody.push_back(first + 1);

            indicesBody.push_back(second);
            indicesBody.push_back(second + 1);
            indicesBody.push_back(first + 1);
        }
    }

    const float crownY = radius * heightFactor + 0.02f;
    const float leafSize = 0.2f;

    verticesLeaves.insert(verticesLeaves.end(), {
                                                    -leafSize, crownY, 0.0f,    0, 1, 0,   0.0f, 0.0f,  
                                                    leafSize, crownY, 0.0f,    0, 1, 0,   1.0f, 0.0f,  
                                                    -leafSize, crownY + leafSize * 0.6f, 0.0f, 0, 1, 0,  0.0f, 1.0f,  
                                                    leafSize, crownY + leafSize * 0.6f, 0.0f, 0, 1, 0,  1.0f, 1.0f   
                                                });
    indicesLeaves.insert(indicesLeaves.end(), {
                                                  0, 1, 2,
                                                  1, 3, 2
                                              });

    auto rotateY45 = [](float x, float z) -> std::pair<float,float> {
        float angle = M_PI / 4.0f; 
        float cosA = std::cos(angle);
        float sinA = std::sin(angle);
        return { x * cosA - z * sinA, x * sinA + z * cosA };
    };

    size_t baseIndex = verticesLeaves.size() / 8; 

    float xBL = -leafSize, zBL = 0.0f;
    float xBR = leafSize,  zBR = 0.0f;
    float xTL = -leafSize, zTL = 0.0f;
    float xTR = leafSize,  zTR = 0.0f;
    float yBase = crownY;
    float yTop = crownY + leafSize * 0.6f;

    auto [xBLr, zBLr] = rotateY45(xBL, zBL);
    auto [xBRr, zBRr] = rotateY45(xBR, zBR);
    auto [xTLr, zTLr] = rotateY45(xTL, zTL);
    auto [xTRr, zTRr] = rotateY45(xTR, zTR);

    verticesLeaves.insert(verticesLeaves.end(), {
                                                    xBLr, yBase, zBLr,    0,1,0,   0.0f, 0.0f,
                                                    xBRr, yBase, zBRr,    0,1,0,   1.0f, 0.0f,
                                                    xTLr, yTop,  zTLr,    0,1,0,   0.0f, 1.0f,
                                                    xTRr, yTop,  zTRr,    0,1,0,   1.0f, 1.0f,
                                                });

    indicesLeaves.insert(indicesLeaves.end(), {
                                                  (GLuint)(baseIndex + 0), (GLuint)(baseIndex + 1), (GLuint)(baseIndex + 2),
                                                  (GLuint)(baseIndex + 1), (GLuint)(baseIndex + 3), (GLuint)(baseIndex + 2)
                                              });

    if (m_isFragment)
        applyFragmentCutPlane(verticesBody, indicesBody);

    QOpenGLContext::currentContext()->extraFunctions()->glBindVertexArray(m_vao);

    this->glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    this->glBufferData(GL_ARRAY_BUFFER, verticesBody.size() * sizeof(GLfloat), verticesBody.data(), GL_STATIC_DRAW);

    this->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    this->glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesBody.size() * sizeof(GLuint), indicesBody.data(), GL_STATIC_DRAW);

    this->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    this->glEnableVertexAttribArray(0);

    this->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    this->glEnableVertexAttribArray(1);

    this->glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    this->glEnableVertexAttribArray(2);

    if (m_hasTexture && m_texture) {
        shaderProgram->setUniformValue("useTexture", true);
        m_texture->bind(0);
        shaderProgram->setUniformValue("appleTexture", 0);
    } else {
        shaderProgram->setUniformValue("useTexture", false);
    }

    this->glDrawElements(GL_TRIANGLES, indicesBody.size(), GL_UNSIGNED_INT, 0);

    if (m_hasTexture && m_texture) {
        m_texture->release();
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    this->glBufferData(GL_ARRAY_BUFFER, verticesLeaves.size() * sizeof(GLfloat), verticesLeaves.data(), GL_STATIC_DRAW);
    this->glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesLeaves.size() * sizeof(GLuint), indicesLeaves.data(), GL_STATIC_DRAW);

    shaderProgram->setUniformValue("useTexture", false);
    shaderProgram->setUniformValue("color", QVector4D(0.0f, 0.4f, 0.0f, 1.0f)); 

    this->glDrawElements(GL_TRIANGLES, indicesLeaves.size(), GL_UNSIGNED_INT, 0);

    QOpenGLContext::currentContext()->extraFunctions()->glBindVertexArray(0);
}

void Projectile::renderAnanas(QOpenGLShaderProgram* shaderProgram) {
    const int slices = 32;
    const int stacks = 16;
    const float bodyHeight = 0.8f;
    const float bodyRadius = 0.3f;
    const float crownHeight = 0.4f;

    std::vector<GLfloat> vertices;
    std::vector<GLuint> indices;
    std::vector<GLuint> indicesBody;

    for (int i = 0; i <= stacks; ++i) {
        float v = float(i) / float(stacks);
        float y = -bodyHeight / 2 + v * bodyHeight;

        float radiusFactor = 1.0f;
        if (v < 0.2f) {
            radiusFactor = 0.7f + 0.3f * (v / 0.2f);
        } else if (v > 0.8f) {
            radiusFactor = 0.9f - 0.2f * (v - 0.8f) / 0.2f;
        } else {
            radiusFactor = 1.0f + 0.05f * std::sin((v - 0.2f) / 0.6f * M_PI);
        }

        float currentRadius = bodyRadius * radiusFactor;

        for (int j = 0; j <= slices; ++j) {
            float u = float(j) / float(slices);
            float theta = 2.0f * M_PI * u;
            float cosTheta = std::cos(theta);
            float sinTheta = std::sin(theta);

            float bumpDepth = 0.03f * std::sin(v * 40.0f) * std::sin(u * 40.0f);
            float bumpRadius = currentRadius + bumpDepth;

            float x = bumpRadius * cosTheta;
            float z = bumpRadius * sinTheta;

            float nx = cosTheta;
            float ny = bumpDepth * 4.0f;
            float nz = sinTheta;

            float len = std::sqrt(nx * nx + ny * ny + nz * nz);

            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);

            vertices.push_back(nx / len);
            vertices.push_back(ny / len);
            vertices.push_back(nz / len);

            vertices.push_back(u);
            vertices.push_back(v);
        }
    }

    const int leaves = 16;
    const int leafDetail = 4;

    vertices.push_back(0.0f);
    vertices.push_back(bodyHeight / 2);
    vertices.push_back(0.0f);
    vertices.push_back(0.0f);
    vertices.push_back(1.0f);
    vertices.push_back(0.0f);
    vertices.push_back(0.5f); 
    vertices.push_back(0.5f); 

    for (int i = 0; i < leaves; ++i) {
        float leafAngle = 2.0f * M_PI * float(i) / float(leaves);
        float leafDirection = leafAngle + M_PI_4 * 0.5f * (float(i % 3) - 1.0f);

        float baseX = 0.15f * std::cos(leafAngle);
        float baseZ = 0.15f * std::sin(leafAngle);
        float baseY = bodyHeight / 2;

        float heightVar = 0.7f + 0.6f * float(i % 3) / 2.0f;
        float tipX = baseX * 0.5f + 0.1f * std::cos(leafDirection);
        float tipZ = baseZ * 0.5f + 0.1f * std::sin(leafDirection);
        float tipY = baseY + crownHeight * heightVar;

        for (int j = 0; j <= leafDetail; ++j) {
            float t = float(j) / float(leafDetail);

            float curveOffset = 0.1f * std::sin(t * M_PI);
            float px = baseX * (1.0f - t) + tipX * t + curveOffset * std::cos(leafDirection + M_PI_2);
            float pz = baseZ * (1.0f - t) + tipZ * t + curveOffset * std::sin(leafDirection + M_PI_2);
            float py = baseY + (tipY - baseY) * (t * t);

            float width = 0.06f * (1.0f - t * 0.8f);
            float widthAngle = leafDirection + M_PI_2;
            float wx = width * std::cos(widthAngle);
            float wz = width * std::sin(widthAngle);

            vertices.push_back(px - wx);
            vertices.push_back(py);
            vertices.push_back(pz - wz);
            vertices.push_back(wx);
            vertices.push_back(1.0f - t);
            vertices.push_back(wz);
            vertices.push_back(0.0f); 
            vertices.push_back(t);    

            vertices.push_back(px + wx);
            vertices.push_back(py);
            vertices.push_back(pz + wz);
            vertices.push_back(-wx);
            vertices.push_back(1.0f - t);
            vertices.push_back(-wz);
            vertices.push_back(1.0f); 
            vertices.push_back(t);    
        }
    }

    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < slices; ++j) {
            int first = i * (slices + 1) + j;
            int second = first + slices + 1;

            indicesBody.push_back(first);
            indicesBody.push_back(second);
            indicesBody.push_back(first + 1);

            indicesBody.push_back(second);
            indicesBody.push_back(second + 1);
            indicesBody.push_back(first + 1);
        }
    }

    int crownCenterIndex = (stacks + 1) * (slices + 1);
    int leafBaseIndex = crownCenterIndex + 1;

    for (int i = 0; i < leaves; ++i) {
        int leafOffset = i * (leafDetail + 1) * 2;

        for (int j = 0; j < leafDetail; ++j) {
            int first = leafBaseIndex + leafOffset + j * 2;
            int second = first + 2;

            indices.push_back(first);
            indices.push_back(first + 1);
            indices.push_back(second);

            indices.push_back(second);
            indices.push_back(first + 1);
            indices.push_back(second + 1);
        }

        int first = leafBaseIndex + leafOffset;
        indices.push_back(crownCenterIndex);
        indices.push_back(first);
        indices.push_back(first + 1);
    }

    if (m_isFragment) {
        applyFragmentCutPlane(vertices, indices);
    }

    QOpenGLContext::currentContext()->extraFunctions()->glBindVertexArray(m_vao);

    this->glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    this->glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);

    this->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);

    std::vector<GLuint> allIndices;
    allIndices.insert(allIndices.end(), indicesBody.begin(), indicesBody.end());
    allIndices.insert(allIndices.end(), indices.begin(), indices.end());

    this->glBufferData(GL_ELEMENT_ARRAY_BUFFER, allIndices.size() * sizeof(GLuint), allIndices.data(), GL_STATIC_DRAW);

    this->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    this->glEnableVertexAttribArray(0);

    this->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    this->glEnableVertexAttribArray(1);

    this->glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    this->glEnableVertexAttribArray(2);

    int bodyIndexCount = indicesBody.size();

    if (m_hasTexture && m_texture) {
        shaderProgram->setUniformValue("useTexture", true);
        m_texture->bind(0);
        shaderProgram->setUniformValue("appleTexture", 0);

        shaderProgram->setUniformValue("color", QVector4D(1.0f, 1.0f, 1.0f, 1.0f));
    } else {
        shaderProgram->setUniformValue("useTexture", false);
        shaderProgram->setUniformValue("color", QVector4D(0.85f, 0.65f, 0.25f, 1.0f));  
    }

    glDrawElements(GL_TRIANGLES, bodyIndexCount, GL_UNSIGNED_INT, 0);

    if (m_hasTexture && m_texture) {
        m_texture->release();
    }

    int crownIndexCount = indices.size();
    const void* crownIndicesOffset = (const void*)(bodyIndexCount * sizeof(GLuint));

    shaderProgram->setUniformValue("useTexture", false);
    shaderProgram->setUniformValue("color", QVector4D(0.05f, 0.3f, 0.05f, 1.0f));  
    glDrawElements(GL_TRIANGLES, crownIndexCount, GL_UNSIGNED_INT, crownIndicesOffset);

    if (m_hasTexture && m_texture) {
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    QOpenGLContext::currentContext()->extraFunctions()->glBindVertexArray(0);
}

void Projectile::renderFraise(QOpenGLShaderProgram* shaderProgram) {
    const int stacks = 24;
    const int slices = 36;
    const float radius = 0.32f;   
    const float height = 0.6f;   

    std::vector<GLfloat> verticesBody;
    std::vector<GLuint> indicesBody;
    std::vector<GLfloat> verticesLeaves;
    std::vector<GLuint> indicesLeaves;

    for (int i = 0; i <= stacks; ++i) {
        float v = float(i) / float(stacks);
        float theta = M_PI * v / 2.0f;
        float r = radius * (1.0f - v * 0.9f) * std::sin(theta);
        float y = height * (1.0f - v);

        for (int j = 0; j <= slices; ++j) {
            float u = float(j) / float(slices);
            float phi = 2.0f * M_PI * u;

            float x = r * std::cos(phi);
            float z = r * std::sin(phi);

            QVector3D normal = QVector3D(x, radius * 0.6f, z).normalized();

            verticesBody.push_back(x);
            verticesBody.push_back(y);
            verticesBody.push_back(z);

            verticesBody.push_back(normal.x());
            verticesBody.push_back(normal.y());
            verticesBody.push_back(normal.z());

            verticesBody.push_back(u);
            verticesBody.push_back(v);
        }
    }

    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < slices; ++j) {
            int first = i * (slices + 1) + j;
            int second = first + slices + 1;

            indicesBody.push_back(first);
            indicesBody.push_back(second);
            indicesBody.push_back(first + 1);

            indicesBody.push_back(second);
            indicesBody.push_back(second + 1);
            indicesBody.push_back(first + 1);
        }
    }

    const int leafCount = 8;
    const float leafRadius = radius * 0.2f;   
    const float leafHeight = 0.04f;            
    const float crownY = height;                

    for (int i = 0; i < leafCount; ++i) {
        float angle = 2.0f * M_PI * i / leafCount;
        float nextAngle = 2.0f * M_PI * (i + 1) / leafCount;

        float x1 = leafRadius * std::cos(angle);
        float z1 = leafRadius * std::sin(angle);

        float x2 = leafRadius * std::cos(nextAngle);
        float z2 = leafRadius * std::sin(nextAngle);

        float tipX = (leafRadius + 0.02f) * std::cos(angle + M_PI / leafCount);  
        float tipZ = (leafRadius + 0.02f) * std::sin(angle + M_PI / leafCount);
        float tipY = crownY + leafHeight;

        QVector3D normalBase1 = QVector3D(x1, 0.0f, z1).normalized();
        QVector3D normalBase2 = QVector3D(x2, 0.0f, z2).normalized();
        QVector3D normalTip = QVector3D(tipX, leafHeight, tipZ).normalized();

        verticesLeaves.push_back(x1);
        verticesLeaves.push_back(crownY);
        verticesLeaves.push_back(z1);

        verticesLeaves.push_back(normalBase1.x());
        verticesLeaves.push_back(normalBase1.y());
        verticesLeaves.push_back(normalBase1.z());

        verticesLeaves.push_back(0.0f);
        verticesLeaves.push_back(0.0f);

        verticesLeaves.push_back(x2);
        verticesLeaves.push_back(crownY);
        verticesLeaves.push_back(z2);

        verticesLeaves.push_back(normalBase2.x());
        verticesLeaves.push_back(normalBase2.y());
        verticesLeaves.push_back(normalBase2.z());

        verticesLeaves.push_back(1.0f);
        verticesLeaves.push_back(0.0f);

        verticesLeaves.push_back(tipX);
        verticesLeaves.push_back(tipY);
        verticesLeaves.push_back(tipZ);

        verticesLeaves.push_back(normalTip.x());
        verticesLeaves.push_back(normalTip.y());
        verticesLeaves.push_back(normalTip.z());

        verticesLeaves.push_back(0.5f);
        verticesLeaves.push_back(1.0f);

        GLuint idx = i * 3;
        indicesLeaves.push_back(idx);
        indicesLeaves.push_back(idx + 1);
        indicesLeaves.push_back(idx + 2);
    }

    if (m_isFragment)
        applyFragmentCutPlane(verticesBody, indicesBody);

    QOpenGLContext::currentContext()->extraFunctions()->glBindVertexArray(m_vao);

    this->glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    this->glBufferData(GL_ARRAY_BUFFER, verticesBody.size() * sizeof(GLfloat), verticesBody.data(), GL_STATIC_DRAW);

    this->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    this->glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesBody.size() * sizeof(GLuint), indicesBody.data(), GL_STATIC_DRAW);

    this->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    this->glEnableVertexAttribArray(0);

    this->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    this->glEnableVertexAttribArray(1);

    this->glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    this->glEnableVertexAttribArray(2);

    if (m_hasTexture && m_texture) {
        shaderProgram->setUniformValue("useTexture", true);
        m_texture->bind(0);
        shaderProgram->setUniformValue("appleTexture", 0);
    } else {
        shaderProgram->setUniformValue("useTexture", false);
        shaderProgram->setUniformValue("color", QVector4D(1.0f, 0.1f, 0.2f, 1.0f));  
    }

    this->glDrawElements(GL_TRIANGLES, indicesBody.size(), GL_UNSIGNED_INT, 0);

    if (m_hasTexture && m_texture) {
        m_texture->release();
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    this->glBufferData(GL_ARRAY_BUFFER, verticesLeaves.size() * sizeof(GLfloat), verticesLeaves.data(), GL_STATIC_DRAW);
    this->glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesLeaves.size() * sizeof(GLuint), indicesLeaves.data(), GL_STATIC_DRAW);

    shaderProgram->setUniformValue("useTexture", false);
    shaderProgram->setUniformValue("color", QVector4D(0.05f, 0.35f, 0.05f, 1.0f)); 

    this->glDrawElements(GL_TRIANGLES, indicesLeaves.size(), GL_UNSIGNED_INT, 0);

    QOpenGLContext::currentContext()->extraFunctions()->glBindVertexArray(0);
}

void Projectile::renderShadow(QOpenGLShaderProgram* shaderProgram, const QMatrix4x4& projection, const QMatrix4x4& view, float groundLevel) {
    if (!m_active || !m_initialized) return;

    float heightAboveGround = m_position.y() - groundLevel;
    float maxShadowHeight = 5.0f; 
    float shadowOpacity = 0.8f - (heightAboveGround / maxShadowHeight) * 0.5f;
    shadowOpacity = std::max(0.2f, shadowOpacity); 

    float shadowScale = m_scale * (0.9f - heightAboveGround * 0.05f);
    shadowScale = std::max(0.5f, shadowScale); 

    QMatrix4x4 shadowModel;
    shadowModel.setToIdentity();

    shadowModel.translate(m_position.x(), groundLevel + 0.02f, m_position.z()); 

    shadowModel.scale(1.0f, 0.01f, 1.0f); 

    shadowModel.rotate(m_rotationAngle, 0.0f, 1.0f, 0.0f);
    shadowModel.scale(shadowScale); 

    shaderProgram->setUniformValue("mvpMatrix", projection * view * shadowModel);
    shaderProgram->setUniformValue("modelMatrix", shadowModel);
    shaderProgram->setUniformValue("viewMatrix", view);
    shaderProgram->setUniformValue("normalMatrix", shadowModel.normalMatrix());

    shaderProgram->setUniformValue("useLighting", false);

    shaderProgram->setUniformValue("useTexture", false);
    shaderProgram->setUniformValue("color", QVector4D(0.0f, 0.0f, 0.0f, shadowOpacity));

    shaderProgram->setUniformValue("isFragment", false);

    switch (m_type) {
    case Type::BANANA:
        renderBananaShadow(shaderProgram);
        break;
    case Type::APPLE:
        renderAppleShadow(shaderProgram);
        break;
    case Type::ANANAS:
        renderAnanasShadow(shaderProgram);
        break;
    case Type::WOOD_CUBE:
        renderWoodCubeShadow(shaderProgram);
        break;
    case Type::FRAISE:
        renderFraiseShadow(shaderProgram);
        break;
    }
}

void Projectile::renderBananaShadow(QOpenGLShaderProgram* ) {

    const int segments = 24;
    std::vector<GLfloat> vertices;
    std::vector<GLuint> indices;

    float width = 0.3f;
    float length = 0.7f;

    vertices.push_back(0.0f);  
    vertices.push_back(0.0f);  
    vertices.push_back(0.0f);  

    for (int i = 0; i <= segments; i++) {
        float angle = 2.0f * M_PI * float(i) / float(segments);
        float x = width * std::cos(angle);
        float z = length * std::sin(angle);

        vertices.push_back(x);
        vertices.push_back(0.0f);
        vertices.push_back(z);

        if (i < segments) {
            indices.push_back(0);  
            indices.push_back(i+1);
            indices.push_back(i+2);
        }
    }

    this->glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    this->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);

    this->glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);
    this->glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

    this->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    this->glEnableVertexAttribArray(0);

    this->glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
}

void Projectile::renderAppleShadow(QOpenGLShaderProgram* ) {

    const int segments = 24;
    std::vector<GLfloat> vertices;
    std::vector<GLuint> indices;

    vertices.push_back(0.0f);  
    vertices.push_back(0.0f);  
    vertices.push_back(0.0f);  

    float radius = 0.4f;

    for (int i = 0; i <= segments; i++) {
        float angle = 2.0f * M_PI * float(i) / float(segments);
        float x = radius * std::cos(angle);
        float z = radius * std::sin(angle);

        vertices.push_back(x);
        vertices.push_back(0.0f);
        vertices.push_back(z);

        if (i < segments) {
            indices.push_back(0);  
            indices.push_back(i+1);
            indices.push_back(i+2);
        }
    }

    this->glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    this->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);

    this->glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);
    this->glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

    this->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    this->glEnableVertexAttribArray(0);

    this->glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
}

void Projectile::renderAnanasShadow(QOpenGLShaderProgram* ) {

    const int segments = 24;
    std::vector<GLfloat> vertices;
    std::vector<GLuint> indices;

    vertices.push_back(0.0f);  
    vertices.push_back(0.0f);  
    vertices.push_back(0.0f);  

    float radius = 0.35f;

    for (int i = 0; i <= segments; i++) {
        float angle = 2.0f * M_PI * float(i) / float(segments);
        float x = radius * std::cos(angle);
        float z = radius * std::sin(angle);

        vertices.push_back(x);
        vertices.push_back(0.0f);
        vertices.push_back(z);

        if (i < segments) {
            indices.push_back(0);  
            indices.push_back(i+1);
            indices.push_back(i+2);
        }
    }

    this->glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    this->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);

    this->glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);
    this->glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

    this->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    this->glEnableVertexAttribArray(0);

    this->glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
}

void Projectile::renderFraiseShadow(QOpenGLShaderProgram* ) {

    const int segments = 30;
    std::vector<GLfloat> vertices;
    std::vector<GLuint> indices;

    vertices.push_back(0.0f);  
    vertices.push_back(0.0f);  
    vertices.push_back(0.0f);  

    float radius = 0.3f;

    for (int i = 0; i <= segments; i++) {
        float angle = 2.0f * M_PI * float(i) / float(segments);
        float x = radius * std::cos(angle);
        float z = radius * std::sin(angle);

        if (z > 0) {
            z *= 1.2f;  
        }

        vertices.push_back(x);
        vertices.push_back(0.0f);
        vertices.push_back(z);

        if (i < segments) {
            indices.push_back(0);  
            indices.push_back(i+1);
            indices.push_back(i+2);
        }
    }

    this->glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    this->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);

    this->glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);
    this->glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

    this->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    this->glEnableVertexAttribArray(0);

    this->glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
}

void Projectile::applyFragmentCutPlane(std::vector<GLfloat>& vertices, std::vector<GLuint>& indices) {

    if (!m_isFragment) return;

    QVector3D planeNormal = m_sliceNormal;
    QVector3D planePoint = m_position;

    std::vector<GLfloat> newVertices;
    std::map<int, int> oldToNewIndex;
    std::vector<GLuint> newIndices;

    int vertexSize = 8;  
    for (size_t i = 0; i < vertices.size(); i += vertexSize) {
        QVector3D vertex(vertices[i], vertices[i + 1], vertices[i + 2]);
        QVector3D toVertex = vertex - planePoint;

        float side = float(m_fragmentSide);
        if (QVector3D::dotProduct(toVertex, planeNormal) * side > -0.001f) {

            oldToNewIndex[i/vertexSize] = newVertices.size()/vertexSize;

            for (int j = 0; j < vertexSize; ++j) {
                newVertices.push_back(vertices[i + j]);
            }
        }
    }

    for (size_t i = 0; i < indices.size(); i += 3) {
        GLuint idx1 = indices[i];
        GLuint idx2 = indices[i + 1];
        GLuint idx3 = indices[i + 2];

        if (oldToNewIndex.find(idx1) != oldToNewIndex.end() && 
            oldToNewIndex.find(idx2) != oldToNewIndex.end() && 
            oldToNewIndex.find(idx3) != oldToNewIndex.end()) {

            newIndices.push_back(oldToNewIndex[idx1]);
            newIndices.push_back(oldToNewIndex[idx2]);
            newIndices.push_back(oldToNewIndex[idx3]);
        }
    }

    if (!m_cutVertices.empty()) {
        int baseIdx = newVertices.size() / vertexSize;

        for (const auto& cutVertex : m_cutVertices) {

            newVertices.push_back(cutVertex.x());
            newVertices.push_back(cutVertex.y());
            newVertices.push_back(cutVertex.z());

            newVertices.push_back(planeNormal.x() * float(-m_fragmentSide));
            newVertices.push_back(planeNormal.y() * float(-m_fragmentSide));
            newVertices.push_back(planeNormal.z() * float(-m_fragmentSide));

            newVertices.push_back(0.5f);
            newVertices.push_back(0.5f);
        }

        for (size_t i = 1; i < m_cutVertices.size() - 1; ++i) {
            newIndices.push_back(baseIdx);
            newIndices.push_back(baseIdx + i);
            newIndices.push_back(baseIdx + i + 1);
        }
    }

    vertices = newVertices;
    indices = newIndices;
}

void Projectile::renderWoodCube(QOpenGLShaderProgram* ) {

    const float size = 0.4f; 

    if (m_hasTexture && m_texture) {
        m_texture->bind();
    }

    std::vector<GLfloat> vertices = {

        -size, -size, size,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,  
        size, -size, size,    0.0f, 0.0f, 1.0f,   1.0f, 0.0f,  
        size, size, size,     0.0f, 0.0f, 1.0f,   1.0f, 1.0f,  
        -size, size, size,    0.0f, 0.0f, 1.0f,   0.0f, 1.0f,  

        -size, -size, -size,  0.0f, 0.0f, -1.0f,  1.0f, 0.0f,  
        -size, size, -size,   0.0f, 0.0f, -1.0f,  1.0f, 1.0f,  
        size, size, -size,    0.0f, 0.0f, -1.0f,  0.0f, 1.0f,  
        size, -size, -size,   0.0f, 0.0f, -1.0f,  0.0f, 0.0f,  

        -size, size, -size,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f,  
        -size, size, size,    0.0f, 1.0f, 0.0f,   0.0f, 0.0f,  
        size, size, size,     0.0f, 1.0f, 0.0f,   1.0f, 0.0f,  
        size, size, -size,    0.0f, 1.0f, 0.0f,   1.0f, 1.0f,  

        -size, -size, -size,  0.0f, -1.0f, 0.0f,  1.0f, 1.0f,  
        size, -size, -size,   0.0f, -1.0f, 0.0f,  0.0f, 1.0f,  
        size, -size, size,    0.0f, -1.0f, 0.0f,  0.0f, 0.0f,  
        -size, -size, size,   0.0f, -1.0f, 0.0f,  1.0f, 0.0f,  

        size, -size, -size,   1.0f, 0.0f, 0.0f,   1.0f, 0.0f,  
        size, size, -size,    1.0f, 0.0f, 0.0f,   1.0f, 1.0f,  
        size, size, size,     1.0f, 0.0f, 0.0f,   0.0f, 1.0f,  
        size, -size, size,    1.0f, 0.0f, 0.0f,   0.0f, 0.0f,  

        -size, -size, -size,  -1.0f, 0.0f, 0.0f,  0.0f, 0.0f,  
        -size, -size, size,   -1.0f, 0.0f, 0.0f,  1.0f, 0.0f,  
        -size, size, size,    -1.0f, 0.0f, 0.0f,  1.0f, 1.0f,  
        -size, size, -size,   -1.0f, 0.0f, 0.0f,  0.0f, 1.0f   
    };

    std::vector<GLuint> indices = {
        0, 1, 2, 2, 3, 0,       
        4, 5, 6, 6, 7, 4,       
        8, 9, 10, 10, 11, 8,    
        12, 13, 14, 14, 15, 12, 
        16, 17, 18, 18, 19, 16, 
        20, 21, 22, 22, 23, 20  
    };

    this->glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    this->glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);

    this->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    this->glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

    this->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);                
    this->glEnableVertexAttribArray(0);
    this->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float))); 
    this->glEnableVertexAttribArray(1);
    this->glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float))); 
    this->glEnableVertexAttribArray(2);

    this->glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

    this->glDisableVertexAttribArray(0);
    this->glDisableVertexAttribArray(1);
    this->glDisableVertexAttribArray(2);
}

void Projectile::renderWoodCubeShadow(QOpenGLShaderProgram* shaderProgram) {

    const float size = 0.4f;

    shaderProgram->setUniformValue("color", QVector4D(0.0f, 0.0f, 0.0f, 0.5f));

    std::vector<GLfloat> vertices = {

        -size, 0.01f, -size,   0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
        size, 0.01f, -size,    0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
        size, 0.01f, size,     0.0f, 1.0f, 0.0f,   1.0f, 1.0f,
        -size, 0.01f, size,    0.0f, 1.0f, 0.0f,   0.0f, 1.0f
    };

    std::vector<GLuint> indices = {
        0, 1, 2, 2, 3, 0  
    };

    this->glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    this->glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);

    this->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    this->glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

    this->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);                
    this->glEnableVertexAttribArray(0);
    this->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float))); 
    this->glEnableVertexAttribArray(1);
    this->glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float))); 
    this->glEnableVertexAttribArray(2);

    this->glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

    this->glDisableVertexAttribArray(0);
    this->glDisableVertexAttribArray(1);
    this->glDisableVertexAttribArray(2);
}