# 🍍 Fruit Slicer 3D — Slice Defender

**Fruit Slicer 3D** is an interactive gesture-based game built with Qt, OpenGL, and OpenCV. The player slices flying fruits in a 3D arena using a virtual sword controlled by real-time hand movements captured via webcam.

## 🎮 Features

- **3D Gameplay** with OpenGL rendering: fruits, textures, sword, and a realistic environment.
- **Real-time Hand Tracking** using OpenCV and webcam input.
- **Interactive Controls**: move the camera using arrow keys and control the sword by moving your hand.
- **Fruit Slicing Physics**: fruits split into two animated fragments with visible cut planes and realistic gravity.
- **Dynamic Shadows and Textures** for immersive visual effects.
- **Score System & Timer**: game lasts 120 seconds, and your score increases with each successful slice.

## 🧠 Architecture

The application is composed of the following core classes:

| Class           | Role                                                                           |
| --------------- | ------------------------------------------------------------------------------ |
| `MainWindow`    | Main GUI window handling score display, camera feed, and game start/stop.      |
| `OpenGLWidget`  | Core 3D rendering engine (arena, sword, fruits) and game logic.                |
| `WebcamHandler` | Captures webcam frames and detects hand regions using Haar Cascade.            |
| `PalmTracker`   | Tracks the palm's position using ORB+FLANN for precise control.                |
| `Projectile`    | Models the fruits and their physical behaviors (movement, slicing, rendering). |
| `PalmDetection` | Handles initial palm detection before tracking.                                |

## 🔧 Requirements

- **Qt 5+**
- **OpenGL**
- **OpenCV 4+**

## 🚀 How to Build

1. Clone the repository.
2. Open the project in Qt Creator or your preferred C++ IDE.
3. Make sure OpenCV and OpenGL development packages are installed.
4. Build and run the project.

## 📸 Controls & Gameplay

- Move your hand in front of the webcam — your virtual sword will follow.
- Slice fruits by intersecting them with your virtual blade.
- Arrow keys allow rotating the camera around the scene.
- Avoid slicing wood cubes – slicing one will end the game immediately!

## ✅ Completed Features

- Fully functional 3D scene rendering with textures and shadows.
- Real-time webcam integration and hand detection.
- Smooth slicing animations and realistic fruit fragment physics.
- In-game HUD for timer and score.

## 🧪 Known Limitations

- Hand tracking may be less accurate under low lighting or fast movements.
- Haar cascade detection can occasionally misfire — deep learning models may improve this.

## ✨ Future Improvements

- Add sound effects and ambient music.
- Introduce difficulty levels and bonus items.
- Replace palm detection with a neural network for robustness.
- Polish UI elements for better UX.

## 👩‍💻 Authors

- **Aymane ASSERRAR**
- **Marieme BENZHA**

_Academic Project – Télécom Saint-Étienne | 2024–2025_
