cmake_minimum_required(VERSION 3.10)
project(PID_Hoverboard_GUI VERSION 1.0 LANGUAGES CXX)

# Ativa MOC, UIC e RCC automáticos
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Procurar módulos do Qt5
find_package(Qt5 COMPONENTS Core Widgets SerialPort Charts Gamepad REQUIRED)



find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})




# Lista dos sources
set(PROJECT_SRCS
    src/main.cpp
    src/QInfoSlider.cpp
    src/SerialHandler.cpp
    src/WheelWidget.cpp
    include/QInfoSlider.h
    include/SerialHandler.h
    include/WheelWidget.h


    
)

# Cria o executável
add_executable(${PROJECT_NAME} ${PROJECT_SRCS})

# Inclui diretório de headers
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Linka com as libs Qt5
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt5::Core
    Qt5::Widgets
    Qt5::SerialPort
    Qt5::Charts
    Qt5::Gamepad
    ${SDL2_LIBRARIES}
)