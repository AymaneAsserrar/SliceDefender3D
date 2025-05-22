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
      m_vao(0),
      m_vbo(0),
      m_ebo(0),
      m_initialized(false),
      m_isFragment(false),
     m_texture(nullptr),
      m_hasTexture(false)
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
    QOpenGLContext::currentContext()->extraFunctions()->glGenVertexArrays(1, &m_vao);
    this->glGenBuffers(1, &m_vbo);
    this->glGenBuffers(1, &m_ebo);








    // --- Texture ---
    if (m_type == Type::APPLE) {   // Charge la texture uniquement pour la pomme (ou en fonction du type)
        if (m_texture) delete m_texture; // sécurité

        m_texture = new QOpenGLTexture(QImage(":/new/prefix2/resources/images/apple_texture.jpg").mirrored());
        m_texture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
        m_texture->setMagnificationFilter(QOpenGLTexture::Linear);
        m_texture->setWrapMode(QOpenGLTexture::Repeat);
        m_hasTexture = true;
    }

    else if (m_type == Type::BANANA) {
        if (m_texture) delete m_texture;
        m_texture = new QOpenGLTexture(QImage(":/new/prefix2/resources/images/banana4_texture.jpg").mirrored());
        m_texture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
        m_texture->setMagnificationFilter(QOpenGLTexture::Linear);
        m_texture->setWrapMode(QOpenGLTexture::Repeat);
        m_hasTexture= true;
    }

    else if (m_type == Type::FRAISE) {
        if (m_texture) delete m_texture;
        m_texture = new QOpenGLTexture(QImage(":/new/prefix2/resources/images/Fraise_texture.jpg").mirrored());
        m_texture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
        m_texture->setMagnificationFilter(QOpenGLTexture::Linear);
        m_texture->setWrapMode(QOpenGLTexture::Repeat);
        m_hasTexture = true;
    }



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

void Projectile::render(QOpenGLShaderProgram* shaderProgram,
                        const QMatrix4x4& projection,
                        const QMatrix4x4& view,
                        const QVector3D& cameraPosition) {
    if (!m_active || !m_initialized) return;

    // Calcul de la matrice modèle
    QMatrix4x4 model;
    model.translate(m_position);
    model.rotate(m_rotationAngle, m_rotationAxis);
    model.scale(m_scale);

    // Passage des matrices au shader
    shaderProgram->setUniformValue("modelMatrix", model);
    shaderProgram->setUniformValue("viewMatrix", view);
    shaderProgram->setUniformValue("projectionMatrix", projection);

    // Matrice normale pour éclairage correct des normales
    shaderProgram->setUniformValue("normalMatrix", model.normalMatrix());

    // Position et couleur de la lumière (lampadaire en haut de la scène)
    QVector3D lightPos(0.0f, 5.0f, 0.0f);
    QVector3D lightColor(1.0f, 1.0f, 1.0f);

    // Position de la caméra (vue) pour calcul de l’éclairage spéculaire
    shaderProgram->setUniformValue("lightPos", lightPos);
    shaderProgram->setUniformValue("viewPos", cameraPosition);
    shaderProgram->setUniformValue("lightColor", lightColor);

    // Couleur selon le type et si c’est un fragment
    QVector4D color;
    switch (m_type) {
    case Type::BANANA:
        color = m_isFragment ? QVector4D(1.0f, 0.98f, 0.8f, 1.0f) : QVector4D(1.0f, 0.9f, 0.0f, 1.0f);
        break;
    case Type::APPLE:
        color = m_isFragment ? QVector4D(0.98f, 0.98f, 0.95f, 1.0f) : QVector4D(0.4f, 0.8f, 0.2f, 1.0f);
        break;
    case Type::ANANAS:
        color = m_isFragment ? QVector4D(0.98f, 0.93f, 0.7f, 1.0f) : QVector4D(0.9f, 0.7f, 0.1f, 1.0f);
        break;
    case Type::FRAISE:
        // Fraise a texture spécifique, donc rendu spécial
        shaderProgram->setUniformValue("useTexture", true);
        renderFraise(shaderProgram, model, view, projection, lightPos, cameraPosition, lightColor);
        shaderProgram->setUniformValue("useTexture", false);
        glBindTexture(GL_TEXTURE_2D, 0);
        return; // On sort ici car rendu complet dans renderFraise()
    }

    shaderProgram->setUniformValue("color", color);

    // Informations pour le rendu des fragments (coupures)
    if (m_isFragment) {
        shaderProgram->setUniformValue("isFragment", true);
        shaderProgram->setUniformValue("sliceNormal", m_sliceNormal);
        shaderProgram->setUniformValue("fragmentSide", m_fragmentSide);
    } else {
        shaderProgram->setUniformValue("isFragment", false);
    }

    // Rendu selon le type
    switch (m_type) {
    case Type::BANANA:
        shaderProgram->setUniformValue("useTexture", true);
        renderBanana(shaderProgram, model, view, projection, lightPos, cameraPosition, lightColor);
        shaderProgram->setUniformValue("useTexture", false);
        glBindTexture(GL_TEXTURE_2D, 0);
        break;
    case Type::APPLE:
        shaderProgram->setUniformValue("useTexture", true);
        renderApple(shaderProgram, model, view, projection, lightPos, cameraPosition, lightColor);
        shaderProgram->setUniformValue("useTexture", false);
        glBindTexture(GL_TEXTURE_2D, 0);
        break;
    case Type::ANANAS:
        shaderProgram->setUniformValue("useTexture", false);
        renderAnanas(shaderProgram, model, view, projection, lightPos, cameraPosition, lightColor);
        break;
    case Type::FRAISE:
        // Cas déjà traité plus haut (early return)
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
    
    // Create exactly 2 halves (instead of random fragments)
    
    // Calculate the slice plane normal (perpendicular to knife movement)
    // Use rotation axis as an approximation for slice direction
    QVector3D sliceNormal = m_rotationAxis.normalized();
    
    // Create two halves of the fruit
    for (int i = 0; i < 2; ++i) {
        // Position offset for each half (opposite directions along slice normal)
        float direction = (i == 0) ? 1.0f : -1.0f;
        QVector3D halfOffset = sliceNormal * direction * 0.1f; // Slight offset for separation
        QVector3D fragmentPos = m_position + halfOffset;
        
        // Velocity for each half - base on original velocity plus some separation
        QVector3D fragmentVel = m_velocity;
        
        // Add lateral force in opposite directions
        fragmentVel += halfOffset * 2.0f; // Stronger separation for halves
        
        // Add slight upward and random rotation force
        fragmentVel.setY(fragmentVel.y() + QRandomGenerator::global()->bounded(50, 150) / 100.0f);
        
        // Create half with same type but mark as fragment
        Projectile fragment(m_type, fragmentPos, fragmentVel);
        fragment.m_scale = m_scale * 0.9f; // Slightly smaller than original
        fragment.m_isFragment = true;
        
        // Store which half this is (for rendering)
        fragment.m_fragmentSide = (i == 0) ? 1 : -1;
        
        // Also store slice normal for rendering the cut face
        fragment.m_sliceNormal = sliceNormal;
        
        // Initialize OpenGL resources
        fragment.initializeGL();

        fragments.push_back(fragment);
    }
    
    return fragments;
}

// Updated helper function to show visible cut surfaces
void Projectile::applyFragmentCutPlane(std::vector<GLfloat>& vertices, std::vector<GLuint>& indices) {
    if (!m_isFragment) return; // Only apply to fragments
    
    // We need to modify vertices to represent only half of the fruit
    // and generate a visible cut surface
    
    std::vector<GLfloat> newVertices;
    std::vector<GLuint> newIndices;
    
    // Extract cut plane from fragment info
    QVector3D planeNormal = m_sliceNormal;
    float side = static_cast<float>(m_fragmentSide);
    
    // Process each vertex and keep only those on the correct side of the cut plane
    int vertCount = vertices.size() / 6; // 6 floats per vertex (pos + normal)
    std::vector<bool> vertexVisible(vertCount, false);
    
    // First pass: determine which vertices are visible (on our side of the cut)
    int visibleCount = 0;
    for (int i = 0; i < vertCount; i++) {
        // Extract position
        float x = vertices[i * 6];
        float y = vertices[i * 6 + 1];
        float z = vertices[i * 6 + 2];
        
        // Check which side of the cut plane this vertex is on
        float dot = x * planeNormal.x() + y * planeNormal.y() + z * planeNormal.z();
        
        // Keep vertex if it's on the correct side of the plane
        if ((dot * side) >= -0.001f) { // Add small epsilon to avoid floating point issues
            vertexVisible[i] = true;
            visibleCount++;
        }
    }
    
    // Create a mapping from old vertex indices to new ones
    std::vector<int> indexMap(vertCount, -1);
    int newIndex = 0;
    
    // Second pass: copy visible vertices to new array
    for (int i = 0; i < vertCount; i++) {
        if (vertexVisible[i]) {
            // Copy vertex to new array
            for (int j = 0; j < 6; j++) {
                newVertices.push_back(vertices[i * 6 + j]);
            }
            indexMap[i] = newIndex++;
        }
    }
    
    // Collect edge vertices that need a cut face
    std::vector<int> cutEdgeVertices;
    std::vector<std::pair<int, int>> cutEdges;
    
    // Third pass: Process triangles and rebuild indices
    for (size_t i = 0; i < indices.size(); i += 3) {
        // Get vertices of this triangle
        int idx1 = indices[i];
        int idx2 = indices[i + 1];
        int idx3 = indices[i + 2];
        
        // Skip if indices are out of range
        if (idx1 >= vertCount || idx2 >= vertCount || idx3 >= vertCount) {
            continue;
        }
        
        // Check if all vertices are on our side
        if (vertexVisible[idx1] && vertexVisible[idx2] && vertexVisible[idx3]) {
            // Triangle is fully visible - add with remapped indices
            newIndices.push_back(indexMap[idx1]);
            newIndices.push_back(indexMap[idx2]);
            newIndices.push_back(indexMap[idx3]);
        }
        // Check if triangle crosses the cut plane (some vertices visible, some not)
        else if (vertexVisible[idx1] || vertexVisible[idx2] || vertexVisible[idx3]) {
            // Triangle intersects cut plane - collect edges for cut face generation
            if (vertexVisible[idx1] && vertexVisible[idx2]) {
                cutEdges.push_back(std::make_pair(idx1, idx2));
            }
            if (vertexVisible[idx2] && vertexVisible[idx3]) {
                cutEdges.push_back(std::make_pair(idx2, idx3));
            }
            if (vertexVisible[idx3] && vertexVisible[idx1]) {
                cutEdges.push_back(std::make_pair(idx3, idx1));
            }
            
            // Collect vertices for cut surface
            if (vertexVisible[idx1]) cutEdgeVertices.push_back(idx1);
            if (vertexVisible[idx2]) cutEdgeVertices.push_back(idx2);
            if (vertexVisible[idx3]) cutEdgeVertices.push_back(idx3);
        }
    }
    
    // Create cut face - find center point of all edge vertices
    if (!cutEdgeVertices.empty()) {
        float cutCenterX = 0.0f, cutCenterY = 0.0f, cutCenterZ = 0.0f;
        float cutVertCount = static_cast<float>(cutEdgeVertices.size());
        
        // Calculate average position of cut edges
        for (int idx : cutEdgeVertices) {
            cutCenterX += vertices[idx * 6];
            cutCenterY += vertices[idx * 6 + 1];
            cutCenterZ += vertices[idx * 6 + 2];
        }
        
        cutCenterX /= cutVertCount;
        cutCenterY /= cutVertCount;
        cutCenterZ /= cutVertCount;
        
        // Add center vertex
        int centerIdx = newIndex;
        newVertices.push_back(cutCenterX);
        newVertices.push_back(cutCenterY);
        newVertices.push_back(cutCenterZ);
        // Normal points along slice normal
        newVertices.push_back(-planeNormal.x() * side);
        newVertices.push_back(-planeNormal.y() * side);
        newVertices.push_back(-planeNormal.z() * side);
        newIndex++;
        
        // Create a list of unique boundary vertices (no duplicates)
        std::set<int> uniqueBoundary;
        for (auto& edge : cutEdges) {
            if (vertexVisible[edge.first]) uniqueBoundary.insert(edge.first);
            if (vertexVisible[edge.second]) uniqueBoundary.insert(edge.second);
        }
        
        // Sort boundary vertices to form a closed loop
        std::vector<int> boundary(uniqueBoundary.begin(), uniqueBoundary.end());
        
        // Create triangles for cut face - fan from center
        for (size_t i = 0; i < boundary.size(); i++) {
            int idx1 = boundary[i];
            int idx2 = boundary[(i + 1) % boundary.size()];
            
            // Skip if same vertex or mapping not found
            if (idx1 == idx2 || indexMap[idx1] == -1 || indexMap[idx2] == -1) continue;
            
            // Add triangle (center, idx1, idx2)
            newIndices.push_back(centerIdx);
            newIndices.push_back(indexMap[idx1]);
            newIndices.push_back(indexMap[idx2]);
        }
    }
    
    // Update output
    vertices = newVertices;
    indices = newIndices;
}

// Update rendering methods to handle fragments
void Projectile::renderBanana(QOpenGLShaderProgram* shaderProgram,
                              const QMatrix4x4& model,
                              const QMatrix4x4& view,
                              const QMatrix4x4& projection,
                              const QVector3D& lightPos,
                              const QVector3D& viewPos,
                              const QVector3D& lightColor)
{
    const int segments = 12;
    const float baseRadius = 0.08f;
    const float length = 0.7f;

    std::vector<GLfloat> vertices;
    std::vector<GLuint> indices;

    for (int i = 0; i <= segments; ++i) {
        float t = float(i) / float(segments);  // De 0 à 1
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

            // Normales
            float nx = std::cos(circleAngle);
            float nz = std::sin(circleAngle);

            float normalAngle = angle + M_PI_2;
            float normalFactorX = std::cos(normalAngle);
            float normalFactorY = std::sin(normalAngle);

            float adjustedNx = nx * normalFactorX - normalFactorY;
            float adjustedNy = nx * normalFactorY + normalFactorX;

            float len = std::sqrt(adjustedNx * adjustedNx + adjustedNy * adjustedNy + nz * nz);

            // Coordonnées de texture
            float u = float(j) / 16.0f;
            float v = t;

            // Ajout des données : x y z nx ny nz u v
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

    // Indices
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

    // Attributs
    this->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);                // Position
    this->glEnableVertexAttribArray(0);

    this->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float))); // Normale
    this->glEnableVertexAttribArray(1);

    this->glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float))); // Texture
    this->glEnableVertexAttribArray(2);

    // Passer les matrices et la lumière au shader
    shaderProgram->setUniformValue("modelMatrix", model);
    shaderProgram->setUniformValue("viewMatrix", view);
    shaderProgram->setUniformValue("projectionMatrix", projection);
    shaderProgram->setUniformValue("lightPos", lightPos);
    shaderProgram->setUniformValue("viewPos", viewPos);
    shaderProgram->setUniformValue("lightColor", lightColor);

    // Texture
    if (m_hasTexture && m_texture) {
        shaderProgram->setUniformValue("useTexture", true);
        m_texture->bind(0);
        shaderProgram->setUniformValue("appleTexture", 0); // Même uniform que pomme
    } else {
        shaderProgram->setUniformValue("useTexture", false);
    }

    this->glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

    if (m_hasTexture && m_texture) {
        m_texture->release();
    }

    QOpenGLContext::currentContext()->extraFunctions()->glBindVertexArray(0);
}



void Projectile::renderApple(QOpenGLShaderProgram* shaderProgram,
                             const QMatrix4x4& model,
                             const QMatrix4x4& view,
                             const QMatrix4x4& projection,
                             const QVector3D& lightPos,
                             const QVector3D& viewPos,
                             const QVector3D& lightColor)
{
    const int stacks = 24;
    const int slices = 36;
    const float radius = 0.45f;
    const float heightFactor = 1.1f;

    std::vector<GLfloat> verticesBody;
    std::vector<GLuint> indicesBody;
    std::vector<GLfloat> verticesLeaves;
    std::vector<GLuint> indicesLeaves;

    // --- Génération corps pomme (sphère modifiée) ---
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

    // Indices corps pomme
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

    // --- Génération des 2 feuilles planes ---
    const float crownY = radius * heightFactor + 0.02f;
    const float leafSize = 0.2f;

    verticesLeaves.insert(verticesLeaves.end(), {
                                                    -leafSize, crownY, 0.0f,    0, 1, 0,   0.0f, 0.0f,  // bas-gauche
                                                    leafSize, crownY, 0.0f,     0, 1, 0,   1.0f, 0.0f,  // bas-droite
                                                    -leafSize, crownY + leafSize * 0.6f, 0.0f, 0, 1, 0,  0.0f, 1.0f,  // haut-gauche
                                                    leafSize, crownY + leafSize * 0.6f, 0.0f,  0, 1, 0,  1.0f, 1.0f   // haut-droite
                                                });
    indicesLeaves.insert(indicesLeaves.end(), {
                                                  0, 1, 2,
                                                  1, 3, 2
                                              });

    auto rotateY45 = [](float x, float z) -> std::pair<float, float> {
        float angle = M_PI / 4.0f;
        float cosA = std::cos(angle);
        float sinA = std::sin(angle);
        return { x * cosA - z * sinA, x * sinA + z * cosA };
    };

    size_t baseIndex = verticesLeaves.size() / 8;

    float xBL = -leafSize, zBL = 0.0f;
    float xBR = leafSize, zBR = 0.0f;
    float xTL = -leafSize, zTL = 0.0f;
    float xTR = leafSize, zTR = 0.0f;
    float yBase = crownY;
    float yTop = crownY + leafSize * 0.6f;

    auto [xBLr, zBLr] = rotateY45(xBL, zBL);
    auto [xBRr, zBRr] = rotateY45(xBR, zBR);
    auto [xTLr, zTLr] = rotateY45(xTL, zTL);
    auto [xTRr, zTRr] = rotateY45(xTR, zTR);

    verticesLeaves.insert(verticesLeaves.end(), {
                                                    xBLr, yBase, zBLr,    0, 1, 0,   0.0f, 0.0f,
                                                    xBRr, yBase, zBRr,    0, 1, 0,   1.0f, 0.0f,
                                                    xTLr, yTop, zTLr,     0, 1, 0,   0.0f, 1.0f,
                                                    xTRr, yTop, zTRr,     0, 1, 0,   1.0f, 1.0f,
                                                });

    indicesLeaves.insert(indicesLeaves.end(), {
                                                  (GLuint)(baseIndex + 0), (GLuint)(baseIndex + 1), (GLuint)(baseIndex + 2),
                                                  (GLuint)(baseIndex + 1), (GLuint)(baseIndex + 3), (GLuint)(baseIndex + 2)
                                              });

    if (m_isFragment)
        applyFragmentCutPlane(verticesBody, indicesBody);

    QOpenGLContext::currentContext()->extraFunctions()->glBindVertexArray(m_vao);

    // Upload données corps pomme
    this->glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    this->glBufferData(GL_ARRAY_BUFFER, verticesBody.size() * sizeof(GLfloat), verticesBody.data(), GL_STATIC_DRAW);

    this->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    this->glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesBody.size() * sizeof(GLuint), indicesBody.data(), GL_STATIC_DRAW);

    // Attributs
    this->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);  // position
    this->glEnableVertexAttribArray(0);

    this->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));  // normale
    this->glEnableVertexAttribArray(1);

    this->glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));  // texture
    this->glEnableVertexAttribArray(2);

    // Uniformes d’éclairage et position caméra
    shaderProgram->setUniformValue("modelMatrix", model);
    shaderProgram->setUniformValue("viewMatrix", view);
    shaderProgram->setUniformValue("projectionMatrix", projection);
    shaderProgram->setUniformValue("lightPos", lightPos);
    shaderProgram->setUniformValue("viewPos", viewPos);
    shaderProgram->setUniformValue("lightColor", lightColor);

    if (m_hasTexture && m_texture) {
        shaderProgram->setUniformValue("useTexture", true);
        m_texture->bind(0);
        shaderProgram->setUniformValue("appleTexture", 0);
    } else {
        shaderProgram->setUniformValue("useTexture", false);
    }

    // Dessiner corps pomme
    this->glDrawElements(GL_TRIANGLES, indicesBody.size(), GL_UNSIGNED_INT, 0);

    if (m_hasTexture && m_texture) {
        m_texture->release();
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    // Upload données feuilles
    this->glBufferData(GL_ARRAY_BUFFER, verticesLeaves.size() * sizeof(GLfloat), verticesLeaves.data(), GL_STATIC_DRAW);
    this->glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesLeaves.size() * sizeof(GLuint), indicesLeaves.data(), GL_STATIC_DRAW);

    shaderProgram->setUniformValue("useTexture", false);
    shaderProgram->setUniformValue("color", QVector4D(0.0f, 0.4f, 0.0f, 1.0f)); // vert foncé

    // Dessiner feuilles
    this->glDrawElements(GL_TRIANGLES, indicesLeaves.size(), GL_UNSIGNED_INT, 0);

    QOpenGLContext::currentContext()->extraFunctions()->glBindVertexArray(0);
}


void Projectile::renderAnanas(QOpenGLShaderProgram* shaderProgram,
                              const QMatrix4x4& model,
                              const QMatrix4x4& view,
                              const QMatrix4x4& projection,
                              const QVector3D& lightPos,
                              const QVector3D& viewPos,
                              const QVector3D& lightColor)
{
    const int slices = 32;
    const int stacks = 16;
    const float bodyHeight = 0.8f;
    const float bodyRadius = 0.3f;
    const float crownHeight = 0.4f;

    std::vector<GLfloat> vertices;
    std::vector<GLuint> indices;

    // Corps de l'ananas
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
        }
    }

    // Couronne de feuilles (top)
    const int leaves = 16;
    const int leafDetail = 4;

    // Point central de la couronne
    vertices.push_back(0.0f);
    vertices.push_back(bodyHeight / 2);
    vertices.push_back(0.0f);
    vertices.push_back(0.0f);
    vertices.push_back(1.0f);
    vertices.push_back(0.0f);

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

            // Bord gauche
            vertices.push_back(px - wx);
            vertices.push_back(py);
            vertices.push_back(pz - wz);
            vertices.push_back(wx);
            vertices.push_back(1.0f - t);
            vertices.push_back(wz);

            // Bord droit
            vertices.push_back(px + wx);
            vertices.push_back(py);
            vertices.push_back(pz + wz);
            vertices.push_back(-wx);
            vertices.push_back(1.0f - t);
            vertices.push_back(-wz);
        }
    }

    // Indices corps
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

    // Indices couronne
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

    // Upload data to GPU and set vertex attrib pointers
    QOpenGLContext::currentContext()->extraFunctions()->glBindVertexArray(m_vao);

    this->glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    this->glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);

    this->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    this->glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

    // Position (loc 0)
    this->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    this->glEnableVertexAttribArray(0);

    // Normal (loc 1)
    this->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    this->glEnableVertexAttribArray(1);

    // Ajout éclairage : matrices, lumière, caméra
    shaderProgram->setUniformValue("modelMatrix", model);
    shaderProgram->setUniformValue("viewMatrix", view);
    shaderProgram->setUniformValue("projectionMatrix", projection);
    shaderProgram->setUniformValue("lightPos", lightPos);
    shaderProgram->setUniformValue("viewPos", viewPos);
    shaderProgram->setUniformValue("lightColor", lightColor);

    // Couleur ananas (orange-jaune)
    shaderProgram->setUniformValue("useTexture", false);
    shaderProgram->setUniformValue("color", QVector4D(0.85f, 0.65f, 0.25f, 1.0f));

    int bodyQuads = stacks * slices;
    int bodyIndexCount = bodyQuads * 6;

    glDrawElements(GL_TRIANGLES, bodyIndexCount, GL_UNSIGNED_INT, 0);

    // Indices couronne
    int crownIndexCount = indices.size() - bodyIndexCount;
    const void* crownIndicesOffset = (const void*)(bodyIndexCount * sizeof(GLuint));

    // Couleur vert foncé pour la couronne
    shaderProgram->setUniformValue("color", QVector4D(0.05f, 0.3f, 0.05f, 1.0f));
    glDrawElements(GL_TRIANGLES, crownIndexCount, GL_UNSIGNED_INT, crownIndicesOffset);

    QOpenGLContext::currentContext()->extraFunctions()->glBindVertexArray(0);
}




void Projectile::renderFraise(QOpenGLShaderProgram* shaderProgram,
                              const QMatrix4x4& model,
                              const QMatrix4x4& view,
                              const QMatrix4x4& projection,
                              const QVector3D& lightPos,
                              const QVector3D& viewPos,
                              const QVector3D& lightColor)
{
    const int stacks = 24;
    const int slices = 36;
    const float radius = 0.32f;   // Corps un peu plus petit (au lieu de 0.35f)
    const float height = 0.6f;    // Hauteur légèrement réduite

    std::vector<GLfloat> verticesBody;
    std::vector<GLuint> indicesBody;
    std::vector<GLfloat> verticesLeaves;
    std::vector<GLuint> indicesLeaves;

    // --- Génération corps fraise (cône arrondi) ---
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

    // --- Génération feuilles ---
    const int leafCount = 8;
    const float leafRadius = radius * 0.2f;   // Feuilles un peu plus petites (au lieu de 0.25f)
    const float leafHeight = 0.04f;            // Feuilles un peu plus basses (au lieu de 0.05f)
    const float crownY = height;                // Directement au sommet, sans espace (au lieu de height - 0.01f)

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

    // --- Upload et dessin du corps ---
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

    // Ajout des matrices et de la lumière
    shaderProgram->setUniformValue("modelMatrix", model);
    shaderProgram->setUniformValue("viewMatrix", view);
    shaderProgram->setUniformValue("projectionMatrix", projection);
    shaderProgram->setUniformValue("lightPos", lightPos);
    shaderProgram->setUniformValue("viewPos", viewPos);
    shaderProgram->setUniformValue("lightColor", lightColor);

    if (m_hasTexture && m_texture) {
        shaderProgram->setUniformValue("useTexture", true);
        m_texture->bind(0);
        shaderProgram->setUniformValue("appleTexture", 0);
    } else {
        shaderProgram->setUniformValue("useTexture", false);
        shaderProgram->setUniformValue("color", QVector4D(1.0f, 0.1f, 0.2f, 1.0f));  // Rouge fraise
    }

    this->glDrawElements(GL_TRIANGLES, indicesBody.size(), GL_UNSIGNED_INT, 0);

    if (m_hasTexture && m_texture) {
        m_texture->release();
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    // --- Upload et dessin des feuilles ---
    this->glBufferData(GL_ARRAY_BUFFER, verticesLeaves.size() * sizeof(GLfloat), verticesLeaves.data(), GL_STATIC_DRAW);
    this->glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesLeaves.size() * sizeof(GLuint), indicesLeaves.data(), GL_STATIC_DRAW);

    shaderProgram->setUniformValue("useTexture", false);
    shaderProgram->setUniformValue("color", QVector4D(0.05f, 0.35f, 0.05f, 1.0f)); // Vert feuilles un peu plus foncé

    this->glDrawElements(GL_TRIANGLES, indicesLeaves.size(), GL_UNSIGNED_INT, 0);

    QOpenGLContext::currentContext()->extraFunctions()->glBindVertexArray(0);
}
