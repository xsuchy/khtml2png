project(khtml2png2)

cmake_minimum_required(VERSION 2.4.0)

find_package(Qt3 REQUIRED) # find and setup Qt3 for this project
find_package(KDE3 REQUIRED) # find and setup KDE3 for this project
#find_library(qtlib QT3)

add_definitions(${QT_DEFINITIONS} ${KDE3_DEFINITIONS})

# tell cmake where to search for libraries:
link_directories(${KDE3_LIB_DIR} ${QT_QT_LIBRARY} qt-mt)

# tell cmake where to search for Qt/KDE headers:
include_directories(${KDE3_INCLUDE_DIR} ${QT_INCLUDE_DIR})

# tell cmake to process CMakeLists.txt in that subdirectory
#add_subdirectory(src)
kde3_automoc(khtml2png.cpp)

add_executable(khtml2png2 khtml2png.cpp)

target_link_libraries(khtml2png2 khtml ${QT_QT_LIBRARY} ${KDE3_KDECORE_LIBRARY})

install_targets( /bin khtml2png2)
