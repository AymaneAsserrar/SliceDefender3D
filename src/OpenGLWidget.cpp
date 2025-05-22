#include "OpenGLWidget.h"
#include <QtMath>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QMatrix4x4>
#include <QOpenGLFunctions>
#include <QRandomGenerator>
#include <QKeyEvent>

OpenGLWidget::OpenGLWidget(QWidget *parent) : QOpenGLWidget(parent), shaderProgram(nullptr), vbo(QOpenGLBuffer::VertexBuffer) {
    // Set focus policy to enable keyboard input
    setFocusPolicy(Qt::StrongFocus);
    setFocus();  // Request focus immediately
}


OpenGLWidget::~OpenGLWidget() {

    makeCurrent();
    vbo.destroy();
    delete shaderProgram;
    doneCurrent();
}

void OpenGLWidget::initializeGL() {
    initializeOpenGLFunctions();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    shaderProgram = new QOpenGLShaderProgram(this);

    // Vertex shader avec normales + calcul positions pour éclairage
    shaderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, R"(
        #version 330 core
        layout(location = 0) in vec3 position;
        layout(location = 1) in vec3 normal;
        layout(location = 2) in vec2 texCoord;

        uniform mat4 modelMatrix;
        uniform mat4 viewMatrix;
        uniform mat4 projectionMatrix;

        out vec3 fragPos;
        out vec3 fragNormal;
        out vec2 vTexCoord;

        void main() {
            vec4 worldPos = modelMatrix * vec4(position, 1.0);
            fragPos = worldPos.xyz;
            fragNormal = mat3(transpose(inverse(modelMatrix))) * normal;
            vTexCoord = texCoord;

            gl_Position = projectionMatrix * viewMatrix * worldPos;
        }
    )");

    // Fragment shader avec éclairage Phong simple + texture
    shaderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, R"(
        #version 330 core
        in vec3 fragPos;
        in vec3 fragNormal;
        in vec2 vTexCoord;

        out vec4 fragColor;

        uniform vec3 lightPos;
        uniform vec3 viewPos;
        uniform vec3 lightColor;

        uniform vec4 color;
        uniform sampler2D appleTexture;
        uniform bool useTexture;

        void main() {
            vec3 norm = normalize(fragNormal);
            vec3 lightDir = normalize(lightPos - fragPos);

            // Ambiant
            vec3 ambient = 0.4 * lightColor;

            // Diffuse
            float diff = max(dot(norm, lightDir), 0.0);
            vec3 diffuse = diff * lightColor;

            // Speculaire (Blinn-Phong)
            vec3 viewDir = normalize(viewPos - fragPos);
            vec3 halfwayDir = normalize(lightDir + viewDir);
            float spec = pow(max(dot(norm, halfwayDir), 0.0), 32.0);
            vec3 specular = spec * lightColor;

            vec3 lighting = ambient + diffuse + specular;

            vec4 baseColor = useTexture ? texture(appleTexture, vTexCoord) : color;

            fragColor = vec4(baseColor.rgb * lighting, baseColor.a);
        }
    )");

    shaderProgram->link();

    // Charger textures lame et manche (inchangé)
    bladeTexture = new QOpenGLTexture(QImage(":/new/prefix2/resources/images/blade2_texture.jpg").mirrored());
    bladeTexture->setMinificationFilter(QOpenGLTexture::Linear);
    bladeTexture->setMagnificationFilter(QOpenGLTexture::Linear);
    bladeTexture->setWrapMode(QOpenGLTexture::Repeat);

    handleTexture = new QOpenGLTexture(QImage(":/new/prefix2/resources/images/handle_texture.png").mirrored());
    handleTexture->setMinificationFilter(QOpenGLTexture::Linear);
    handleTexture->setMagnificationFilter(QOpenGLTexture::Linear);
    handleTexture->setWrapMode(QOpenGLTexture::Repeat);

    vbo.create();

    elapsedTimer.start();
    timerId = startTimer(16);

    setHandPosition(0.5f, 0.5f);
    resetCamera();
}


void OpenGLWidget::resetCamera() {
    // Reset camera to default position
    cameraPosition = QVector3D(0, 0, 5);
    cameraYaw = 0.0f;
    cameraPitch = 0.0f;
    cameraDistance = 5.0f;

    // Reset key states
    for (int i = 0; i < 4; i++) {
        keysPressed[i] = false;
    }

    update();
}

void OpenGLWidget::resizeGL(int /*w*/, int /*h*/) {
    // No-op, parameters unused
}

void OpenGLWidget::setHandPosition(float normX, float normY) {
    normHandX = normX;
    normHandY = normY;

    // Define movement range based on webcam aspects
    const float xRange = 0.7f;  // Limit horizontal range to 70% of full cylinder
    const float yRange = 0.8f;  // Limit vertical range to 80% of cylinder height

    // Center the range and scale to webcam motion
    float adjustedX = 0.5f + (normX - 0.5f) * xRange;
    float adjustedY = 0.5f + (normY - 0.5f) * yRange;

    // Map to cylinder coordinates - convert to radians
    float theta = adjustedX * 2.0f * M_PI;

    // Map to vertical position
    float y = (adjustedY - 0.5f) * cylinderHeight;

    // Clamp y to prevent going outside cylinder
    y = std::max(-cylinderHeight/2.0f + 0.1f, std::min(cylinderHeight/2.0f - 0.1f, y));

    // Convert to 3D position
    handX = cylinderRadius * std::cos(theta);
    handY = y;
    handZ = cylinderRadius * std::sin(theta);
    handPosition = QVector3D(handX, handY, handZ);
    handSet = true;
    update();
}

void OpenGLWidget::setHandPosition(const QVector3D& position) {
    handPosition = position;
    handX = position.x();
    handY = position.y();
    handZ = position.z();
    handSet = true;
    update();
}

void OpenGLWidget::paintGL() {

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Configurer matrices projection et vue
    projection.setToIdentity();
    projection.perspective(45.0f, float(width()) / height(), 0.1f, 100.0f);

    view.setToIdentity();
    view.lookAt(cameraPosition, QVector3D(0, 0, 0), QVector3D(0, 1, 0));

    shaderProgram->bind();

    // Position et couleur de la lumière (statique, fixe au-dessus)
    QVector3D lightPos(0.0f, 5.0f, 0.0f);
    QVector3D lightColor(1.0f, 1.0f, 1.0f);

    // Zone de spawn (fond cylindrique transparent)
    {
        QMatrix4x4 model;
        model.setToIdentity();
        model.translate(0, -0.5f, 2.5f);

        shaderProgram->setUniformValue("mvpMatrix", projection * view * model);
        shaderProgram->setUniformValue("modelMatrix", model);
        shaderProgram->setUniformValue("viewMatrix", view);
        shaderProgram->setUniformValue("projectionMatrix", projection);
        shaderProgram->setUniformValue("lightPos", lightPos);
        shaderProgram->setUniformValue("viewPos", cameraPosition);
        shaderProgram->setUniformValue("lightColor", lightColor);

        shaderProgram->setUniformValue("useTexture", false);
        shaderProgram->setUniformValue("color", QVector4D(0.2f, 0.7f, 1.0f, 0.4f));

        glDepthMask(GL_FALSE);
        drawCylinder();
        glDepthMask(GL_TRUE);
    }

    // Dessiner le sabre texturé (avec éclairage)
    if (handSet) {
        QMatrix4x4 model;
        model.setToIdentity();

        float angle = atan2(handZ, handX) * 180.0f / M_PI;

        model.translate(handX, handY - 0.5f, handZ + 2.5f);
        model.rotate(angle, 0.0f, 1.0f, 0.0f);
        model.scale(1.2f, 1.2f, 1.2f);

        shaderProgram->setUniformValue("mvpMatrix", projection * view * model);
        shaderProgram->setUniformValue("modelMatrix", model);
        shaderProgram->setUniformValue("viewMatrix", view);
        shaderProgram->setUniformValue("projectionMatrix", projection);
        shaderProgram->setUniformValue("lightPos", lightPos);
        shaderProgram->setUniformValue("viewPos", cameraPosition);
        shaderProgram->setUniformValue("lightColor", lightColor);

        shaderProgram->setUniformValue("useTexture", true);
        bladeTexture->bind();

        drawSword();

        bladeTexture->release();
    }

    // Dessiner tous les projectiles avec éclairage cohérent
    glDisable(GL_CULL_FACE);  // Eviter disparition de faces en rotation

    for (int i = 0; i < projectiles.size(); ++i) {
        if (!projectiles[i].isActive()) continue;

        // Les projectiles doivent gérer leurs propres uniforms (matrices + éclairage)
        projectiles[i].render(shaderProgram, projection, view, cameraPosition);
    }

    glEnable(GL_CULL_FACE);

    // Ajouter les projectiles en attente (fragments)
    if (!pendingProjectiles.isEmpty()) {
        projectiles.append(pendingProjectiles);
        pendingProjectiles.clear();
    }

    shaderProgram->release();
}


void OpenGLWidget::drawCylinder() {
    const int slices = 48;
    const float h = cylinderHeight;
    const float r = cylinderRadius;
    QVector<QVector3D> vertices;

    // Cercle bas
    for (int i = 0; i < slices; ++i) {
        float theta = float(i) / slices * 2.0f * M_PI;
        vertices.append(QVector3D(r * std::cos(theta), -h / 2, r * std::sin(theta)));
    }

    // Cercle haut
    for (int i = 0; i < slices; ++i) {
        float theta = float(i) / slices * 2.0f * M_PI;
        vertices.append(QVector3D(r * std::cos(theta), h / 2, r * std::sin(theta)));
    }

    // Lignes verticales pour visualiser la surface
    for (int i = 0; i < slices; i += 4) { // Moins de lignes pour la clarté
        float theta = float(i) / slices * 2.0f * M_PI;
        float x = r * std::cos(theta);
        float z = r * std::sin(theta);
        vertices.append(QVector3D(x, -h / 2, z));
        vertices.append(QVector3D(x, h / 2, z));
    }

    // Envoi au VBO
    vbo.bind();
    vbo.allocate(vertices.constData(), vertices.size() * sizeof(QVector3D));

    // Dessin des cercles
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glDrawArrays(GL_LINE_LOOP, 0, slices);
    glDrawArrays(GL_LINE_LOOP, slices, slices);

    // Dessin des lignes verticales
    glDrawArrays(GL_LINES, 2 * slices, vertices.size() - 2 * slices);

    glDisableVertexAttribArray(0);

    vbo.release();
}


void OpenGLWidget::drawSphere(float radius, int lats, int longs) {
    QVector<QVector3D> vertices;

    for (int i = 0; i <= lats; ++i) {
        float lat0 = M_PI * (-0.5 + float(i - 1) / lats);
        float z0 = radius * std::sin(lat0);
        float zr0 = radius * std::cos(lat0);

        float lat1 = M_PI * (-0.5 + float(i) / lats);
        float z1 = radius * std::sin(lat1);
        float zr1 = radius * std::cos(lat1);

        for (int j = 0; j <= longs; ++j) {
            float lng = 2 * M_PI * float(j - 1) / longs;
            float x = std::cos(lng);
            float y = std::sin(lng);
            vertices.append(QVector3D(x * zr0, y * zr0, z0));
            vertices.append(QVector3D(x * zr1, y * zr1, z1));
        }
    }

    // Upload vertices to VBO
    vbo.bind();
    vbo.allocate(vertices.constData(), vertices.size() * sizeof(QVector3D));

    // Draw sphere
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glDrawArrays(GL_QUAD_STRIP, 0, vertices.size());
    glDisableVertexAttribArray(0);

    vbo.release();
}

void OpenGLWidget::timerEvent(QTimerEvent* /*event*/) {
    // Calculate delta time
    deltaTime = elapsedTimer.elapsed() / 1000.0f;
    elapsedTimer.restart();

    gameTime += deltaTime;

    // Update camera based on current inputs
    updateCamera();

    // Spawn projectiles at intervals
    if (gameTime - lastSpawnTime > spawnInterval) {
        spawnProjectile();
        lastSpawnTime = gameTime;
    }

    updateProjectiles(deltaTime);
    checkCollisions();
    update(); // Request repaint
}

void OpenGLWidget::spawnProjectile() {
    // Select a random type for the projectile
    Projectile::Type type;
    int typeRandom = QRandomGenerator::global()->bounded(4);

    if (typeRandom == 0) {
        type = Projectile::Type::BANANA;
    } else if (typeRandom == 1) {
        type = Projectile::Type::APPLE;
    } else if (typeRandom == 2) {
        type = Projectile::Type::ANANAS;
    } else {
        type = Projectile::Type::FRAISE;
    }

    // Start position - spawn from the zone at cylinder height
    float x = QRandomGenerator::global()->bounded(-200, 200) / 100.0f; // -2.0 to 2.0 range
    float y = QRandomGenerator::global()->bounded(-150, 50) / 100.0f;  // -1.5 to 0.5 range (centered around -0.5)
    float z = -5.0f;

    // Calculate a target point on the cylinder surface
    float targetAngle = QRandomGenerator::global()->bounded(628) / 100.0f; // 0 to 2π
    float targetHeight = QRandomGenerator::global()->bounded(-80, 80) / 100.0f * cylinderHeight/2;

    // Cylinder is at z=2.5f and y=-0.5f
    float targetX = cylinderRadius * std::cos(targetAngle);
    float targetY = targetHeight - 0.5f; // Adjust for lowered cylinder
    float targetZ = 2.5f + cylinderRadius * std::sin(targetAngle);

    // Calculate distance to target
    float dx = targetX - x;
    float dz = targetZ - z;

    // Time to reach target
    float time = QRandomGenerator::global()->bounded(80, 120) / 100.0f; // 0.8-1.2 seconds

    // Parabolic trajectory calculation
    // For a parabolic trajectory with peak height h above starting point:
    // vy_initial = (2*h/time) + ((targetY-y)/time)
    float peakHeight = QRandomGenerator::global()->bounded(150, 250) / 100.0f; // Peak 1.5-2.5 units above start

    // Calculate velocity with enhanced parabolic arc
    float vx = dx / time;
    float vy = (2.0f * peakHeight / time) + ((targetY - y) / time); // Parabolic formula
    float vz = dz / time;

    // Add small random variations
    vx += QRandomGenerator::global()->bounded(-10, 10) / 100.0f;
    vz += QRandomGenerator::global()->bounded(-10, 10) / 100.0f;

    // Create and add projectile
    Projectile p(type, QVector3D(x, y, z), QVector3D(vx, vy, vz));
    p.initializeGL();
    projectiles.append(p);
}

void OpenGLWidget::drawSpawningZone() {
    // Draw a white square to mark the spawning zone - moved to cylinder height
    QVector<QVector3D> vertices = {
        QVector3D(-2.0f, -1.5f, -5.0f),  // Bottom-left, at cylinder height
        QVector3D(2.0f, -1.5f, -5.0f),   // Bottom-right
        QVector3D(2.0f, 0.5f, -5.0f),    // Top-right
        QVector3D(-2.0f, 0.5f, -5.0f)    // Top-left
    };

    vbo.bind();
    vbo.allocate(vertices.constData(), vertices.size() * sizeof(QVector3D));

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    shaderProgram->setUniformValue("color", QVector4D(1.0f, 1.0f, 1.0f, 0.5f)); // White with transparency
    glDrawArrays(GL_LINE_LOOP, 0, vertices.size());

    glDisableVertexAttribArray(0);
    vbo.release();
}

void OpenGLWidget::drawSword() {
    // Dimensions
    const float bladeLength = 0.55f;
    const float bladeWidth = 0.07f;
    const float handleLength = 0.25f;
    const float handleWidth = 0.05f;
    const float guardWidth = 0.1f;
    const float guardHeight = 0.02f;
    const float thickness = 0.03f;

    struct VertexData {
        QVector3D position;
        QVector2D texCoord;
    };
    QVector<VertexData> vertices;

    // === LAME ===
    // Front face (triangle)
    vertices.append({QVector3D(-bladeWidth/2, 0, thickness/2), QVector2D(0.0f, 0.0f)});
    vertices.append({QVector3D(bladeWidth/2, 0, thickness/2), QVector2D(1.0f, 0.0f)});
    vertices.append({QVector3D(0, bladeLength, thickness/2), QVector2D(0.5f, 1.0f)});

    // Back face (triangle)
    vertices.append({QVector3D(bladeWidth/2, 0, -thickness/2), QVector2D(1.0f, 0.0f)});
    vertices.append({QVector3D(-bladeWidth/2, 0, -thickness/2), QVector2D(0.0f, 0.0f)});
    vertices.append({QVector3D(0, bladeLength, -thickness/2), QVector2D(0.5f, 1.0f)});

    // Left side face (rectangle = 2 triangles)
    vertices.append({QVector3D(-bladeWidth/2, 0, thickness/2), QVector2D(0.0f, 0.0f)});
    vertices.append({QVector3D(0, bladeLength, thickness/2), QVector2D(0.5f, 1.0f)});
    vertices.append({QVector3D(0, bladeLength, -thickness/2), QVector2D(0.5f, 1.0f)});

    vertices.append({QVector3D(0, bladeLength, -thickness/2), QVector2D(0.5f, 1.0f)});
    vertices.append({QVector3D(-bladeWidth/2, 0, -thickness/2), QVector2D(0.0f, 0.0f)});
    vertices.append({QVector3D(-bladeWidth/2, 0, thickness/2), QVector2D(0.0f, 0.0f)});

    // Right side face (rectangle = 2 triangles)
    vertices.append({QVector3D(bladeWidth/2, 0, thickness/2), QVector2D(1.0f, 0.0f)});
    vertices.append({QVector3D(0, bladeLength, -thickness/2), QVector2D(0.5f, 1.0f)});
    vertices.append({QVector3D(0, bladeLength, thickness/2), QVector2D(0.5f, 1.0f)});

    vertices.append({QVector3D(0, bladeLength, -thickness/2), QVector2D(0.5f, 1.0f)});
    vertices.append({QVector3D(bladeWidth/2, 0, thickness/2), QVector2D(1.0f, 0.0f)});
    vertices.append({QVector3D(bladeWidth/2, 0, -thickness/2), QVector2D(1.0f, 0.0f)});

    // Bottom edge (rectangle = 2 triangles)
    vertices.append({QVector3D(-bladeWidth/2, 0, thickness/2), QVector2D(0.0f, 0.0f)});
    vertices.append({QVector3D(-bladeWidth/2, 0, -thickness/2), QVector2D(0.0f, 0.0f)});
    vertices.append({QVector3D(bladeWidth/2, 0, -thickness/2), QVector2D(1.0f, 0.0f)});

    vertices.append({QVector3D(bladeWidth/2, 0, -thickness/2), QVector2D(1.0f, 0.0f)});
    vertices.append({QVector3D(bladeWidth/2, 0, thickness/2), QVector2D(1.0f, 0.0f)});
    vertices.append({QVector3D(-bladeWidth/2, 0, thickness/2), QVector2D(0.0f, 0.0f)});

    const int bladeVertexCount = 24;

    // === GARDE (Cross-guard) ===
    // Couleur unie, pas de texture
    // Front face
    vertices.append({QVector3D(-guardWidth/2, -guardHeight, thickness/2), QVector2D(0.0f, 0.0f)});
    vertices.append({QVector3D(guardWidth/2, -guardHeight, thickness/2), QVector2D(1.0f, 0.0f)});
    vertices.append({QVector3D(guardWidth/2, 0, thickness/2), QVector2D(1.0f, 1.0f)});

    vertices.append({QVector3D(guardWidth/2, 0, thickness/2), QVector2D(1.0f, 1.0f)});
    vertices.append({QVector3D(-guardWidth/2, 0, thickness/2), QVector2D(0.0f, 1.0f)});
    vertices.append({QVector3D(-guardWidth/2, -guardHeight, thickness/2), QVector2D(0.0f, 0.0f)});

    // Back face
    vertices.append({QVector3D(-guardWidth/2, -guardHeight, -thickness/2), QVector2D(0.0f, 0.0f)});
    vertices.append({QVector3D(guardWidth/2, 0, -thickness/2), QVector2D(1.0f, 1.0f)});
    vertices.append({QVector3D(guardWidth/2, -guardHeight, -thickness/2), QVector2D(1.0f, 0.0f)});

    vertices.append({QVector3D(guardWidth/2, 0, -thickness/2), QVector2D(1.0f, 1.0f)});
    vertices.append({QVector3D(-guardWidth/2, -guardHeight, -thickness/2), QVector2D(0.0f, 0.0f)});
    vertices.append({QVector3D(-guardWidth/2, 0, -thickness/2), QVector2D(0.0f, 1.0f)});

    // Top edge
    vertices.append({QVector3D(-guardWidth/2, -guardHeight, thickness/2), QVector2D(0.0f, 0.0f)});
    vertices.append({QVector3D(guardWidth/2, -guardHeight, thickness/2), QVector2D(1.0f, 0.0f)});
    vertices.append({QVector3D(guardWidth/2, -guardHeight, -thickness/2), QVector2D(1.0f, 1.0f)});

    vertices.append({QVector3D(guardWidth/2, -guardHeight, -thickness/2), QVector2D(1.0f, 1.0f)});
    vertices.append({QVector3D(-guardWidth/2, -guardHeight, -thickness/2), QVector2D(0.0f, 1.0f)});
    vertices.append({QVector3D(-guardWidth/2, -guardHeight, thickness/2), QVector2D(0.0f, 0.0f)});

    // Left edge
    vertices.append({QVector3D(-guardWidth/2, -guardHeight, thickness/2), QVector2D(0.0f, 0.0f)});
    vertices.append({QVector3D(-guardWidth/2, -guardHeight, -thickness/2), QVector2D(1.0f, 0.0f)});
    vertices.append({QVector3D(-guardWidth/2, 0, -thickness/2), QVector2D(1.0f, 1.0f)});

    vertices.append({QVector3D(-guardWidth/2, 0, -thickness/2), QVector2D(1.0f, 1.0f)});
    vertices.append({QVector3D(-guardWidth/2, 0, thickness/2), QVector2D(0.0f, 1.0f)});
    vertices.append({QVector3D(-guardWidth/2, -guardHeight, thickness/2), QVector2D(0.0f, 0.0f)});

    // Right edge
    vertices.append({QVector3D(guardWidth/2, -guardHeight, thickness/2), QVector2D(0.0f, 0.0f)});
    vertices.append({QVector3D(guardWidth/2, 0, -thickness/2), QVector2D(1.0f, 1.0f)});
    vertices.append({QVector3D(guardWidth/2, -guardHeight, -thickness/2), QVector2D(1.0f, 0.0f)});

    vertices.append({QVector3D(guardWidth/2, 0, -thickness/2), QVector2D(1.0f, 1.0f)});
    vertices.append({QVector3D(guardWidth/2, -guardHeight, thickness/2), QVector2D(0.0f, 0.0f)});
    vertices.append({QVector3D(guardWidth/2, 0, thickness/2), QVector2D(0.0f, 1.0f)});

    const int guardVertexCount = 30;

    // === MANCHE ===
    // Front face
    vertices.append({QVector3D(-handleWidth/2, -handleLength - guardHeight, thickness/2), QVector2D(0.0f, 0.0f)});
    vertices.append({QVector3D(handleWidth/2, -handleLength - guardHeight, thickness/2), QVector2D(1.0f, 0.0f)});
    vertices.append({QVector3D(handleWidth/2, -guardHeight, thickness/2), QVector2D(1.0f, 1.0f)});

    vertices.append({QVector3D(handleWidth/2, -guardHeight, thickness/2), QVector2D(1.0f, 1.0f)});
    vertices.append({QVector3D(-handleWidth/2, -guardHeight, thickness/2), QVector2D(0.0f, 1.0f)});
    vertices.append({QVector3D(-handleWidth/2, -handleLength - guardHeight, thickness/2), QVector2D(0.0f, 0.0f)});

    // Back face
    vertices.append({QVector3D(-handleWidth/2, -handleLength - guardHeight, -thickness/2), QVector2D(0.0f, 0.0f)});
    vertices.append({QVector3D(handleWidth/2, -guardHeight, -thickness/2), QVector2D(1.0f, 1.0f)});
    vertices.append({QVector3D(handleWidth/2, -handleLength - guardHeight, -thickness/2), QVector2D(1.0f, 0.0f)});

    vertices.append({QVector3D(handleWidth/2, -guardHeight, -thickness/2), QVector2D(1.0f, 1.0f)});
    vertices.append({QVector3D(-handleWidth/2, -handleLength - guardHeight, -thickness/2), QVector2D(0.0f, 0.0f)});
    vertices.append({QVector3D(-handleWidth/2, -guardHeight, -thickness/2), QVector2D(0.0f, 1.0f)});

    // Bottom edge
    vertices.append({QVector3D(-handleWidth/2, -handleLength - guardHeight, thickness/2), QVector2D(0.0f, 0.0f)});
    vertices.append({QVector3D(handleWidth/2, -handleLength - guardHeight, thickness/2), QVector2D(1.0f, 0.0f)});
    vertices.append({QVector3D(handleWidth/2, -handleLength - guardHeight, -thickness/2), QVector2D(1.0f, 0.0f)});

    vertices.append({QVector3D(handleWidth/2, -handleLength - guardHeight, -thickness/2), QVector2D(1.0f, 0.0f)});
    vertices.append({QVector3D(-handleWidth/2, -handleLength - guardHeight, -thickness/2), QVector2D(0.0f, 0.0f)});
    vertices.append({QVector3D(-handleWidth/2, -handleLength - guardHeight, thickness/2), QVector2D(0.0f, 0.0f)});

    // Left edge
    vertices.append({QVector3D(-handleWidth/2, -handleLength - guardHeight, thickness/2), QVector2D(0.0f, 0.0f)});
    vertices.append({QVector3D(-handleWidth/2, -handleLength - guardHeight, -thickness/2), QVector2D(1.0f, 0.0f)});
    vertices.append({QVector3D(-handleWidth/2, -guardHeight, -thickness/2), QVector2D(1.0f, 1.0f)});

    vertices.append({QVector3D(-handleWidth/2, -guardHeight, -thickness/2), QVector2D(1.0f, 1.0f)});
    vertices.append({QVector3D(-handleWidth/2, -guardHeight, thickness/2), QVector2D(0.0f, 1.0f)});
    vertices.append({QVector3D(-handleWidth/2, -handleLength - guardHeight, thickness/2), QVector2D(0.0f, 0.0f)});

    // Right edge
    vertices.append({QVector3D(handleWidth/2, -handleLength - guardHeight, thickness/2), QVector2D(0.0f, 0.0f)});
    vertices.append({QVector3D(handleWidth/2, -guardHeight, -thickness/2), QVector2D(1.0f, 1.0f)});
    vertices.append({QVector3D(handleWidth/2, -handleLength - guardHeight, -thickness/2), QVector2D(1.0f, 0.0f)});

    vertices.append({QVector3D(handleWidth/2, -guardHeight, -thickness/2), QVector2D(1.0f, 1.0f)});
    vertices.append({QVector3D(handleWidth/2, -handleLength - guardHeight, thickness/2), QVector2D(0.0f, 0.0f)});
    vertices.append({QVector3D(handleWidth/2, -guardHeight, thickness/2), QVector2D(0.0f, 1.0f)});

    const int handleVertexCount = 30;

    // Upload vertices to VBO
    vbo.bind();
    vbo.allocate(vertices.constData(), vertices.size() * sizeof(VertexData));

    // Enable vertex attributes
    glEnableVertexAttribArray(0); // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), nullptr);

    glEnableVertexAttribArray(2); // texCoord
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(VertexData), reinterpret_cast<void*>(offsetof(VertexData, texCoord)));

    // Draw blade with blade texture
    bladeTexture->bind();
    shaderProgram->setUniformValue("useTexture", true);
    glDrawArrays(GL_TRIANGLES, 0, bladeVertexCount);
    bladeTexture->release();

    // Draw guard with solid color (no texture)
    shaderProgram->setUniformValue("useTexture", false);
    shaderProgram->setUniformValue("color", QVector4D(0.9f, 0.8f, 0.2f, 1.0f));  // Gold
    glDrawArrays(GL_TRIANGLES, bladeVertexCount, guardVertexCount);

    // Draw handle with handle texture
    handleTexture->bind();
    shaderProgram->setUniformValue("useTexture", true);
    glDrawArrays(GL_TRIANGLES, bladeVertexCount + guardVertexCount, handleVertexCount);
    handleTexture->release();

    // Disable vertex attributes and release VBO
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(2);
    vbo.release();
}

void OpenGLWidget::keyPressEvent(QKeyEvent* event) {
    qDebug() << "Key pressed:" << event->key();

    // Handle arrow key presses
    switch (event->key()) {
    case Qt::Key_Up:
        keysPressed[0] = true;
        break;
    case Qt::Key_Down:
        keysPressed[1] = true;
        break;
    case Qt::Key_Left:
        keysPressed[2] = true;
        break;
    case Qt::Key_Right:
        keysPressed[3] = true;
        break;
    case Qt::Key_R:
        // Reset camera when R is pressed
        resetCamera();
        break;
    default:
        QOpenGLWidget::keyPressEvent(event);
    }

    // Force immediate update to make camera movement responsive
    update();
}

void OpenGLWidget::keyReleaseEvent(QKeyEvent* event) {
    qDebug() << "Key released:" << event->key();

    // Handle arrow key releases
    switch (event->key()) {
    case Qt::Key_Up:
        keysPressed[0] = false;
        break;
    case Qt::Key_Down:
        keysPressed[1] = false;
        break;
    case Qt::Key_Left:
        keysPressed[2] = false;
        break;
    case Qt::Key_Right:
        keysPressed[3] = false;
        break;
    default:
        QOpenGLWidget::keyReleaseEvent(event);
    }

    // Force immediate update to make camera movement responsive
    update();
}

void OpenGLWidget::resetGame() {
    // Clear all projectiles
    projectiles.clear();
    pendingProjectiles.clear();

    // Reset timers and game state
    gameTime = 0.0f;
    lastSpawnTime = 0.0f;
    score = 0;

    // Force update
    update();
}

void OpenGLWidget::updateProjectiles(float deltaTime) {
    for (int i = 0; i < projectiles.size(); i++) {
        // Add gravity effect for more realistic parabolic motion
        projectiles[i].applyGravity(deltaTime * 1.2f); // Apply gravity force for parabolic trajectory

        // Apply a reduced delta time to simulate less forces over time
        float reducedDelta = deltaTime * 0.9f; // Reduce forces by 10%
        projectiles[i].update(reducedDelta);

        // Remove inactive projectiles
        if (!projectiles[i].isActive()) {
            projectiles.removeAt(i);
            i--;
        }
    }
}

void OpenGLWidget::checkCollisions() {
    if (!handSet) return;

    // Get the current sword position with cylinder translation
    QVector3D swordPosition = handPosition + QVector3D(0, -0.5f, 2.5f); // Adjust for lowered cylinder position

    // Create a smaller "virtual cylinder" around the sword blade
    const float bladeRadius = 0.05f; // Small radius to simulate the sword blade
    const float bladeHeight = 0.3f;  // Height to cover the sword blade

    for (int i = 0; i < projectiles.size(); i++) {
        // Check if projectile intersects with the smaller "blade cylinder"
        if (projectiles[i].checkCollisionWithCylinder(bladeRadius, bladeHeight, swordPosition)) {
            // Check if this is an original projectile (not a fragment)
            bool isOriginal = !projectiles[i].isFragment();

            // Slice the projectile and create fragments
            std::vector<Projectile> fragments = projectiles[i].slice();

            // Remove the original projectile
            projectiles.removeAt(i);

            // Add fragments to pending projectiles
            for (const auto& fragment : fragments) {
                pendingProjectiles.append(fragment);
            }

            // Only emit score increased if this was an original projectile
            if (isOriginal) {
                emit scoreIncreased();
            }

            // Adjust index since we removed an element
            i--;
        }
    }
}

void OpenGLWidget::updateCamera() {
    // Apply keyboard input to camera
    if (keysPressed[0]) { // Up arrow
        cameraPitch += cameraRotationSpeed * deltaTime;
        qDebug() << "Moving camera up, pitch:" << cameraPitch;
    }
    if (keysPressed[1]) { // Down arrow
        cameraPitch -= cameraRotationSpeed * deltaTime;
        qDebug() << "Moving camera down, pitch:" << cameraPitch;
    }
    if (keysPressed[2]) { // Left arrow
        cameraYaw -= cameraRotationSpeed * deltaTime;
        qDebug() << "Moving camera left, yaw:" << cameraYaw;
    }
    if (keysPressed[3]) { // Right arrow
        cameraYaw += cameraRotationSpeed * deltaTime;
        qDebug() << "Moving camera right, yaw:" << cameraYaw;
    }

    // Clamp pitch to prevent camera flipping
    if (cameraPitch > 89.0f) cameraPitch = 89.0f;
    if (cameraPitch < -89.0f) cameraPitch = -89.0f;

    // Calculate camera position in spherical coordinates
    float pitch_rad = qDegreesToRadians(cameraPitch);
    float yaw_rad = qDegreesToRadians(cameraYaw);

    cameraPosition.setX(cameraDistance * qCos(pitch_rad) * qSin(yaw_rad));
    cameraPosition.setY(cameraDistance * qSin(pitch_rad));
    cameraPosition.setZ(cameraDistance * qCos(pitch_rad) * qCos(yaw_rad));

    // Set up view matrix
    view.setToIdentity();
    view.lookAt(cameraPosition, QVector3D(0, 0, 0), QVector3D(0, 1, 0));
}
