cmake_minimum_required(VERSION 3.16)
project(CTProcessorFinal)

set(CMAKE_CXX_STANDARD 17)

# --- 1. OPENCV ---
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

# --- 2. ITK ---
find_package(ITK REQUIRED)
include(${ITK_USE_FILE})

# --- 3. CURL ---
find_package(CURL REQUIRED)
include_directories(${CURL_INCLUDE_DIRS})

# --- 4. EJECUTABLE ---
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

# --- 5. LIBRERÍAS ---
target_link_libraries(ct_processor 
    ${OpenCV_LIBS}
    ${ITK_LIBRARIES}
    ${CURL_LIBRARIES}
)

# --- 6. RPATH ---
set_target_properties(ct_processor PROPERTIES
    BUILD_RPATH "${OpenCV_DIR}/lib:${ITK_DIR}/../../../lib"
    INSTALL_RPATH "${OpenCV_DIR}/lib:${ITK_DIR}/../../../lib"
)
