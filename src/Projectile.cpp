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
    
    // Set color based on fruit type
    QVector4D color;
    switch (m_type) {
        case Type::BANANA:
            // For fragments, show inner banana color (slightly lighter)
            if (m_isFragment) {
                color = QVector4D(1.0f, 0.98f, 0.8f, 1.0f); // Creamy white for banana flesh
            } else {
                color = QVector4D(1.0f, 0.9f, 0.0f, 1.0f); // Yellow for skin
            }
            break;
        case Type::APPLE:
            // For fragments, show inner apple color - pure white with slight cream tint
            if (m_isFragment) {
                color = QVector4D(0.98f, 0.98f, 0.95f, 1.0f); // Almost pure white for apple flesh
            } else {
                color = QVector4D(0.4f, 0.8f, 0.2f, 1.0f); // Green apple skin
            }
            break;
        case Type::ANANAS:
            // For fragments, show inner pineapple color - more yellow-white
            if (m_isFragment) {
                color = QVector4D(0.98f, 0.93f, 0.7f, 1.0f); // Pale yellow for pineapple flesh
            } else {
                color = QVector4D(0.9f, 0.7f, 0.1f, 1.0f); // Golden yellow for outside
            }
            break;
    }
    shaderProgram->setUniformValue("color", color);
    
    // Render based on fruit type
    switch (m_type) {
        case Type::BANANA:
            renderBanana(shaderProgram);
            break;
        case Type::APPLE:
            renderApple(shaderProgram);
            break;
        case Type::ANANAS:
            renderAnanas(shaderProgram);
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
void Projectile::renderBanana(QOpenGLShaderProgram* /* shaderProgram */) {
    // More detailed banana with curved shape and realistic proportions
    const int segments = 12;  // Increase segments for smoother curve
    const float baseRadius = 0.06f;
    const float length = 0.4f;
    
    std::vector<GLfloat> vertices;
    std::vector<GLuint> indices;
    
    // Create the banana using a curved cylinder with varying radius
    // Main body of banana
    for (int i = 0; i <= segments; ++i) {
        float t = float(i) / float(segments);  // Parameter along banana length
        
        // Calculate positions along a curved path (banana curve)
        float angle = t * M_PI * 0.75f;  // 3/4 of a circle for the curve
        
        // Center position along curved path
        float centerX = length * 0.5f * std::sin(angle);
        float centerY = length * 0.5f * (1.0f - std::cos(angle));
        float centerZ = 0.0f;
        
        // Vary radius along the banana for tapered ends
        float radiusFactor = std::sin(t * M_PI);
        float currentRadius = baseRadius * radiusFactor;
        
        // Create circle at this position
        for (int j = 0; j <= 16; ++j) {
            float circleAngle = 2.0f * M_PI * float(j) / 16.0f;
            
            // Vary shape slightly to make it more oval in cross-section
            float ovalFactor = 0.8f + 0.2f * std::cos(circleAngle);
            
            // Calculate position on circle
            float x = centerX + currentRadius * ovalFactor * std::cos(circleAngle);
            float y = centerY;
            float z = centerZ + currentRadius * std::sin(circleAngle);
            
            // Calculate normal
            float nx = std::cos(circleAngle);
            // Remove unused variable ny
            float nz = std::sin(circleAngle);
            
            // Adjust normal direction based on banana curve
            float normalAngle = angle + M_PI_2;  // Perpendicular to curve
            float normalFactorX = std::cos(normalAngle);
            float normalFactorY = std::sin(normalAngle);
            
            float adjustedNx = nx * normalFactorX - normalFactorY;
            float adjustedNy = nx * normalFactorY + normalFactorX;
            
            // Normalize the normal vector
            float len = std::sqrt(adjustedNx*adjustedNx + adjustedNy*adjustedNy + nz*nz);
            
            // Add vertex data
            vertices.push_back(x);  // Position
            vertices.push_back(y);
            vertices.push_back(z);
            vertices.push_back(adjustedNx/len);  // Normal
            vertices.push_back(adjustedNy/len);
            vertices.push_back(nz/len);
        }
    }
    
    // Generate indices for banana body
    const int verticesPerRing = 17;  // 16 segments + 1 duplicate for texture wrap
    for (int i = 0; i < segments; ++i) {
        for (int j = 0; j < 16; ++j) {
            int current = i * verticesPerRing + j;
            int next = current + verticesPerRing;
            
            // Two triangles per quad
            indices.push_back(current);
            indices.push_back(next);
            indices.push_back(current + 1);
            
            indices.push_back(current + 1);
            indices.push_back(next);
            indices.push_back(next + 1);
        }
    }
    
    // Apply cut plane if this is a fragment
    if (m_isFragment) {
        applyFragmentCutPlane(vertices, indices);
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
    
    // Draw the banana
    this->glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    
    // Unbind
    QOpenGLContext::currentContext()->extraFunctions()->glBindVertexArray(0);
}

void Projectile::renderApple(QOpenGLShaderProgram* /* shaderProgram */) {
    // Green apple with slightly elongated shape
    const int stacks = 24;
    const int slices = 36;
    const float radius = 0.45f;
    const float heightFactor = 1.1f; // Make slightly taller for green apple shape
    
    std::vector<GLfloat> vertices;
    std::vector<GLuint> indices;
    
    // Generate apple body - a sphere with modifications for green apple shape
    for (int i = 0; i <= stacks; ++i) {
        float v = float(i) / float(stacks);
        float phi = M_PI * v;
        
        // Modify shape for green apple characteristics
        float r = radius;
        
        // Less pronounced indentation at top and slightly more elongated
        if (v < 0.2f) {  // Top indentation
            r = radius * (0.9f + 0.1f * (v / 0.2f));
        } else if (v > 0.8f) {  // Bottom indentation
            r = radius * (0.98f - 0.08f * (v - 0.8f) / 0.2f);
        }
        
        // Slight elongation for green apple
        if (v >= 0.3f && v <= 0.7f) {
            float bulge = std::sin((v - 0.3f) / 0.4f * M_PI);
            r *= (1.0f + 0.08f * bulge);  // 8% bulge (more than red apple)
        }
        
        float sinPhi = std::sin(phi);
        float cosPhi = std::cos(phi);
        
        for (int j = 0; j <= slices; ++j) {
            float u = float(j) / float(slices);
            float theta = 2.0f * M_PI * u;
            float sinTheta = std::sin(theta);
            float cosTheta = std::cos(theta);
            
            // Position - apply height factor for elongated shape
            float x = r * sinPhi * cosTheta;
            float y = r * cosPhi * heightFactor; // Elongate in y-direction
            float z = r * sinPhi * sinTheta;
            
            // Normal - adjusted for the modified shape
            float nx = sinPhi * cosTheta;
            float ny = cosPhi;
            float nz = sinPhi * sinTheta;
            
            // Normalize the normal vector
            float len = std::sqrt(nx*nx + ny*ny + nz*nz);
            
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
            vertices.push_back(nx/len);
            vertices.push_back(ny/len);
            vertices.push_back(nz/len);
        }
    }
    
    // Add stem at the top of the apple - make stem slightly longer and thinner
    const float stemHeight = 0.18f;
    const float stemRadius = 0.025f;
    const int stemSegments = 8;
    const float stemTilt = 0.15f;  // Less tilt for green apple
    
    for (int i = 0; i <= stemSegments; ++i) {
        float t = float(i) / float(stemSegments);
        float stemY = radius * 0.85f + t * stemHeight;  // Start from the top indentation
        
        // Make stem curve slightly
        float offsetX = stemTilt * std::sin(t * M_PI_2);
        
        for (int j = 0; j <= 8; ++j) {
            float angle = 2.0f * M_PI * float(j) / 8.0f;
            float cosAngle = std::cos(angle);
            float sinAngle = std::sin(angle);
            
            // Create stem with slight tapering
            float taperFactor = 1.0f - 0.3f * t;  // Stem gets thinner at top
            
            float stemX = offsetX + stemRadius * taperFactor * cosAngle;
            float stemZ = stemRadius * taperFactor * sinAngle;
            
            // Normal for stem - points outward from center
            float nx = cosAngle;
            float ny = 0.2f;  // Slight upward component
            float nz = sinAngle;
            float normalLen = std::sqrt(nx*nx + ny*ny + nz*nz);
            
            // Add vertex
            vertices.push_back(stemX);
            vertices.push_back(stemY);
            vertices.push_back(stemZ);
            vertices.push_back(nx/normalLen);
            vertices.push_back(ny/normalLen);
            vertices.push_back(nz/normalLen);
        }
    }
    
    // Generate indices for apple body
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
    
    // Generate indices for stem
    int stemBaseIndex = (stacks + 1) * (slices + 1);
    for (int i = 0; i < stemSegments; ++i) {
        for (int j = 0; j < 8; ++j) {
            int first = stemBaseIndex + i * 9 + j;
            int second = first + 9;
            
            indices.push_back(first);
            indices.push_back(second);
            indices.push_back(first + 1);
            
            indices.push_back(second);
            indices.push_back(second + 1);
            indices.push_back(first + 1);
        }
    }
    
    // Apply cut plane if this is a fragment
    if (m_isFragment) {
        applyFragmentCutPlane(vertices, indices);
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
    
    // Draw the apple
    this->glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    
    // Unbind
    QOpenGLContext::currentContext()->extraFunctions()->glBindVertexArray(0);
}

void Projectile::renderAnanas(QOpenGLShaderProgram* /* shaderProgram */) {
    // More detailed pineapple with texture pattern and spiky crown
    const int slices = 32;
    const int stacks = 16;
    const float bodyHeight = 0.8f;
    const float bodyRadius = 0.3f;
    const float crownHeight = 0.4f;
    
    std::vector<GLfloat> vertices;
    std::vector<GLuint> indices;
    
    // Generate pineapple body with diamond pattern texture
    for (int i = 0; i <= stacks; ++i) {
        float v = float(i) / float(stacks);
        float y = -bodyHeight/2 + v * bodyHeight;
        
        // Vary radius to create slight bulge in middle
        float radiusFactor = 1.0f;
        if (v < 0.2f) {
            radiusFactor = 0.7f + 0.3f * (v / 0.2f);  // Taper at bottom
        } else if (v > 0.8f) {
            radiusFactor = 0.9f - 0.2f * (v - 0.8f) / 0.2f;  // Taper at top
        } else {
            radiusFactor = 1.0f + 0.05f * std::sin((v - 0.2f) / 0.6f * M_PI);  // Slight bulge
        }
        
        float currentRadius = bodyRadius * radiusFactor;
        
        for (int j = 0; j <= slices; ++j) {
            float u = float(j) / float(slices);
            float theta = 2.0f * M_PI * u;
            float cosTheta = std::cos(theta);
            float sinTheta = std::sin(theta);
            
            // Create diamond pattern effect
            float bumpDepth = 0.03f * std::sin(v * 40.0f) * std::sin(u * 40.0f);
            float bumpRadius = currentRadius + bumpDepth;
            
            // Position
            float x = bumpRadius * cosTheta;
            float z = bumpRadius * sinTheta;
            
            // Normal calculation for bumpy surface
            float nx = cosTheta;
            float ny = bumpDepth * 4.0f;  // Exaggerate normal for bump effect
            float nz = sinTheta;
            
            // Normalize the normal
            float len = std::sqrt(nx*nx + ny*ny + nz*nz);
            
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
            vertices.push_back(nx/len);
            vertices.push_back(ny/len);
            vertices.push_back(nz/len);
        }
    }
    
    // Generate crown as a set of spiky leaves
    const int leaves = 16;    // Number of leaves
    const int leafDetail = 4; // Detail level of each leaf
    
    // First add center point for crown
    vertices.push_back(0.0f);                // x
    vertices.push_back(bodyHeight/2);        // y
    vertices.push_back(0.0f);                // z
    vertices.push_back(0.0f);                // nx
    vertices.push_back(1.0f);                // ny
    vertices.push_back(0.0f);                // nz
    
    // Add leaves radiating from center
    for (int i = 0; i < leaves; ++i) {
        float leafAngle = 2.0f * M_PI * float(i) / float(leaves);
        float leafDirection = leafAngle + M_PI_4 * 0.5f * (float(i % 3) - 1.0f);  // Random variation
        
        // Base of leaf
        float baseX = 0.15f * std::cos(leafAngle);
        float baseZ = 0.15f * std::sin(leafAngle);
        float baseY = bodyHeight/2;
        
        // Tip of leaf - varies in height and curvature
        float heightVar = 0.7f + 0.6f * float(i % 3) / 2.0f;  // Vary height
        float tipX = baseX * 0.5f + 0.1f * std::cos(leafDirection);
        float tipZ = baseZ * 0.5f + 0.1f * std::sin(leafDirection);
        float tipY = baseY + crownHeight * heightVar;
        
        // Create the leaf as a tapered shape
        for (int j = 0; j <= leafDetail; ++j) {
            float t = float(j) / float(leafDetail);
            
            // Interpolate between base and tip with a slight curve
            float curveOffset = 0.1f * std::sin(t * M_PI);
            float px = baseX * (1.0f - t) + tipX * t + curveOffset * std::cos(leafDirection + M_PI_2);
            float pz = baseZ * (1.0f - t) + tipZ * t + curveOffset * std::sin(leafDirection + M_PI_2);
            float py = baseY + (tipY - baseY) * (t * t);  // Quadratic curve up
            
            // Width decreases from base to tip
            float width = 0.06f * (1.0f - t * 0.8f);
            
            // Calculate two points for the width of the leaf
            float widthAngle = leafDirection + M_PI_2;
            float wx = width * std::cos(widthAngle);
            float wz = width * std::sin(widthAngle);
            
            // Left edge vertex
            vertices.push_back(px - wx);
            vertices.push_back(py);
            vertices.push_back(pz - wz);
            vertices.push_back(wx);           // Use simplified normals
            vertices.push_back(1.0f - t);     // Normal more vertical near tip
            vertices.push_back(wz);
            
            // Right edge vertex
            vertices.push_back(px + wx);
            vertices.push_back(py);
            vertices.push_back(pz + wz);
            vertices.push_back(-wx);          // Use simplified normals  
            vertices.push_back(1.0f - t);     // Normal more vertical near tip
            vertices.push_back(-wz);
        }
    }
    
    // Generate indices for body
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
    
    // Generate indices for crown leaves
    int crownCenterIndex = (stacks + 1) * (slices + 1);
    int leafBaseIndex = crownCenterIndex + 1;
    
    for (int i = 0; i < leaves; ++i) {
        int leafOffset = i * (leafDetail + 1) * 2;
        
        // Connect leaf vertices as triangle strips
        for (int j = 0; j < leafDetail; ++j) {
            int first = leafBaseIndex + leafOffset + j * 2;
            int second = first + 2;
            
            // Two triangles for each leaf segment
            indices.push_back(first);
            indices.push_back(first + 1);
            indices.push_back(second);
            
            indices.push_back(second);
            indices.push_back(first + 1);
            indices.push_back(second + 1);
        }
        
        // Connect first segment to center point
        int first = leafBaseIndex + leafOffset;
        
        indices.push_back(crownCenterIndex);
        indices.push_back(first);
        indices.push_back(first + 1);
    }
    
    // Apply cut plane if this is a fragment
    if (m_isFragment) {
        applyFragmentCutPlane(vertices, indices);
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
    
    // Draw the pineapple
    this->glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    
    // Unbind
    QOpenGLContext::currentContext()->extraFunctions()->glBindVertexArray(0);
}
