cmake_minimum_required(VERSION 3.28)
project(NBodySolarSystem LANGUAGES CXX)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

include(FetchContent)
set(BUILD_SHARED_LIBS OFF)

# ---- SFML 3 ----
set(SFML_BUILD_AUDIO OFF)
set(SFML_BUILD_NETWORK OFF)
FetchContent_Declare(SFML
  GIT_REPOSITORY https://github.com/SFML/SFML.git
  GIT_TAG 3.0.0
  GIT_SHALLOW ON
  EXCLUDE_FROM_ALL SYSTEM)

# ---- Dear ImGui + ImGui-SFML ----
FetchContent_Declare(imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.91.1
  GIT_SHALLOW ON
  EXCLUDE_FROM_ALL SYSTEM)
FetchContent_Declare(ImGui-SFML
  GIT_REPOSITORY https://github.com/SFML/imgui-sfml.git
  GIT_TAG v3.0
  GIT_SHALLOW ON
  EXCLUDE_FROM_ALL SYSTEM)

# ---- nlohmann/json ----
FetchContent_Declare(json
  URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
  DOWNLOAD_EXTRACT_TIMESTAMP ON)

FetchContent_MakeAvailable(SFML)
FetchContent_MakeAvailable(json)
FetchContent_MakeAvailable(imgui)
set(IMGUI_DIR ${imgui_SOURCE_DIR})
set(IMGUI_SFML_FIND_SFML OFF)
FetchContent_MakeAvailable(ImGui-SFML)

add_executable(nbody
  src/main.cpp
  src/CelestialBody.cpp
  src/Simulation.cpp
  src/DataIO.cpp
  src/Plots.cpp
        src/Localization.cpp
        src/Localization.h)

target_include_directories(nbody PRIVATE src)
target_link_libraries(nbody PRIVATE
  SFML::Graphics SFML::Window SFML::System
  ImGui-SFML::ImGui-SFML
  nlohmann_json::nlohmann_json)


# Her build'de, hedef yeniden derlenmese bile assets dizinini calisma dizinine kopyala.
add_custom_target(copy_assets ALL
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:nbody>/assets
  DEPENDS nbody
  COMMENT "Copying assets to the runtime directory")
