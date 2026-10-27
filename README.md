# N-Body Solar System Simulation

A real-time 2D N-Body Solar System simulation developed in modern C++ using SFML, Dear ImGui, and JSON-based configuration. The project utilizes the **Heun Method (2nd-Order Runge-Kutta)** with a fixed-timestep accumulator pattern to ensure physical accuracy and stability independent of display frame rates.

---

## 🌟 Key Features

* **Numerical Integration:** Heun's Method (RK2) for stable gravitational simulation.
* **Fixed Timestep Engine:** Physics loop is completely decoupled from rendering FPS.
* **Real-time Analytics:** Tracks total energy drift, angular momentum conservation, and orbital telemetries.
* **Interactive UI:** Dear ImGui panel featuring real-time property plots (distance, velocity, kinetic energy).
* **Internationalization (i18n):** JSON-driven language swapping system (English / Turkish).
* **Data-Driven Architecture:** Celestial body definitions and properties loaded dynamically via JSON (`assets/planets.txt`).

---

## 🛠️ Prerequisites

To build and run this project, you need:

* **C++20** compatible compiler (GCC, Clang, or MSVC)
* **CMake** (v3.16 or higher)
* **SFML 3.x**
* **Dear ImGui & ImGui-SFML**
* **nlohmann_json**
# N-Body Solar System Simulation

A real-time 2D N-Body Solar System simulation developed in modern C++ using SFML, Dear ImGui, and JSON-based configuration. The project utilizes the **Heun Method (2nd-Order Runge-Kutta)** with a fixed-timestep accumulator pattern to ensure physical accuracy and stability independent of display frame rates.

---

## 🌟 Key Features

* **Numerical Integration:** Heun's Method (RK2) for stable gravitational simulation.
* **Fixed Timestep Engine:** Physics loop is completely decoupled from rendering FPS.
* **Real-time Analytics:** Tracks total energy drift, angular momentum conservation, and orbital telemetries.
* **Interactive UI:** Dear ImGui panel featuring real-time property plots (distance, velocity, kinetic energy).
* **Internationalization (i18n):** JSON-driven language swapping system (English / Turkish).
* **Data-Driven Architecture:** Celestial body definitions and properties loaded dynamically via JSON (`assets/planets.txt`).

---

## 🛠️ Prerequisites

To build and run this project, you need:

* **C++20** compatible compiler (GCC, Clang, or MSVC)
* **CMake** (v3.16 or higher)
* **SFML 3.x**
* **Dear ImGui & ImGui-SFML**
* **nlohmann_json**

---

## 🚀 How to Build and Run

### 1. Clone the Repository
```bash
git clone https://github.com/Efbiay/nbody-solar-system-simulation.git
cd nbody-solar-system-simulation
```


### 2. Build with CMake
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

### 3. Run the Simulation
Windows:
```bash
build\Release\nbody-solar-system-simulation.exe
```
Linux/macOS:
```bash
./build/nbody-solar-system-simulation
```

Note: Ensure that the assets/ directory (containing logos, fonts, and language JSON files) is copied to the working directory alongside the executable


