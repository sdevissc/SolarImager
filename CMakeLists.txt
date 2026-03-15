cmake_minimum_required(VERSION 3.16)
project(SolarImager VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# ── Qt5 ───────────────────────────────────────────────────────────────────────
find_package(Qt5 REQUIRED COMPONENTS
    Core Widgets Gui SerialPort)

# ── Basler Pylon SDK ──────────────────────────────────────────────────────────
set(PYLON_ROOT "/opt/pylon" CACHE PATH "Path to Pylon SDK root")

find_library(PYLON_LIB NAMES pylonbase
    PATHS "${PYLON_ROOT}/lib" NO_DEFAULT_PATH)
if(NOT PYLON_LIB)
    message(FATAL_ERROR "Pylon library not found under ${PYLON_ROOT}/lib")
endif()
file(GLOB PYLON_LIBS "${PYLON_ROOT}/lib/*.so")

# ── ZWO ASI SDK ───────────────────────────────────────────────────────────────
find_library(ZWO_LIB NAMES ASICamera2
    PATHS /usr/local/lib /usr/lib)
if(NOT ZWO_LIB)
    message(WARNING "ZWO ASI library not found – ZWO will run in simulation")
    set(ZWO_LIB "")
endif()

# ── Player One SDK ────────────────────────────────────────────────────────────
find_library(PLAYERONE_LIB NAMES PlayerOneCamera
    PATHS /usr/local/lib /usr/lib)
if(NOT PLAYERONE_LIB)
    message(WARNING "Player One library not found – Player One will run in simulation")
    set(PLAYERONE_LIB "")
endif()

# ── cfitsio ───────────────────────────────────────────────────────────────────
find_library(CFITSIO_LIB NAMES cfitsio
    PATHS /usr/lib/x86_64-linux-gnu /usr/local/lib /usr/lib)
if(NOT CFITSIO_LIB)
    message(FATAL_ERROR "cfitsio not found. Install libcfitsio-dev.")
endif()
message(STATUS "cfitsio: ${CFITSIO_LIB}")

# ── BaslerCamera OBJECT library (Pylon headers private) ───────────────────────
add_library(BaslerCameraLib OBJECT src/BaslerCamera.cpp)
set_target_properties(BaslerCameraLib PROPERTIES AUTOMOC OFF)
target_include_directories(BaslerCameraLib PRIVATE src/ "${PYLON_ROOT}/include")
target_compile_definitions(BaslerCameraLib PRIVATE USE_GIGE_VISION GENICAM_NO_AUTO_IMPLIB)
target_link_libraries(BaslerCameraLib PRIVATE Qt5::Core Qt5::Gui)

# ── ZwoCamera OBJECT library (ASI headers private) ────────────────────────────
add_library(ZwoCameraLib OBJECT src/ZwoCameraInterface.cpp)
set_target_properties(ZwoCameraLib PROPERTIES AUTOMOC OFF)
target_include_directories(ZwoCameraLib PRIVATE src/)
target_link_libraries(ZwoCameraLib PRIVATE Qt5::Core Qt5::Gui)

# ── PlayerOneCamera OBJECT library (PO headers private) ───────────────────────
add_library(PlayerOneCameraLib OBJECT src/PlayerOneCameraInterface.cpp)
set_target_properties(PlayerOneCameraLib PROPERTIES AUTOMOC OFF)
target_include_directories(PlayerOneCameraLib PRIVATE src/)
target_link_libraries(PlayerOneCameraLib PRIVATE Qt5::Core Qt5::Gui)

# ── Main application ──────────────────────────────────────────────────────────
set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/FrameGrabber.cpp
    src/HistogramWidget.cpp
    src/SSMReader.cpp
    src/SeePlot.cpp
    src/PreviewWidget.cpp
    src/SerPlayerDialog.cpp
)

set(HEADERS
    src/MainWindow.h
    src/CameraInterface.h
    src/BaslerCamera.h
    src/ZwoCameraInterface.h
    src/PlayerOneCameraInterface.h
    src/FrameGrabber.h
    src/HistogramWidget.h
    src/SSMReader.h
    src/SeePlot.h
    src/Theme.h
    src/PreviewWidget.h
    src/SerPlayerDialog.h
)

set_source_files_properties(
    src/BaslerCamera.cpp
    src/ZwoCameraInterface.cpp
    src/PlayerOneCameraInterface.cpp
    PROPERTIES SKIP_AUTOMOC ON)

add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
    $<TARGET_OBJECTS:BaslerCameraLib>
    $<TARGET_OBJECTS:ZwoCameraLib>
    $<TARGET_OBJECTS:PlayerOneCameraLib>
)

target_include_directories(${PROJECT_NAME} PRIVATE src/)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt5::Core Qt5::Widgets Qt5::Gui Qt5::SerialPort
    ${PYLON_LIBS}
    ${ZWO_LIB}
    ${PLAYERONE_LIB}
    ${CFITSIO_LIB}
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    INSTALL_RPATH "${PYLON_ROOT}/lib:/usr/local/lib"
    BUILD_RPATH   "${PYLON_ROOT}/lib:/usr/local/lib"
)

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
