cmake_minimum_required(VERSION 3.16)
project(CTProcessorFinal)

set(CMAKE_CXX_STANDARD 17)
# Deja estos compiladores si estás segura de que tienes la versión 12 instalada
set(CMAKE_CXX_COMPILER g++-12)
set(CMAKE_C_COMPILER gcc-12)

# --- 1. OPENCV ---
set(OpenCV_DIR "/home/karen/aplicaciones/Librerias/opencv/build")
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

# --- 2. ITK ---
set(ITK_DIR "/home/karen/aplicaciones/Librerias/itk/install/lib/cmake/ITK-5.4")
find_package(ITK REQUIRED)
include(${ITK_USE_FILE})

# --- 3. CURL (Para Flask) ---
find_package(CURL REQUIRED)
include_directories(${CURL_INCLUDE_DIRS})

# --- 4. DEFINIR EJECUTABLE ---
add_executable(ct_processor 
    main.cpp
    Base64.cpp
    FlaskClient.cpp
    Operaciones.cpp
    InterfazIntegrada.cpp
    Pulmones.cpp
    Huesos.cpp
    Corazon.cpp
)

# --- 5. LINKEAR TODO JUNTO (OpenCV + ITK + CURL) ---
target_link_libraries(ct_processor 
    ${OpenCV_LIBS}
    ${ITK_LIBRARIES}
    ${CURL_LIBRARIES}  # <-- CURL agregado aquí correctamente
)

# --- 6. RPATH (Para que encuentre las librerías al ejecutar) ---
set_target_properties(ct_processor PROPERTIES
    BUILD_RPATH "/home/karen/aplicaciones/Librerias/itk/install/lib:/home/karen/aplicaciones/Librerias/opencv/build/lib"
    INSTALL_RPATH "/home/karen/aplicaciones/Librerias/itk/install/lib:/home/karen/aplicaciones/Librerias/opencv/build/lib"
)