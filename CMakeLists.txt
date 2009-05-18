project(khtml2png)

cmake_minimum_required(VERSION 2.6)

find_package(KDE4 REQUIRED)

include_directories( ${KDE4_INCLUDES} )

set(mySources khtml2png.cpp)

kde4_add_executable(khtml2png ${mySources})

target_link_libraries(khtml2png ${KDE4_KDEUI_LIBS} ${KDE4_KPARTS_LIBS} ${KDE4_KIO_LIBS} /usr/lib/kde4/libkhtmlpart.so)

install(TARGETS khtml2png DESTINATION bin)
