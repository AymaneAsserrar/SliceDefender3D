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

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Compile and link shaders
    shaderProgram = new QOpenGLShaderProgram(this);
    shaderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, R"(
        #version 330 core
        layout(location = 0) in vec3 position;
        uniform mat4 mvpMatrix;
        void main() {
            gl_Position = mvpMatrix * vec4(position, 1.0);
        }
    )");
    shaderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, R"(
        #version 330 core
        out vec4 fragColor;
        uniform vec4 color;
        void main() {
            fragColor = color;
        }
    )");
    shaderProgram->link();

    // Initialize vertex buffer object (VBO)
    vbo.create();

    // Start the game timer
    elapsedTimer.start();
    timerId = startTimer(16); // ~60 FPS
    
    // Position knife at center of cylinder facing projectiles
    // Use 0.5 for normX to place at front of cylinder (180 degrees on cylinder)
    // Use 0.5 for normY to place at center height of cylinder
    setHandPosition(0.5f, 0.5f);
    
    // Initialize camera position
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
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    projection.setToIdentity();
    projection.perspective(45.0f, float(width()) / height(), 0.1f, 100.0f);
    view.setToIdentity();
    view.lookAt(cameraPosition, QVector3D(0, 0, 0), QVector3D(0, 1, 0));

    shaderProgram->bind();
    shaderProgram->setUniformValue("mvpMatrix", projection * view);

    // Draw the spawning zone marker
    drawSpawningZone();

    QMatrix4x4 model;
    QMatrix4x4 mvp = projection * view * model;

    shaderProgram->bind();
    shaderProgram->setUniformValue("mvpMatrix", mvp);

    // Disable depth writing (but keep testing) for transparent objects
    glDepthMask(GL_FALSE);
    
    // Draw cylinder - move it closer to the camera and slightly lower
    model.setToIdentity();
    model.translate(0, -0.5f, 2.5f); // Move cylinder halfway toward camera and lower by 0.5 units
    mvp = projection * view * model;
    shaderProgram->setUniformValue("mvpMatrix", mvp);
    shaderProgram->setUniformValue("color", QVector4D(0.2f, 0.7f, 1.0f, 0.4f)); // More transparent
    drawCylinder();
    
    // Re-enable depth writing for non-transparent objects
    glDepthMask(GL_TRUE);

    // Reset model matrix for other objects
    model.setToIdentity();
    
    // Draw hand marker
    if (handSet) {
        model.setToIdentity();
        
        // Calculate rotation to point outward from cylinder center
        float angle = atan2(handZ, handX) * 180.0f / M_PI;
        
        // Position at hand location and adjust for the cylinder translation (including the Y offset)
        model.translate(handX, handY - 0.5f, handZ + 2.5f);  // Add cylinder's z-translation and subtract y-offset
        
        // Rotate to point outward from cylinder
        model.rotate(angle, 0.0f, 1.0f, 0.0f);  // Y-axis rotation
        
        // Remove the Z-axis rotation that was causing the sword to be horizontal
        // The sword geometry is already defined to point up in Y direction
        
        // Scale to appropriate size
        model.scale(1.2f, 1.2f, 1.2f);  // Doubled from 0.6f to make sword twice as big
        
        mvp = projection * view * model;
        shaderProgram->setUniformValue("mvpMatrix", mvp);
        
        drawSword();
    }

    // Render all active projectiles - temporarily disable back-face culling
    // to prevent faces from disappearing during rotation
    glDisable(GL_CULL_FACE);
    
    for (int i = 0; i < projectiles.size(); i++) {
        projectiles[i].render(shaderProgram, projection, view);
    }
    
    // Re-enable back-face culling after drawing projectiles
    glEnable(GL_CULL_FACE);

    // Add any pending projectiles (fragments from slicing)
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

    // Bottom circle
    for (int i = 0; i < slices; ++i) {
        float theta = float(i) / slices * 2.0f * M_PI;
        vertices.append(QVector3D(r * std::cos(theta), -h / 2, r * std::sin(theta)));
    }

    // Top circle
    for (int i = 0; i < slices; ++i) {
        float theta = float(i) / slices * 2.0f * M_PI;
        vertices.append(QVector3D(r * std::cos(theta), h / 2, r * std::sin(theta)));
    }

    // Vertical lines to better visualize the cylinder surface
    for (int i = 0; i < slices; i += 4) { // Draw fewer lines for clarity
        float theta = float(i) / slices * 2.0f * M_PI;
        float x = r * std::cos(theta);
        float z = r * std::sin(theta);
        vertices.append(QVector3D(x, -h / 2, z));
        vertices.append(QVector3D(x, h / 2, z));
    }

    // Upload vertices to VBO
    vbo.bind();
    vbo.allocate(vertices.constData(), vertices.size() * sizeof(QVector3D));

    // Draw bottom and top circles
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glDrawArrays(GL_LINE_LOOP, 0, slices);
    glDrawArrays(GL_LINE_LOOP, slices, slices);
    
    // Draw vertical lines
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
    int typeRand = QRandomGenerator::global()->bounded(3); // 0, 1, or 2
    
    if (typeRand == 0) {
        type = Projectile::Type::CUBE;
    } else if (typeRand == 1) {
        type = Projectile::Type::PRISM;
    } else {
        type = Projectile::Type::CONE;
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
    // Sword dimensions
    const float bladeLength = 0.25f;    // Blade length
    const float bladeWidth = 0.06f;     // Blade width
    const float handleLength = 0.15f;   // Handle length
    const float handleWidth = 0.03f;    // Handle width
    const float guardWidth = 0.1f;      // Guard width
    const float guardHeight = 0.02f;    // Guard height
    const float thickness = 0.03f;      // Increased thickness for 3D look
    
    QVector<QVector3D> vertices;
    
    // BLADE
    // Front face (triangular)
    vertices.append(QVector3D(-bladeWidth/2, 0, thickness/2));        // Bottom left
    vertices.append(QVector3D(bladeWidth/2, 0, thickness/2));         // Bottom right
    vertices.append(QVector3D(0, bladeLength, thickness/2));          // Top point
    
    // Back face (triangular)
    vertices.append(QVector3D(bladeWidth/2, 0, -thickness/2));        // Bottom right
    vertices.append(QVector3D(-bladeWidth/2, 0, -thickness/2));       // Bottom left  
    vertices.append(QVector3D(0, bladeLength, -thickness/2));         // Top point
    
    // Left side face
    vertices.append(QVector3D(-bladeWidth/2, 0, thickness/2));        // Front bottom
    vertices.append(QVector3D(0, bladeLength, thickness/2));          // Front top
    vertices.append(QVector3D(0, bladeLength, -thickness/2));         // Back top
    
    vertices.append(QVector3D(0, bladeLength, -thickness/2));         // Back top
    vertices.append(QVector3D(-bladeWidth/2, 0, -thickness/2));       // Back bottom
    vertices.append(QVector3D(-bladeWidth/2, 0, thickness/2));        // Front bottom
    
    // Right side face
    vertices.append(QVector3D(bladeWidth/2, 0, thickness/2));         // Front bottom
    vertices.append(QVector3D(0, bladeLength, -thickness/2));         // Back top
    vertices.append(QVector3D(0, bladeLength, thickness/2));          // Front top
    
    vertices.append(QVector3D(0, bladeLength, -thickness/2));         // Back top
    vertices.append(QVector3D(bladeWidth/2, 0, thickness/2));         // Front bottom
    vertices.append(QVector3D(bladeWidth/2, 0, -thickness/2));        // Back bottom
    
    // Bottom edge
    vertices.append(QVector3D(-bladeWidth/2, 0, thickness/2));        // Front left
    vertices.append(QVector3D(-bladeWidth/2, 0, -thickness/2));       // Back left
    vertices.append(QVector3D(bladeWidth/2, 0, -thickness/2));        // Back right
    
    vertices.append(QVector3D(bladeWidth/2, 0, -thickness/2));        // Back right
    vertices.append(QVector3D(bladeWidth/2, 0, thickness/2));         // Front right
    vertices.append(QVector3D(-bladeWidth/2, 0, thickness/2));        // Front left
    
    // CROSS-GUARD
    // Front face
    vertices.append(QVector3D(-guardWidth/2, -guardHeight, thickness/2));  // Top left
    vertices.append(QVector3D(guardWidth/2, -guardHeight, thickness/2));   // Top right
    vertices.append(QVector3D(guardWidth/2, 0, thickness/2));             // Bottom right
    
    vertices.append(QVector3D(guardWidth/2, 0, thickness/2));             // Bottom right
    vertices.append(QVector3D(-guardWidth/2, 0, thickness/2));            // Bottom left
    vertices.append(QVector3D(-guardWidth/2, -guardHeight, thickness/2)); // Top left
    
    // Back face
    vertices.append(QVector3D(-guardWidth/2, -guardHeight, -thickness/2));  // Top left
    vertices.append(QVector3D(guardWidth/2, 0, -thickness/2));              // Bottom right
    vertices.append(QVector3D(guardWidth/2, -guardHeight, -thickness/2));   // Top right
    
    vertices.append(QVector3D(guardWidth/2, 0, -thickness/2));              // Bottom right
    vertices.append(QVector3D(-guardWidth/2, -guardHeight, -thickness/2));  // Top left
    vertices.append(QVector3D(-guardWidth/2, 0, -thickness/2));             // Bottom left
    
    // Top edge
    vertices.append(QVector3D(-guardWidth/2, -guardHeight, thickness/2));   // Front left
    vertices.append(QVector3D(guardWidth/2, -guardHeight, thickness/2));    // Front right
    vertices.append(QVector3D(guardWidth/2, -guardHeight, -thickness/2));   // Back right
    
    vertices.append(QVector3D(guardWidth/2, -guardHeight, -thickness/2));   // Back right
    vertices.append(QVector3D(-guardWidth/2, -guardHeight, -thickness/2));  // Back left
    vertices.append(QVector3D(-guardWidth/2, -guardHeight, thickness/2));   // Front left
    
    // Left edge
    vertices.append(QVector3D(-guardWidth/2, -guardHeight, thickness/2));   // Top front
    vertices.append(QVector3D(-guardWidth/2, -guardHeight, -thickness/2));  // Top back
    vertices.append(QVector3D(-guardWidth/2, 0, -thickness/2));             // Bottom back
    
    vertices.append(QVector3D(-guardWidth/2, 0, -thickness/2));             // Bottom back
    vertices.append(QVector3D(-guardWidth/2, 0, thickness/2));              // Bottom front
    vertices.append(QVector3D(-guardWidth/2, -guardHeight, thickness/2));   // Top front
    
    // Right edge
    vertices.append(QVector3D(guardWidth/2, -guardHeight, thickness/2));    // Top front
    vertices.append(QVector3D(guardWidth/2, 0, -thickness/2));              // Bottom back
    vertices.append(QVector3D(guardWidth/2, -guardHeight, -thickness/2));   // Top back
    
    vertices.append(QVector3D(guardWidth/2, 0, -thickness/2));              // Bottom back
    vertices.append(QVector3D(guardWidth/2, -guardHeight, thickness/2));    // Top front
    vertices.append(QVector3D(guardWidth/2, 0, thickness/2));               // Bottom front
    
    // HANDLE
    // Front face
    vertices.append(QVector3D(-handleWidth/2, -handleLength-guardHeight, thickness/2));  // Bottom left
    vertices.append(QVector3D(handleWidth/2, -handleLength-guardHeight, thickness/2));   // Bottom right
    vertices.append(QVector3D(handleWidth/2, -guardHeight, thickness/2));               // Top right
    
    vertices.append(QVector3D(handleWidth/2, -guardHeight, thickness/2));               // Top right
    vertices.append(QVector3D(-handleWidth/2, -guardHeight, thickness/2));              // Top left
    vertices.append(QVector3D(-handleWidth/2, -handleLength-guardHeight, thickness/2)); // Bottom left
    
    // Back face
    vertices.append(QVector3D(-handleWidth/2, -handleLength-guardHeight, -thickness/2)); // Bottom left
    vertices.append(QVector3D(handleWidth/2, -guardHeight, -thickness/2));               // Top right
    vertices.append(QVector3D(handleWidth/2, -handleLength-guardHeight, -thickness/2));  // Bottom right
    
    vertices.append(QVector3D(handleWidth/2, -guardHeight, -thickness/2));               // Top right
    vertices.append(QVector3D(-handleWidth/2, -handleLength-guardHeight, -thickness/2)); // Bottom left
    vertices.append(QVector3D(-handleWidth/2, -guardHeight, -thickness/2));              // Top left
    
    // Bottom edge
    vertices.append(QVector3D(-handleWidth/2, -handleLength-guardHeight, thickness/2));  // Front left
    vertices.append(QVector3D(handleWidth/2, -handleLength-guardHeight, thickness/2));   // Front right
    vertices.append(QVector3D(handleWidth/2, -handleLength-guardHeight, -thickness/2));  // Back right
    
    vertices.append(QVector3D(handleWidth/2, -handleLength-guardHeight, -thickness/2));  // Back right
    vertices.append(QVector3D(-handleWidth/2, -handleLength-guardHeight, -thickness/2)); // Back left
    vertices.append(QVector3D(-handleWidth/2, -handleLength-guardHeight, thickness/2));  // Front left
    
    // Left edge
    vertices.append(QVector3D(-handleWidth/2, -handleLength-guardHeight, thickness/2));  // Bottom front
    vertices.append(QVector3D(-handleWidth/2, -handleLength-guardHeight, -thickness/2)); // Bottom back
    vertices.append(QVector3D(-handleWidth/2, -guardHeight, -thickness/2));              // Top back
    
    vertices.append(QVector3D(-handleWidth/2, -guardHeight, -thickness/2));              // Top back
    vertices.append(QVector3D(-handleWidth/2, -guardHeight, thickness/2));               // Top front
    vertices.append(QVector3D(-handleWidth/2, -handleLength-guardHeight, thickness/2));  // Bottom front
    
    // Right edge
    vertices.append(QVector3D(handleWidth/2, -handleLength-guardHeight, thickness/2));   // Bottom front
    vertices.append(QVector3D(handleWidth/2, -guardHeight, -thickness/2));               // Top back
    vertices.append(QVector3D(handleWidth/2, -handleLength-guardHeight, -thickness/2));  // Bottom back
    
    vertices.append(QVector3D(handleWidth/2, -guardHeight, -thickness/2));               // Top back
    vertices.append(QVector3D(handleWidth/2, -handleLength-guardHeight, thickness/2));   // Bottom front
    vertices.append(QVector3D(handleWidth/2, -guardHeight, thickness/2));                // Top front
    
    // Upload vertices to VBO
    vbo.bind();
    vbo.allocate(vertices.constData(), vertices.size() * sizeof(QVector3D));
    
    // Draw the blade (metallic gray)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    shaderProgram->setUniformValue("color", QVector4D(0.8f, 0.8f, 0.9f, 1.0f));  // Metallic color
    glDrawArrays(GL_TRIANGLES, 0, 24);  // Draw blade (increased number of faces)
    
    // Draw the cross-guard (gold/yellow)
    shaderProgram->setUniformValue("color", QVector4D(0.9f, 0.8f, 0.2f, 1.0f));  // Gold color
    glDrawArrays(GL_TRIANGLES, 24, 30); // Draw guard (increased number of faces)
    
    // Draw the handle (wooden brown)
    shaderProgram->setUniformValue("color", QVector4D(0.6f, 0.3f, 0.1f, 1.0f));  // Brown color
    glDrawArrays(GL_TRIANGLES, 54, 30); // Draw handle (increased number of faces)
    
    glDisableVertexAttribArray(0);
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