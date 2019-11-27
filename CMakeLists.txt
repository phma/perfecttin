project(perfecttin)
cmake_minimum_required(VERSION 3.8.0)
# Copyright 2019 Pierre Abbat.
# This file is part of PerfectTIN.
#
# PerfectTIN is free software: you can redistribute it and/or modify
# modify it under the terms of the GNU Lesser General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# PerfectTIN is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License and Lesser General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License
# and Lesser General Public License along with PerfectTIN. If not, see
# <http://www.gnu.org/licenses/>.

include(TestBigEndian)
include(CheckTypeSize)

add_subdirectory(doc)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 17) # appeared in CMake 3.8
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set(SHARE_DIR ${CMAKE_INSTALL_PREFIX}/share/perfecttin)
add_definitions(-DBOOST_ALL_NO_LIB)
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
set(Boost_USE_STATIC_LIBS ON)
else ()
set(Boost_USE_STATIC_LIBS OFF)
endif ()

option (FORCE_COLORED_OUTPUT "Always produce ANSI-colored output (GNU/Clang only)." FALSE)
if (${FORCE_COLORED_OUTPUT})
  if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    add_compile_options (-fdiagnostics-color=always)
  elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    add_compile_options (-fcolor-diagnostics)
  endif ()
endif ()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
find_package(LibPLYXX)
find_package(Qt5 COMPONENTS Core Widgets Gui LinguistTools REQUIRED)
find_package(Boost COMPONENTS program_options)
find_package(Threads)

qt5_add_resources(lib_resources perfecttin.qrc)
qt5_add_translation(qm_files perfecttin_en.ts perfecttin_es.ts)
# To update translations, run "lupdate *.cpp -ts *.ts" in the source directory.

if (${Boost_FOUND})
add_executable(perfecttin adjelev.cpp angle.cpp binio.cpp
	       boundrect.cpp carlsontin.cpp cloud.cpp cogo.cpp
               dxf.cpp edgeop.cpp fileio.cpp landxml.cpp las.cpp ldecimal.cpp
               leastsquares.cpp manysum.cpp matrix.cpp neighbor.cpp octagon.cpp
               perfecttin.cpp ply.cpp point.cpp pointlist.cpp ps.cpp
               qindex.cpp random.cpp relprime.cpp test.cpp
               threads.cpp tin.cpp tintext.cpp triangle.cpp triop.cpp)
endif (${Boost_FOUND})
add_executable(perfecttin-gui adjelev.cpp angle.cpp binio.cpp
	       boundrect.cpp carlsontin.cpp cloud.cpp cogo.cpp configdialog.cpp
               dxf.cpp edgeop.cpp fileio.cpp gui.cpp landxml.cpp las.cpp ldecimal.cpp
               leastsquares.cpp lissajous.cpp mainwindow.cpp
               manysum.cpp matrix.cpp neighbor.cpp octagon.cpp
               ply.cpp point.cpp pointlist.cpp ps.cpp
               qindex.cpp random.cpp relprime.cpp test.cpp
               threads.cpp tin.cpp tincanvas.cpp tintext.cpp triangle.cpp triop.cpp
               unitbutton.cpp ${lib_resources} ${qm_files})
add_executable(fuzzptin adjelev.cpp angle.cpp binio.cpp
	       boundrect.cpp carlsontin.cpp cloud.cpp cogo.cpp dxf.cpp
               edgeop.cpp fileio.cpp fuzz.cpp landxml.cpp las.cpp ldecimal.cpp
               leastsquares.cpp manysum.cpp matrix.cpp neighbor.cpp octagon.cpp
               ply.cpp point.cpp pointlist.cpp ps.cpp qindex.cpp random.cpp
               relprime.cpp threads.cpp tin.cpp tintext.cpp triangle.cpp triop.cpp)
add_executable(testptin adjelev.cpp angle.cpp binio.cpp
	       boundrect.cpp carlsontin.cpp cloud.cpp cogo.cpp dxf.cpp
               edgeop.cpp fileio.cpp landxml.cpp las.cpp ldecimal.cpp
               leastsquares.cpp manysum.cpp matrix.cpp neighbor.cpp octagon.cpp
               ply.cpp point.cpp pointlist.cpp ps.cpp qindex.cpp random.cpp
               relprime.cpp testptin.cpp threads.cpp tin.cpp tintext.cpp triangle.cpp triop.cpp)
if (${Boost_FOUND})
target_link_libraries(perfecttin ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARIES})
target_compile_definitions(perfecttin PUBLIC _USE_MATH_DEFINES)
endif (${Boost_FOUND})
target_link_libraries(perfecttin-gui ${CMAKE_THREAD_LIBS_INIT} Qt5::Widgets Qt5::Core)
target_link_libraries(fuzzptin ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(testptin ${CMAKE_THREAD_LIBS_INIT})
target_compile_definitions(fuzzptin PUBLIC _USE_MATH_DEFINES)
target_compile_definitions(testptin PUBLIC _USE_MATH_DEFINES)
target_compile_definitions(perfecttin-gui PUBLIC _USE_MATH_DEFINES)
if (${LibPLYXX_FOUND})
if (${Boost_FOUND})
target_link_libraries(perfecttin ${LibPLYXX_LIBRARY})
endif (${Boost_FOUND})
target_link_libraries(perfecttin-gui ${LibPLYXX_LIBRARY})
target_link_libraries(fuzzptin ${LibPLYXX_LIBRARY})
target_link_libraries(testptin ${LibPLYXX_LIBRARY})
endif (${LibPLYXX_FOUND})
set_target_properties(perfecttin-gui PROPERTIES WIN32_EXECUTABLE TRUE)

include(CheckIncludeFiles)
check_include_files(time.h HAVE_TIME_H)
check_include_files(sys/time.h HAVE_SYS_TIME_H)
check_include_files(sys/resource.h HAVE_SYS_RESOURCE_H)
check_include_files(windows.h HAVE_WINDOWS_H)

if (${Boost_FOUND})
install(TARGETS perfecttin DESTINATION bin)
endif (${Boost_FOUND})
install(TARGETS perfecttin-gui DESTINATION bin)
install(FILES ${qm_files} DESTINATION share/perfecttin)

if (WIN32)
include(windeployqt)
windeployqt(perfecttin-gui bin)
endif (WIN32)

test_big_endian(BIGENDIAN)
check_type_size("int" INT)
check_type_size("int *" INT_POINTER)

set(PERFECTTIN_MAJOR_VERSION 0)
set(PERFECTTIN_MINOR_VERSION 3)
set(PERFECTTIN_PATCH_VERSION 3+)
set(PERFECTTIN_VERSION ${PERFECTTIN_MAJOR_VERSION}.${PERFECTTIN_MINOR_VERSION}.${PERFECTTIN_PATCH_VERSION})
set(COPY_YEAR 2019)

include_directories(${PROJECT_BINARY_DIR})
if (${Boost_FOUND})
include_directories(${Boost_INCLUDE_DIR})
endif (${Boost_FOUND})
if (${LibPLYXX_FOUND})
include_directories(${LibPLYXX_INCLUDE_DIR})
endif (${LibPLYXX_FOUND})
configure_file (config.h.in config.h)

set(CPACK_PACKAGE_VERSION_MAJOR ${PERFECTTIN_MAJOR_VERSION})
set(CPACK_PACKAGE_VERSION_MINOR ${PERFECTTIN_MINOR_VERSION})
set(CPACK_PACKAGE_VERSION_PATCH ${PERFECTTIN_PATCH_VERSION})
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_SOURCE_DIR}/COPYING)
set(CPACK_SOURCE_IGNORE_FILES /\\\\.git;.*~)
include(CPack)

include(CTest)
add_test(geom testptin area3 in)
add_test(arith testptin relprime manysum)
add_test(random testptin random)
add_test(matrix testptin matrix)
add_test(leastsquares testptin leastsquares)
add_test(fileio testptin csvline pnezd ldecimal)
