#include "OpenGLWidget.h"
#include <QtMath>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QMatrix4x4>
#include <QOpenGLFunctions>
#include <QRandomGenerator>
#include <QKeyEvent>

OpenGLWidget::OpenGLWidget(QWidget *parent) : QOpenGLWidget(parent), shaderProgram(nullptr), vbo(QOpenGLBuffer::VertexBuffer) {
    setFocusPolicy(Qt::StrongFocus);
    setFocus();
}

OpenGLWidget::~OpenGLWidget() {
    makeCurrent();
    vbo.destroy();
    delete shaderProgram;
    delete bladeTexture;
    delete handleTexture;
    delete groundTexture;
    delete wallTexture;
    delete backWallTexture;
    delete roofTexture;
    doneCurrent();
}

void OpenGLWidget::initializeGL() {
    initializeOpenGLFunctions();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    shaderProgram = new QOpenGLShaderProgram(this);

    shaderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, R"(
        #version 330 core
        layout(location = 0) in vec3 position;
        layout(location = 1) in vec3 normal;
        layout(location = 2) in vec2 texCoord;

        uniform mat4 mvpMatrix;
        uniform mat4 modelMatrix;
        uniform mat4 viewMatrix;
        uniform mat3 normalMatrix;

        out vec2 vTexCoord;
        out vec3 vNormal;
        out vec3 vPosition;
        out vec3 vViewPosition;

        void main() {
            gl_Position = mvpMatrix * vec4(position, 1.0);
            vTexCoord = texCoord;
            vNormal = normalize(normalMatrix * normal);
            vPosition = vec3(modelMatrix * vec4(position, 1.0));

                vViewPosition = vec3(viewMatrix * modelMatrix * vec4(position, 1.0));
        }
    )");

    shaderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, R"(
        #version 330 core
        in vec2 vTexCoord;
        in vec3 vNormal;
        in vec3 vPosition;
        in vec3 vViewPosition;

        out vec4 fragColor;

        uniform vec4 color;
        uniform sampler2D appleTexture;
        uniform bool useTexture;

        uniform bool useLighting = true;
        uniform vec3 lightPosition = vec3(0, 5, 0);
        uniform vec3 lightColor = vec3(1.0, 1.0, 0.9);
        uniform float ambientStrength = 0.3;
        uniform float specularStrength = 0.5;
        uniform float shininess = 32.0;

        uniform bool isFragment;
        uniform vec3 sliceNormal;
        uniform int fragmentSide;
        uniform vec4 cutSurfaceColor;

        void main() {
            vec4 baseColor;
            if (useTexture) {
                baseColor = texture(appleTexture, vTexCoord);
            } else {
                if (isFragment && gl_FrontFacing == false) {

                    baseColor = cutSurfaceColor;
                } else {
                    baseColor = color;
                }
            }
            if (!useLighting) {
                fragColor = baseColor;
                return;
            }

            vec3 normal = normalize(vNormal);

            vec3 ambient = ambientStrength * lightColor;

            vec3 lightDir = normalize(lightPosition - vPosition);
            float diff = max(dot(normal, lightDir), 0.0);
            vec3 diffuse = diff * lightColor;

            vec3 viewDir = normalize(-vViewPosition);
            vec3 halfwayDir = normalize(lightDir + viewDir);
            float spec = pow(max(dot(normal, halfwayDir), 0.0), shininess);
            vec3 specular = specularStrength * spec * lightColor;

            vec3 lighting = ambient + diffuse + specular;

            fragColor = vec4(lighting * baseColor.rgb, baseColor.a);
        }
    )");

    shaderProgram->link();

    vbo.create();
    zoneVBO = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    if (!zoneVBO.create()) {
        qWarning("Failed to create zoneVBO");
    }

    elapsedTimer.start();
    timerId = startTimer(16);

    setHandPosition(0.5f, 0.5f);

    resetCamera();

    groundTexture = new QOpenGLTexture(QImage(":/new/prefix2/resources/images/floor_texture.jpg").flipped());
    if (groundTexture) {
        groundTexture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
        groundTexture->setMagnificationFilter(QOpenGLTexture::Linear);
        groundTexture->setWrapMode(QOpenGLTexture::Repeat);
    } else {
        qWarning("Failed to load ground texture");
    }

        wallTexture = new QOpenGLTexture(QImage(":/new/prefix2/resources/images/wall_text.jpg").flipped());
    if (wallTexture) {
        wallTexture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
        wallTexture->setMagnificationFilter(QOpenGLTexture::Linear);
        wallTexture->setWrapMode(QOpenGLTexture::Repeat);
    } else {
        qWarning("Failed to load wall texture");
    }

    backWallTexture = new QOpenGLTexture(QImage(":/new/prefix2/resources/images/door_texture.jpg").flipped());
    if (backWallTexture) {
        backWallTexture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
        backWallTexture->setMagnificationFilter(QOpenGLTexture::Linear);
        backWallTexture->setWrapMode(QOpenGLTexture::Repeat);
    } else {
        qWarning("Failed to load door texture");
    }

    roofTexture = new QOpenGLTexture(QImage(":/new/prefix2/resources/images/roof_texture.jpg").flipped());
    if (roofTexture) {
        roofTexture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
        roofTexture->setMagnificationFilter(QOpenGLTexture::Linear);
        roofTexture->setWrapMode(QOpenGLTexture::Repeat);
    } else {
        qWarning("Failed to load roof texture");
    }

    bladeTexture = new QOpenGLTexture(QImage(":/new/prefix2/resources/images/blade2_texture.jpg").flipped());
    if (bladeTexture) {
        bladeTexture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
        bladeTexture->setMagnificationFilter(QOpenGLTexture::Linear);
        bladeTexture->setWrapMode(QOpenGLTexture::Repeat);
    } else {
        qWarning("Failed to load blade texture");
    }

    handleTexture = new QOpenGLTexture(QImage(":/new/prefix2/resources/images/handle_texture.jpg").flipped());
    if (handleTexture) {
        handleTexture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
        handleTexture->setMagnificationFilter(QOpenGLTexture::Linear);
        handleTexture->setWrapMode(QOpenGLTexture::Repeat);
    } else {
        qWarning("Failed to load handle texture");
    }
}

void OpenGLWidget::resetCamera() {

    cameraPosition = QVector3D(0, 0, 5);
    cameraYaw = 0.0f;
    cameraPitch = 0.0f;
    cameraDistance = 5.0f;

    for (int i = 0; i < 4; i++) {
        keysPressed[i] = false;
    }

    update();
}

void OpenGLWidget::resizeGL(int , int ) {
}

void OpenGLWidget::setHandPosition(float normX, float normY) {
    normHandX = normX;
    normHandY = normY;

    const float xRange = 0.7f;
    const float yRange = 0.8f;

    float adjustedX = 0.5f + (normX - 0.5f) * xRange;
    float adjustedY = 0.5f + (normY - 0.5f) * yRange;

    float theta;
    if (adjustedX < 0.0f) {
        adjustedX = 0.0f;
    } else if (adjustedX > 1.0f) {
        adjustedX = 1.0f;
    }
    theta = M_PI_2 + adjustedX * (3.0f * M_PI_2);

    float y = (adjustedY - 0.5f) * cylinderHeight;

    y = std::max(-cylinderHeight/2.0f + 0.1f, std::min(cylinderHeight/2.0f - 0.1f, y));

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

    float lightTime = gameTime * 0.5f;
    QVector3D lightPosition(
        3.0f * sin(lightTime),
        5.0f + 1.0f * sin(lightTime * 0.5f),
        3.0f * cos(lightTime)
    );

    shaderProgram->bind();
    shaderProgram->setUniformValue("mvpMatrix", projection * view);
    shaderProgram->setUniformValue("viewMatrix", view);

    shaderProgram->setUniformValue("useLighting", true);
    shaderProgram->setUniformValue("lightPosition", lightPosition);
    shaderProgram->setUniformValue("lightColor", QVector3D(1.0f, 1.0f, 0.9f));
    shaderProgram->setUniformValue("ambientStrength", 0.3f);
    shaderProgram->setUniformValue("specularStrength", 0.5f);
    shaderProgram->setUniformValue("shininess", 32.0f);

    drawLightSource(lightPosition);
    drawGround();
    drawWalls();
    drawRoof();
    drawSpawningZone();

    QMatrix4x4 model;
    QMatrix4x4 mvp = projection * view * model;

    shaderProgram->bind();
    shaderProgram->setUniformValue("mvpMatrix", mvp);

    glDepthMask(GL_FALSE);

    model.setToIdentity();
    model.translate(0, -0.3f, 2.5f); 
    mvp = projection * view * model;
    shaderProgram->setUniformValue("mvpMatrix", mvp);
    shaderProgram->setUniformValue("color", QVector4D(0.2f, 0.7f, 1.0f, 0.4f));
    drawCylinder();

    glDepthMask(GL_TRUE);

    model.setToIdentity();

    if (handSet) {
        model.setToIdentity();

        float angle = atan2(handZ, handX) * 180.0f / M_PI;

        model.translate(handX, handY - 0.3f, handZ + 2.5f);

        model.rotate(angle, 0.0f, 1.0f, 0.0f);

        model.scale(1.2f, 1.2f, 1.2f);

        mvp = projection * view * model;
        shaderProgram->setUniformValue("mvpMatrix", mvp);
        shaderProgram->setUniformValue("modelMatrix", model);
        shaderProgram->setUniformValue("normalMatrix", model.normalMatrix());

        drawSword();

        QMatrix4x4 shadowModel;

        const float groundLevel = -cylinderHeight/2.0f - 0.3f + 0.01f;

        shadowModel.setToIdentity();

        shadowModel.translate(handX, groundLevel, handZ + 2.5f);

        shadowModel.rotate(angle, 0.0f, 1.0f, 0.0f);
        shadowModel.rotate(90.0f, 1.0f, 0.0f, 0.0f); 

        shadowModel.scale(1.3f, 1.3f, 1.3f);

        mvp = projection * view * shadowModel;
        shaderProgram->setUniformValue("mvpMatrix", mvp);
        shaderProgram->setUniformValue("modelMatrix", shadowModel);
        shaderProgram->setUniformValue("normalMatrix", shadowModel.normalMatrix());

        drawSwordShadow();
    }

    glDisable(GL_CULL_FACE);

    const float groundLevel = -cylinderHeight/2.0f - 0.3f;

    glDepthMask(GL_FALSE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for (int i = 0; i < projectiles.size(); i++) {
        if (projectiles[i].isActive()) {
            projectiles[i].renderShadow(shaderProgram, projection, view, groundLevel);
        }
    }

    glDepthMask(GL_TRUE);

    for (int i = 0; i < projectiles.size(); i++) {
        projectiles[i].render(shaderProgram, projection, view);
    }

    glEnable(GL_CULL_FACE);

    if (!pendingProjectiles.isEmpty()) {
        projectiles.append(pendingProjectiles);
        pendingProjectiles.clear();
    }

    shaderProgram->release();
}

void OpenGLWidget::drawCylinder() {
    const int slices = 48;
    const float r = cylinderRadius;
    QVector<QVector3D> vertices;

    const float groundLevel = -cylinderHeight/2.0f - 0.3f;
    const float wallHeight = 4.0f;
    const float roofLevel = groundLevel + wallHeight;

    const float bottom = groundLevel;
    const float top = roofLevel;

    for (int i = 0; i < slices; ++i) {
        float theta = float(i) / slices * 2.0f * M_PI;
        vertices.append(QVector3D(r * std::cos(theta), bottom, r * std::sin(theta)));
    }

    for (int i = 0; i < slices; ++i) {
        float theta = float(i) / slices * 2.0f * M_PI;
        vertices.append(QVector3D(r * std::cos(theta), top, r * std::sin(theta)));
    }

    for (int i = 0; i < slices; i += 4) {
        float theta = float(i) / slices * 2.0f * M_PI;
        float x = r * std::cos(theta);
        float z = r * std::sin(theta);
        vertices.append(QVector3D(x, bottom, z));
        vertices.append(QVector3D(x, top, z));
    }

    vbo.bind();
    vbo.allocate(vertices.constData(), vertices.size() * sizeof(QVector3D));

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glDrawArrays(GL_LINE_LOOP, 0, slices);
    glDrawArrays(GL_LINE_LOOP, slices, slices);

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

    vbo.bind();
    vbo.allocate(vertices.constData(), vertices.size() * sizeof(QVector3D));

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glDrawArrays(GL_QUAD_STRIP, 0, vertices.size());
    glDisableVertexAttribArray(0);

    vbo.release();
}

void OpenGLWidget::timerEvent(QTimerEvent* ) {
    deltaTime = elapsedTimer.elapsed() / 1000.0f;
    elapsedTimer.restart();

    gameTime += deltaTime;

    updateCamera();

    if (isGameRunning && gameTime - lastSpawnTime > spawnInterval) {
        spawnProjectile();
        lastSpawnTime = gameTime;
    }

    updateProjectiles(deltaTime);

    if (isGameRunning) {
        checkCollisions();
    }
    update();
}

void OpenGLWidget::spawnProjectile() {
    Projectile::Type type;
    int typeRandom = QRandomGenerator::global()->bounded(5);

    if (typeRandom == 0) {
        type = Projectile::Type::BANANA;
    } else if (typeRandom == 1) {
        type = Projectile::Type::APPLE;
    } else if (typeRandom == 2) {
        type = Projectile::Type::ANANAS;
    } else if (typeRandom == 3) {
        type = Projectile::Type::FRAISE;
    } else if (typeRandom == 4) {
        type = Projectile::Type::WOOD_CUBE;
    }

    float x = 0.0f; 
    float y = -0.5f; 
    float z = -7.0f;

    float targetAngle = QRandomGenerator::global()->bounded(628) / 100.0f;
    float targetHeight = QRandomGenerator::global()->bounded(-80, 80) / 100.0f * cylinderHeight/2;

    float targetX = cylinderRadius * std::cos(targetAngle);
    float targetY = targetHeight - 0.5f;
    float targetZ = 2.5f + cylinderRadius * std::sin(targetAngle);

    float dx = targetX - x;
    float dz = targetZ - z;

    float time = QRandomGenerator::global()->bounded(70, 100) / 100.0f;

    float peakHeight = QRandomGenerator::global()->bounded(170, 270) / 100.0f;

    float vx = dx / time;
    float vy = (2.0f * peakHeight / time) + ((targetY - y) / time);
    float vz = dz / time;

    vx += QRandomGenerator::global()->bounded(-10, 10) / 100.0f;
    vz += QRandomGenerator::global()->bounded(-10, 10) / 100.0f;

    Projectile p(type, QVector3D(x, y, z), QVector3D(vx, vy, vz));
    p.initializeGL();
    projectiles.append(p);
}

void OpenGLWidget::drawSpawningZone() {
    QVector<QVector3D> vertices = {
        QVector3D(-2.0f, -1.5f, -5.0f),
        QVector3D(2.0f, -1.5f, -5.0f),
        QVector3D(2.0f, 0.5f, -5.0f),
        QVector3D(-2.0f, 0.5f, -5.0f)
    };

    if (vertices.isEmpty()) {
        qWarning("drawSpawningZone: No vertices to draw.");
        return;
    }

    zoneVBO.bind();
    zoneVBO.allocate(vertices.constData(), vertices.size() * sizeof(QVector3D));

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glDrawArrays(GL_LINE_LOOP, 0, vertices.size());
    glDisableVertexAttribArray(0);

    zoneVBO.release();
}

void OpenGLWidget::drawSword() {

    const float bladeLength = 0.25f;    
    const float bladeWidth = 0.06f;     
    const float handleLength = 0.15f;   
    const float handleWidth = 0.03f;    
    const float guardWidth = 0.1f;      
    const float guardHeight = 0.02f;    
    const float thickness = 0.03f;      

    QVector<GLfloat> vertices;

    vertices << -bladeWidth/2 << 0 << thickness/2 << 0 << 0 << 1 << 0.0f << 0.0f;  
    vertices << bladeWidth/2 << 0 << thickness/2 << 0 << 0 << 1 << 1.0f << 0.0f;   
    vertices << 0 << bladeLength << thickness/2 << 0 << 0 << 1 << 0.5f << 1.0f;    

    vertices << bladeWidth/2 << 0 << -thickness/2 << 0 << 0 << -1 << 0.0f << 0.0f;  
    vertices << -bladeWidth/2 << 0 << -thickness/2 << 0 << 0 << -1 << 1.0f << 0.0f; 
    vertices << 0 << bladeLength << -thickness/2 << 0 << 0 << -1 << 0.5f << 1.0f;   

    vertices << -bladeWidth/2 << 0 << thickness/2 << -1 << 0.5f << 0 << 0.0f << 0.0f;   
    vertices << 0 << bladeLength << thickness/2 << -1 << 0.5f << 0 << 1.0f << 1.0f;     
    vertices << 0 << bladeLength << -thickness/2 << -1 << 0.5f << 0 << 0.0f << 1.0f;    

    vertices << 0 << bladeLength << -thickness/2 << -1 << 0.5f << 0 << 0.0f << 1.0f;    
    vertices << -bladeWidth/2 << 0 << -thickness/2 << -1 << 0.5f << 0 << 1.0f << 0.0f;  
    vertices << -bladeWidth/2 << 0 << thickness/2 << -1 << 0.5f << 0 << 0.0f << 0.0f;   

    vertices << bladeWidth/2 << 0 << thickness/2 << 1 << 0.5f << 0 << 0.0f << 0.0f;     
    vertices << 0 << bladeLength << -thickness/2 << 1 << 0.5f << 0 << 1.0f << 1.0f;     
    vertices << 0 << bladeLength << thickness/2 << 1 << 0.5f << 0 << 0.0f << 1.0f;      

    vertices << 0 << bladeLength << -thickness/2 << 1 << 0.5f << 0 << 1.0f << 1.0f;     
    vertices << bladeWidth/2 << 0 << thickness/2 << 1 << 0.5f << 0 << 0.0f << 0.0f;     
    vertices << bladeWidth/2 << 0 << -thickness/2 << 1 << 0.5f << 0 << 1.0f << 0.0f;    

    vertices << -bladeWidth/2 << 0 << thickness/2 << 0 << -1 << 0 << 0.0f << 0.0f;     
    vertices << -bladeWidth/2 << 0 << -thickness/2 << 0 << -1 << 0 << 0.0f << 1.0f;    
    vertices << bladeWidth/2 << 0 << -thickness/2 << 0 << -1 << 0 << 1.0f << 1.0f;     

    vertices << bladeWidth/2 << 0 << -thickness/2 << 0 << -1 << 0 << 1.0f << 1.0f;     
    vertices << bladeWidth/2 << 0 << thickness/2 << 0 << -1 << 0 << 1.0f << 0.0f;      
    vertices << -bladeWidth/2 << 0 << thickness/2 << 0 << -1 << 0 << 0.0f << 0.0f;     

    vertices << -guardWidth/2 << -guardHeight << thickness/2 << 0 << 0 << 1 << 0.0f << 1.0f;   
    vertices << guardWidth/2 << -guardHeight << thickness/2 << 0 << 0 << 1 << 1.0f << 1.0f;     
    vertices << guardWidth/2 << 0 << thickness/2 << 0 << 0 << 1 << 1.0f << 0.0f;              

    vertices << guardWidth/2 << 0 << thickness/2 << 0 << 0 << 1 << 1.0f << 0.0f;              
    vertices << -guardWidth/2 << 0 << thickness/2 << 0 << 0 << 1 << 0.0f << 0.0f;             
    vertices << -guardWidth/2 << -guardHeight << thickness/2 << 0 << 0 << 1 << 0.0f << 1.0f;   

    vertices << -guardWidth/2 << -guardHeight << -thickness/2 << 0 << 0 << -1 << 1.0f << 1.0f;  
    vertices << guardWidth/2 << 0 << -thickness/2 << 0 << 0 << -1 << 0.0f << 0.0f;             
    vertices << guardWidth/2 << -guardHeight << -thickness/2 << 0 << 0 << -1 << 0.0f << 1.0f;   

    vertices << guardWidth/2 << 0 << -thickness/2 << 0 << 0 << -1 << 0.0f << 0.0f;             
    vertices << -guardWidth/2 << -guardHeight << -thickness/2 << 0 << 0 << -1 << 1.0f << 1.0f;  
    vertices << -guardWidth/2 << 0 << -thickness/2 << 0 << 0 << -1 << 1.0f << 0.0f;            

    vertices << -guardWidth/2 << -guardHeight << thickness/2 << 0 << -1 << 0 << 0.0f << 0.0f;   
    vertices << guardWidth/2 << -guardHeight << thickness/2 << 0 << -1 << 0 << 1.0f << 0.0f;    
    vertices << guardWidth/2 << -guardHeight << -thickness/2 << 0 << -1 << 0 << 1.0f << 1.0f;   

    vertices << guardWidth/2 << -guardHeight << -thickness/2 << 0 << -1 << 0 << 1.0f << 1.0f;   
    vertices << -guardWidth/2 << -guardHeight << -thickness/2 << 0 << -1 << 0 << 0.0f << 1.0f;  
    vertices << -guardWidth/2 << -guardHeight << thickness/2 << 0 << -1 << 0 << 0.0f << 0.0f;   

    vertices << -guardWidth/2 << -guardHeight << thickness/2 << -1 << 0 << 0 << 1.0f << 1.0f;   
    vertices << -guardWidth/2 << -guardHeight << -thickness/2 << -1 << 0 << 0 << 0.0f << 1.0f;  
    vertices << -guardWidth/2 << 0 << -thickness/2 << -1 << 0 << 0 << 0.0f << 0.0f;            

    vertices << -guardWidth/2 << 0 << -thickness/2 << -1 << 0 << 0 << 0.0f << 0.0f;            
    vertices << -guardWidth/2 << 0 << thickness/2 << -1 << 0 << 0 << 1.0f << 0.0f;             
    vertices << -guardWidth/2 << -guardHeight << thickness/2 << -1 << 0 << 0 << 1.0f << 1.0f;   

    vertices << guardWidth/2 << -guardHeight << thickness/2 << 1 << 0 << 0 << 0.0f << 1.0f;     
    vertices << guardWidth/2 << 0 << -thickness/2 << 1 << 0 << 0 << 1.0f << 0.0f;              
    vertices << guardWidth/2 << -guardHeight << -thickness/2 << 1 << 0 << 0 << 1.0f << 1.0f;    

    vertices << guardWidth/2 << 0 << -thickness/2 << 1 << 0 << 0 << 1.0f << 0.0f;              
    vertices << guardWidth/2 << -guardHeight << thickness/2 << 1 << 0 << 0 << 0.0f << 1.0f;     
    vertices << guardWidth/2 << 0 << thickness/2 << 1 << 0 << 0 << 0.0f << 0.0f;              

    vertices << -handleWidth/2 << -handleLength-guardHeight << thickness/2 << 0 << 0 << 1 << 0.0f << 1.0f;  
    vertices << handleWidth/2 << -handleLength-guardHeight << thickness/2 << 0 << 0 << 1 << 1.0f << 1.0f;   
    vertices << handleWidth/2 << -guardHeight << thickness/2 << 0 << 0 << 1 << 1.0f << 0.0f;               

    vertices << handleWidth/2 << -guardHeight << thickness/2 << 0 << 0 << 1 << 1.0f << 0.0f;               
    vertices << -handleWidth/2 << -guardHeight << thickness/2 << 0 << 0 << 1 << 0.0f << 0.0f;              
    vertices << -handleWidth/2 << -handleLength-guardHeight << thickness/2 << 0 << 0 << 1 << 0.0f << 1.0f; 

    vertices << -handleWidth/2 << -handleLength-guardHeight << -thickness/2 << 0 << 0 << -1 << 0.0f << 1.0f; 
    vertices << handleWidth/2 << -guardHeight << -thickness/2 << 0 << 0 << -1 << 1.0f << 0.0f;               
    vertices << handleWidth/2 << -handleLength-guardHeight << -thickness/2 << 0 << 0 << -1 << 1.0f << 1.0f;  

    vertices << handleWidth/2 << -guardHeight << -thickness/2 << 0 << 0 << -1 << 1.0f << 0.0f;               
    vertices << -handleWidth/2 << -handleLength-guardHeight << -thickness/2 << 0 << 0 << -1 << 0.0f << 1.0f; 
    vertices << -handleWidth/2 << -guardHeight << -thickness/2 << 0 << 0 << -1 << 0.0f << 0.0f;              

    vertices << -handleWidth/2 << -handleLength-guardHeight << thickness/2 << 0 << -1 << 0 << 0.0f << 0.0f;  
    vertices << handleWidth/2 << -handleLength-guardHeight << thickness/2 << 0 << -1 << 0 << 1.0f << 0.0f;   
    vertices << handleWidth/2 << -handleLength-guardHeight << -thickness/2 << 0 << -1 << 0 << 1.0f << 1.0f;  

    vertices << handleWidth/2 << -handleLength-guardHeight << -thickness/2 << 0 << -1 << 0 << 1.0f << 1.0f;  
    vertices << -handleWidth/2 << -handleLength-guardHeight << -thickness/2 << 0 << -1 << 0 << 0.0f << 1.0f; 
    vertices << -handleWidth/2 << -handleLength-guardHeight << thickness/2 << 0 << -1 << 0 << 0.0f << 0.0f;  

    vertices << -handleWidth/2 << -handleLength-guardHeight << thickness/2 << -1 << 0 << 0 << 0.0f << 1.0f;  
    vertices << -handleWidth/2 << -handleLength-guardHeight << -thickness/2 << -1 << 0 << 0 << 1.0f << 1.0f; 
    vertices << -handleWidth/2 << -guardHeight << -thickness/2 << -1 << 0 << 0 << 1.0f << 0.0f;              

    vertices << -handleWidth/2 << -guardHeight << -thickness/2 << -1 << 0 << 0 << 1.0f << 0.0f;              
    vertices << -handleWidth/2 << -guardHeight << thickness/2 << -1 << 0 << 0 << 0.0f << 0.0f;               
    vertices << -handleWidth/2 << -handleLength-guardHeight << thickness/2 << -1 << 0 << 0 << 0.0f << 1.0f;  

    vertices << handleWidth/2 << -handleLength-guardHeight << thickness/2 << 1 << 0 << 0 << 1.0f << 1.0f;   
    vertices << handleWidth/2 << -guardHeight << -thickness/2 << 1 << 0 << 0 << 0.0f << 0.0f;               
    vertices << handleWidth/2 << -handleLength-guardHeight << -thickness/2 << 1 << 0 << 0 << 0.0f << 1.0f;  

    vertices << handleWidth/2 << -guardHeight << -thickness/2 << 1 << 0 << 0 << 0.0f << 0.0f;               
    vertices << handleWidth/2 << -handleLength-guardHeight << thickness/2 << 1 << 0 << 0 << 1.0f << 1.0f;   
    vertices << handleWidth/2 << -guardHeight << thickness/2 << 1 << 0 << 0 << 1.0f << 0.0f;                

    vbo.bind();
    vbo.allocate(vertices.constData(), vertices.size() * sizeof(GLfloat));

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), nullptr);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), reinterpret_cast<void*>(3 * sizeof(GLfloat)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), reinterpret_cast<void*>(6 * sizeof(GLfloat)));

    const int bladeVertexCount = 24; 
    const int guardVertexCount = 30; 
    const int handleVertexCount = 30; 

    if (bladeTexture) {
        shaderProgram->setUniformValue("useTexture", true);
        bladeTexture->bind(0);
        shaderProgram->setUniformValue("appleTexture", 0); 
    } else {
        shaderProgram->setUniformValue("useTexture", false);
        shaderProgram->setUniformValue("color", QVector4D(0.8f, 0.8f, 0.9f, 1.0f)); 
    }
    glDrawArrays(GL_TRIANGLES, 0, bladeVertexCount);

    shaderProgram->setUniformValue("useTexture", false);
    shaderProgram->setUniformValue("color", QVector4D(0.9f, 0.8f, 0.2f, 1.0f));  
    glDrawArrays(GL_TRIANGLES, bladeVertexCount, guardVertexCount);

    if (handleTexture) {
        shaderProgram->setUniformValue("useTexture", true);
        handleTexture->bind(0);
        shaderProgram->setUniformValue("appleTexture", 0); 
    } else {
        shaderProgram->setUniformValue("useTexture", false);
        shaderProgram->setUniformValue("color", QVector4D(0.6f, 0.3f, 0.1f, 1.0f)); 
    }
    glDrawArrays(GL_TRIANGLES, bladeVertexCount + guardVertexCount, handleVertexCount);

    if (bladeTexture || handleTexture) {
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    glDisableVertexAttribArray(2);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(0);
    vbo.release();
}

void OpenGLWidget::drawSwordShadow() {

    const float bladeLength = 0.25f;
    const float bladeWidth = 0.06f;
    const float handleLength = 0.15f;
    const float handleWidth = 0.03f;
    const float guardWidth = 0.1f;
    const float guardHeight = 0.02f;

    QVector<GLfloat> vertices;

    const float nx = 0.0f, ny = 1.0f, nz = 0.0f;

    const float tu = 0.0f, tv = 0.0f;

    vertices << -bladeWidth/2 << 0 << 0 << nx << ny << nz << tu << tv;  
    vertices << bladeWidth/2 << 0 << 0 << nx << ny << nz << tu << tv;   
    vertices << 0 << bladeLength << 0 << nx << ny << nz << tu << tv;    

    vertices << -guardWidth/2 << -guardHeight << 0 << nx << ny << nz << tu << tv;  
    vertices << guardWidth/2 << -guardHeight << 0 << nx << ny << nz << tu << tv;   
    vertices << guardWidth/2 << 0 << 0 << nx << ny << nz << tu << tv;             

    vertices << guardWidth/2 << 0 << 0 << nx << ny << nz << tu << tv;             
    vertices << -guardWidth/2 << 0 << 0 << nx << ny << nz << tu << tv;            
    vertices << -guardWidth/2 << -guardHeight << 0 << nx << ny << nz << tu << tv; 

    float handleY = -guardHeight;
    vertices << -handleWidth/2 << handleY << 0 << nx << ny << nz << tu << tv;                  
    vertices << handleWidth/2 << handleY << 0 << nx << ny << nz << tu << tv;                   
    vertices << handleWidth/2 << handleY - handleLength << 0 << nx << ny << nz << tu << tv;    

    vertices << handleWidth/2 << handleY - handleLength << 0 << nx << ny << nz << tu << tv;    
    vertices << -handleWidth/2 << handleY - handleLength << 0 << nx << ny << nz << tu << tv;   
    vertices << -handleWidth/2 << handleY << 0 << nx << ny << nz << tu << tv;                  

    vbo.bind();
    vbo.allocate(vertices.constData(), vertices.size() * sizeof(GLfloat));

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), nullptr);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), reinterpret_cast<void*>(3 * sizeof(GLfloat)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), reinterpret_cast<void*>(6 * sizeof(GLfloat)));

    shaderProgram->setUniformValue("useTexture", false);
    shaderProgram->setUniformValue("color", QVector4D(0.0f, 0.0f, 0.0f, 0.5f)); 

    glDrawArrays(GL_TRIANGLES, 0, vertices.size() / 8); 

    glDisableVertexAttribArray(2);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(0);
    vbo.release();
}

void OpenGLWidget::keyPressEvent(QKeyEvent* event) {
    qDebug() << "Key pressed:" << event->key();

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

        resetCamera();
        break;
    default:
        QOpenGLWidget::keyPressEvent(event);
    }

    update();
}

void OpenGLWidget::keyReleaseEvent(QKeyEvent* event) {
    qDebug() << "Key released:" << event->key();

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

    update();
}

void OpenGLWidget::resetGame() {

    projectiles.clear();
    pendingProjectiles.clear();

    gameTime = 0.0f;
    lastSpawnTime = 0.0f;
    score = 0;

    isGameRunning = true;

    gameOverEffect = false;
    gameOverEffectTime = 0.0f;

    update();
}

void OpenGLWidget::updateProjectiles(float deltaTime) {

    if (!isGameRunning) return;

    for (int i = 0; i < projectiles.size(); i++) {

        projectiles[i].applyGravity(deltaTime * 0.8f); 

        float reducedDelta = deltaTime * 0.9f; 
        projectiles[i].update(reducedDelta);

        QVector3D pos = projectiles[i].position();
        if (!projectiles[i].isFragment() && pos.z() > 5.0f && pos.z() < 7.0f) {

            isGameRunning = false;
            gameOverEffect = true;
            projectiles[i].markForGameOver();
            emit gameOver();
            return;
        }

        if (!projectiles[i].isActive()) {
            projectiles.removeAt(i);
            i--;
        }
    }
}

void OpenGLWidget::checkCollisions() {
    if (!handSet) return;

    QVector3D swordPosition = handPosition + QVector3D(0, -0.5f, 2.5f); 

    const float bladeRadius = 0.05f; 
    const float bladeHeight = 0.3f;  

    for (int i = 0; i < projectiles.size(); i++) {

        if (projectiles[i].checkCollisionWithCylinder(bladeRadius, bladeHeight, swordPosition)) {

            bool isOriginal = !projectiles[i].isFragment();

            std::vector<Projectile> fragments = projectiles[i].slice();

            projectiles.removeAt(i);

            for (const auto& fragment : fragments) {
                pendingProjectiles.append(fragment);
            }

            if (isOriginal) {
                emit scoreIncreased();
            }

            i--;
        }
    }
}

void OpenGLWidget::updateCamera() {

    if (keysPressed[0]) { 
        cameraPitch += cameraRotationSpeed * deltaTime;
        qDebug() << "Moving camera up, pitch:" << cameraPitch;
    }
    if (keysPressed[1]) { 
        cameraPitch -= cameraRotationSpeed * deltaTime;
        qDebug() << "Moving camera down, pitch:" << cameraPitch;
    }
    if (keysPressed[2]) { 
        cameraYaw -= cameraRotationSpeed * deltaTime;
        qDebug() << "Moving camera left, yaw:" << cameraYaw;
    }
    if (keysPressed[3]) { 
        cameraYaw += cameraRotationSpeed * deltaTime;
        qDebug() << "Moving camera right, yaw:" << cameraYaw;
    }

    if (cameraPitch > 89.0f) cameraPitch = 89.0f;
    if (cameraPitch < -89.0f) cameraPitch = -89.0f;

    float pitch_rad = qDegreesToRadians(cameraPitch);
    float yaw_rad = qDegreesToRadians(cameraYaw);

    cameraPosition.setX(cameraDistance * qCos(pitch_rad) * qSin(yaw_rad));
    cameraPosition.setY(cameraDistance * qSin(pitch_rad));
    cameraPosition.setZ(cameraDistance * qCos(pitch_rad) * qCos(yaw_rad));

    view.setToIdentity();
    view.lookAt(cameraPosition, QVector3D(0, 0, 0), QVector3D(0, 1, 0));
}

void OpenGLWidget::drawGround() {

    const float groundWidth = 8.0f;         
    const float groundLevel = -cylinderHeight/2.0f - 0.3f; 

    float groundVertices[] = {

        -groundWidth/2.0f, groundLevel, 2.5f + 1.0f,      0.0f, 1.0f, 0.0f,     0.0f, 0.0f,   
        groundWidth/2.0f, groundLevel, 2.5f + 1.0f,       0.0f, 1.0f, 0.0f,     4.0f, 0.0f,   
        groundWidth/2.0f, groundLevel, -5.0f - 3.0f,      0.0f, 1.0f, 0.0f,     4.0f, 4.0f,   

        groundWidth/2.0f, groundLevel, -5.0f - 3.0f,      0.0f, 1.0f, 0.0f,     4.0f, 4.0f,   
        -groundWidth/2.0f, groundLevel, -5.0f - 3.0f,     0.0f, 1.0f, 0.0f,     0.0f, 4.0f,   
        -groundWidth/2.0f, groundLevel, 2.5f + 1.0f,      0.0f, 1.0f, 0.0f,     0.0f, 0.0f    
    };

    QVector<QVector3D> lineVertices;

    const float gridSpacing = 1.0f;
    const float lineWidth = 0.01f;

    for (float x = -groundWidth/2.0f; x <= groundWidth/2.0f; x += gridSpacing) {
        lineVertices.append(QVector3D(x, groundLevel + lineWidth, 2.5f + 1.0f));         
        lineVertices.append(QVector3D(x, groundLevel + lineWidth, -5.0f - 3.0f));        
    }

    for (float z = 2.5f + 1.0f; z >= -5.0f - 3.0f; z -= gridSpacing) {
        lineVertices.append(QVector3D(-groundWidth/2.0f, groundLevel + lineWidth, z));   
        lineVertices.append(QVector3D(groundWidth/2.0f, groundLevel + lineWidth, z));    
    }

    shaderProgram->bind();

    if (groundTexture) {

        QMatrix4x4 model;
        model.setToIdentity();

        shaderProgram->setUniformValue("modelMatrix", model);
        shaderProgram->setUniformValue("normalMatrix", model.normalMatrix());
        shaderProgram->setUniformValue("mvpMatrix", projection * view * model);

        shaderProgram->setUniformValue("useLighting", true);
        shaderProgram->setUniformValue("ambientStrength", 0.4f);    
        shaderProgram->setUniformValue("specularStrength", 0.1f);   
        shaderProgram->setUniformValue("shininess", 8.0f);          

        vbo.bind();
        vbo.allocate(groundVertices, sizeof(groundVertices));

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), nullptr);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void*>(6 * sizeof(float)));

        shaderProgram->setUniformValue("useTexture", true);
        groundTexture->bind(0);
        shaderProgram->setUniformValue("appleTexture", 0);  

        glDrawArrays(GL_TRIANGLES, 0, 6);

        groundTexture->release();
        shaderProgram->setUniformValue("useTexture", false);
        glDisableVertexAttribArray(2);
    } else {

        vbo.bind();
        vbo.allocate(groundVertices, sizeof(groundVertices));

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);

        shaderProgram->setUniformValue("useTexture", false);
        shaderProgram->setUniformValue("color", QVector4D(0.2f, 0.2f, 0.2f, 0.9f));
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    vbo.allocate(lineVertices.constData(), lineVertices.size() * sizeof(QVector3D));
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    shaderProgram->setUniformValue("useTexture", false);
    shaderProgram->setUniformValue("color", QVector4D(0.4f, 0.4f, 0.4f, 1.0f));
    glDrawArrays(GL_LINES, 0, lineVertices.size());

    glDisableVertexAttribArray(0);
    vbo.release();
}

void OpenGLWidget::drawWalls() {

    const float groundWidth = 8.0f;
    const float wallHeight = 4.0f;
    const float groundLevel = -cylinderHeight / 2.0f - 0.3f;

    float wallVertices[] = {

        -groundWidth / 2.0f, groundLevel, -5.0f - 3.0f,        0.0f, 0.0f, 1.0f,        0.0f, 0.0f,  
         groundWidth / 2.0f, groundLevel, -5.0f - 3.0f,        0.0f, 0.0f, 1.0f,        1.0f, 0.0f,  
         groundWidth / 2.0f, groundLevel + wallHeight, -5.0f - 3.0f,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f,  

         groundWidth / 2.0f, groundLevel + wallHeight, -5.0f - 3.0f,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f,  
        -groundWidth / 2.0f, groundLevel + wallHeight, -5.0f - 3.0f,  0.0f, 0.0f, 1.0f,  0.0f, 1.0f,  
        -groundWidth / 2.0f, groundLevel, -5.0f - 3.0f,        0.0f, 0.0f, 1.0f,        0.0f, 0.0f,  

        -groundWidth / 2.0f, groundLevel, 2.5f + 1.0f,         1.0f, 0.0f, 0.0f,        0.0f, 0.0f,  
        -groundWidth / 2.0f, groundLevel, -5.0f - 3.0f,        1.0f, 0.0f, 0.0f,        1.0f, 0.0f,  
        -groundWidth / 2.0f, groundLevel + wallHeight, -5.0f - 3.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,  

        -groundWidth / 2.0f, groundLevel + wallHeight, -5.0f - 3.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,  
        -groundWidth / 2.0f, groundLevel + wallHeight, 2.5f + 1.0f,    1.0f, 0.0f, 0.0f,    0.0f, 1.0f,  
        -groundWidth / 2.0f, groundLevel, 2.5f + 1.0f,         1.0f, 0.0f, 0.0f,        0.0f, 0.0f,  

         groundWidth / 2.0f, groundLevel, 2.5f + 1.0f,         -1.0f, 0.0f, 0.0f,        0.0f, 0.0f,  
         groundWidth / 2.0f, groundLevel + wallHeight, -5.0f - 3.0f,   -1.0f, 0.0f, 0.0f,   1.0f, 1.0f,  
         groundWidth / 2.0f, groundLevel, -5.0f - 3.0f,        -1.0f, 0.0f, 0.0f,        1.0f, 0.0f,  

         groundWidth / 2.0f, groundLevel + wallHeight, -5.0f - 3.0f,   -1.0f, 0.0f, 0.0f,   1.0f, 1.0f,  
         groundWidth / 2.0f, groundLevel, 2.5f + 1.0f,         -1.0f, 0.0f, 0.0f,        0.0f, 0.0f,  
         groundWidth / 2.0f, groundLevel + wallHeight, 2.5f + 1.0f,    -1.0f, 0.0f, 0.0f,    0.0f, 1.0f   
    };

    shaderProgram->bind();

    QMatrix4x4 model;
    model.setToIdentity();

    shaderProgram->setUniformValue("modelMatrix", model);
    shaderProgram->setUniformValue("normalMatrix", model.normalMatrix());
    shaderProgram->setUniformValue("mvpMatrix", projection * view * model);

    shaderProgram->setUniformValue("useLighting", true);
    shaderProgram->setUniformValue("ambientStrength", 0.35f);   
    shaderProgram->setUniformValue("specularStrength", 0.2f);   
    shaderProgram->setUniformValue("shininess", 16.0f);         

    if (wallTexture && backWallTexture) {

        vbo.bind();
        vbo.allocate(wallVertices, sizeof(wallVertices));

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), nullptr);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void*>(6 * sizeof(float)));

        shaderProgram->setUniformValue("useTexture", true);

        backWallTexture->bind(0);
        shaderProgram->setUniformValue("appleTexture", 0);
        glDrawArrays(GL_TRIANGLES, 0, 6);  
        backWallTexture->release();

        wallTexture->bind(0);
        shaderProgram->setUniformValue("appleTexture", 0);
        glDrawArrays(GL_TRIANGLES, 6, 12);  
        wallTexture->release();

        shaderProgram->setUniformValue("useTexture", false);
        glDisableVertexAttribArray(2);
    } else {

        vbo.bind();
        vbo.allocate(wallVertices, sizeof(wallVertices));

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);

        shaderProgram->setUniformValue("useTexture", false);

        shaderProgram->setUniformValue("color", QVector4D(0.6f, 0.4f, 0.2f, 1.0f));  
        glDrawArrays(GL_TRIANGLES, 0, 6);  

        shaderProgram->setUniformValue("color", QVector4D(0.5f, 0.5f, 0.5f, 1.0f));  
        glDrawArrays(GL_TRIANGLES, 6, 12); 
    }

    glDisableVertexAttribArray(0);
    vbo.release();
}

void OpenGLWidget::drawRoof() {

    const float groundWidth = 8.0f;         
    const float wallHeight = 4.0f;          
    const float groundLevel = -cylinderHeight/2.0f - 0.3f;  
    const float roofLevel = groundLevel + wallHeight;       

    float roofVertices[] = {

        -groundWidth/2.0f, roofLevel, 2.5f+1.0f,        0.0f, -1.0f, 0.0f,    0.0f, 0.0f,   
        -groundWidth/2.0f, roofLevel, -5.0f-3.0f,       0.0f, -1.0f, 0.0f,    0.0f, 4.0f,   
        groundWidth/2.0f, roofLevel, -5.0f-3.0f,        0.0f, -1.0f, 0.0f,    4.0f, 4.0f,   

        groundWidth/2.0f, roofLevel, -5.0f-3.0f,        0.0f, -1.0f, 0.0f,    4.0f, 4.0f,   
        groundWidth/2.0f, roofLevel, 2.5f+1.0f,         0.0f, -1.0f, 0.0f,    4.0f, 0.0f,   
        -groundWidth/2.0f, roofLevel, 2.5f+1.0f,        0.0f, -1.0f, 0.0f,    0.0f, 0.0f    
    };

    QVector<QVector3D> lineVertices;
    const float gridSpacing = 1.0f;
    const float lineWidth = 0.01f;

    for (float x = -groundWidth/2.0f; x <= groundWidth/2.0f; x += gridSpacing) {
        lineVertices.append(QVector3D(x, roofLevel + lineWidth, 2.5f+1.0f));       
        lineVertices.append(QVector3D(x, roofLevel + lineWidth, -5.0f-3.0f));      
    }

    for (float z = 2.5f+1.0f; z >= -5.0f-3.0f; z -= gridSpacing) {
        lineVertices.append(QVector3D(-groundWidth/2.0f, roofLevel + lineWidth, z)); 
        lineVertices.append(QVector3D(groundWidth/2.0f, roofLevel + lineWidth, z));  
    }

    const float skylightSize = groundWidth/4.0f;

    QVector<QVector3D> skylightVertices;

    skylightVertices.append(QVector3D(-skylightSize, roofLevel+0.05f, -skylightSize));     
    skylightVertices.append(QVector3D(-skylightSize, roofLevel+0.05f, skylightSize-5.0f)); 
    skylightVertices.append(QVector3D(skylightSize, roofLevel+0.05f, skylightSize-5.0f));  

    skylightVertices.append(QVector3D(skylightSize, roofLevel+0.05f, skylightSize-5.0f));  
    skylightVertices.append(QVector3D(skylightSize, roofLevel+0.05f, -skylightSize));      
    skylightVertices.append(QVector3D(-skylightSize, roofLevel+0.05f, -skylightSize));     

    shaderProgram->bind();

    QMatrix4x4 model;
    model.setToIdentity();

    shaderProgram->setUniformValue("modelMatrix", model);
    shaderProgram->setUniformValue("normalMatrix", model.normalMatrix());
    shaderProgram->setUniformValue("mvpMatrix", projection * view * model);

    shaderProgram->setUniformValue("useLighting", true);
    shaderProgram->setUniformValue("ambientStrength", 0.45f);   
    shaderProgram->setUniformValue("specularStrength", 0.15f);  
    shaderProgram->setUniformValue("shininess", 12.0f);         

    if (roofTexture) {

        vbo.bind();
        vbo.allocate(roofVertices, sizeof(roofVertices));

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), nullptr);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void*>(6 * sizeof(float)));

        shaderProgram->setUniformValue("useTexture", true);
        roofTexture->bind(0);
        shaderProgram->setUniformValue("appleTexture", 0);  

        glDrawArrays(GL_TRIANGLES, 0, 6);

        roofTexture->release();
        shaderProgram->setUniformValue("useTexture", false);
        glDisableVertexAttribArray(2);
    } else {

        vbo.bind();
        vbo.allocate(roofVertices, sizeof(roofVertices));

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);

        shaderProgram->setUniformValue("useTexture", false);
        shaderProgram->setUniformValue("color", QVector4D(0.3f, 0.4f, 0.5f, 0.8f));
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    vbo.bind();
    vbo.allocate(lineVertices.constData(), lineVertices.size() * sizeof(QVector3D));
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    shaderProgram->setUniformValue("useTexture", false);
    shaderProgram->setUniformValue("color", QVector4D(0.5f, 0.6f, 0.7f, 0.9f));
    glDrawArrays(GL_LINES, 0, lineVertices.size());

    vbo.bind();
    vbo.allocate(skylightVertices.constData(), skylightVertices.size() * sizeof(QVector3D));
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    shaderProgram->setUniformValue("useTexture", false);
    shaderProgram->setUniformValue("color", QVector4D(0.1f, 0.6f, 0.8f, 0.6f));
    glDrawArrays(GL_TRIANGLES, 0, skylightVertices.size());

    glDisableVertexAttribArray(0);
    vbo.release();
}

void OpenGLWidget::drawLightSource(const QVector3D& position) {

    const float radius = 0.2f;
    const int lats = 16;
    const int longs = 16;
    QVector<GLfloat> vertices;

    for (int i = 0; i <= lats; i++) {
        float lat0 = M_PI * (-0.5 + (float) (i - 1) / lats);
        float z0 = sin(lat0);
        float zr0 = cos(lat0);

        float lat1 = M_PI * (-0.5 + (float) i / lats);
        float z1 = sin(lat1);
        float zr1 = cos(lat1);

        for (int j = 0; j <= longs; j++) {
            float lng = 2 * M_PI * (float) j / longs;
            float x = cos(lng);
            float y = sin(lng);

            float xPos = radius * x * zr0;
            float yPos = radius * y * zr0;
            float zPos = radius * z0;

            float nx = x * zr0;
            float ny = y * zr0;
            float nz = z0;

            float u = 0.0f, v = 0.0f;

            vertices << xPos << yPos << zPos << nx << ny << nz << u << v;

            xPos = radius * x * zr1;
            yPos = radius * y * zr1;
            zPos = radius * z1;

            nx = x * zr1;
            ny = y * zr1;
            nz = z1;

            vertices << xPos << yPos << zPos << nx << ny << nz << u << v;
        }
    }

    QMatrix4x4 model;
    model.setToIdentity();
    model.translate(position);

    shaderProgram->setUniformValue("modelMatrix", model);
    shaderProgram->setUniformValue("mvpMatrix", projection * view * model);
    shaderProgram->setUniformValue("normalMatrix", model.normalMatrix());

    shaderProgram->setUniformValue("useLighting", false);

    shaderProgram->setUniformValue("useTexture", false);
    shaderProgram->setUniformValue("color", QVector4D(1.0f, 1.0f, 0.8f, 1.0f)); 

    vbo.bind();
    vbo.allocate(vertices.constData(), vertices.size() * sizeof(GLfloat));

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), nullptr);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), reinterpret_cast<void*>(3 * sizeof(GLfloat)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), reinterpret_cast<void*>(6 * sizeof(GLfloat)));

    for (int i = 0; i < lats; i++) {
        glDrawArrays(GL_TRIANGLE_STRIP, i * (longs + 1) * 2, (longs + 1) * 2);
    }

    glDisableVertexAttribArray(2);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(0);
    vbo.release();

    shaderProgram->setUniformValue("useLighting", true);
}
