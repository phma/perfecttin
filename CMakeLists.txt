project(perfecttin)
cmake_minimum_required(VERSION 3.8.0)
# Copyright 2019-2022 Pierre Abbat.
# This file is part of PerfectTIN.
#
# PerfectTIN is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as
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

option(ENABLE_MITOBREVNO "Enable debugging with Mitobrevno if found." FALSE)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
find_package(Plytapus)
find_package(Qt5 COMPONENTS Core Widgets Gui LinguistTools REQUIRED)
find_package(Boost COMPONENTS program_options)
find_package(Mitobrevno)
find_package(Threads)

qt5_add_resources(lib_resources perfecttin.qrc)
qt5_add_translation(qm_files perfecttin_en.ts perfecttin_es.ts)
# To update translations, run "lupdate *.cpp -ts *.ts" in the source directory.

set(common_files adjelev.cpp angle.cpp arc.cpp bezier3d.cpp binio.cpp boundrect.cpp
    carlsontin.cpp cloud.cpp cogo.cpp color.cpp contour.cpp
    csv.cpp dxf.cpp edgeop.cpp fileio.cpp
    landxml.cpp las.cpp ldecimal.cpp leastsquares.cpp lohi.cpp manysum.cpp matrix.cpp
    minquad.cpp neighbor.cpp octagon.cpp ply.cpp point.cpp pointlist.cpp polyline.cpp ps.cpp
    qindex.cpp quaternion.cpp random.cpp relprime.cpp rootfind.cpp segment.cpp spiral.cpp
    stl.cpp threads.cpp tin.cpp tintext.cpp
    triangle.cpp triop.cpp units.cpp xyzfile.cpp)

if (${Boost_FOUND})
add_executable(perfecttin perfecttin.cpp test.cpp ${common_files})
add_executable(dibathy dibathy.cpp ${common_files})
add_executable(vecinos vecinos.cpp ${common_files})
endif (${Boost_FOUND})
add_executable(perfecttin-gui ciaction.cpp cidialog.cpp configdialog.cpp
	       csaction.cpp gui.cpp lissajous.cpp
	       mainwindow.cpp tincanvas.cpp unitbutton.cpp
	       ${common_files} ${lib_resources} ${qm_files})
add_executable(fuzzptin fuzz.cpp ${common_files})
add_executable(testptin testptin.cpp test.cpp ${common_files})
if (${Boost_FOUND})
target_link_libraries(perfecttin ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARIES})
target_compile_definitions(perfecttin PUBLIC _USE_MATH_DEFINES)
target_link_libraries(dibathy ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARIES})
target_compile_definitions(dibathy PUBLIC _USE_MATH_DEFINES)
target_link_libraries(vecinos ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARIES})
target_compile_definitions(vecinos PUBLIC _USE_MATH_DEFINES)
endif (${Boost_FOUND})
target_link_libraries(perfecttin-gui ${CMAKE_THREAD_LIBS_INIT} Qt5::Widgets Qt5::Core)
target_link_libraries(fuzzptin ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(testptin ${CMAKE_THREAD_LIBS_INIT})
target_compile_definitions(fuzzptin PUBLIC _USE_MATH_DEFINES)
target_compile_definitions(testptin PUBLIC _USE_MATH_DEFINES)
target_compile_definitions(perfecttin-gui PUBLIC _USE_MATH_DEFINES)
if (${Plytapus_FOUND})
if (${Boost_FOUND})
target_link_libraries(perfecttin ${Plytapus_LIBRARY})
target_link_libraries(dibathy ${Plytapus_LIBRARY})
target_link_libraries(vecinos ${Plytapus_LIBRARY})
endif (${Boost_FOUND})
target_link_libraries(perfecttin-gui ${Plytapus_LIBRARY})
target_link_libraries(fuzzptin ${Plytapus_LIBRARY})
target_link_libraries(testptin ${Plytapus_LIBRARY})
endif (${Plytapus_FOUND})
if (${Mitobrevno_FOUND})
target_link_libraries(perfecttin ${Mitobrevno_LIBRARY})
target_link_libraries(perfecttin-gui ${Mitobrevno_LIBRARY})
target_link_libraries(fuzzptin ${Mitobrevno_LIBRARY})
target_link_libraries(testptin ${Mitobrevno_LIBRARY})
endif (${Mitobrevno_FOUND})
set_target_properties(perfecttin-gui PROPERTIES WIN32_EXECUTABLE TRUE)

include(CheckIncludeFiles)
check_include_files(time.h HAVE_TIME_H)
check_include_files(sys/time.h HAVE_SYS_TIME_H)
check_include_files(sys/resource.h HAVE_SYS_RESOURCE_H)
check_include_files(windows.h HAVE_WINDOWS_H)

# Define NO_INSTALL when compiling for fuzzing. This avoids the error
# "The install of the perfecttin target requires changing an RPATH", which
# occurs when using the AFL compiler wrapper with the Ninja generator.
# There is no need to install a binary built for fuzzing.
if (NOT DEFINED NO_INSTALL)
if (${Boost_FOUND})
install(TARGETS perfecttin DESTINATION bin)
endif (${Boost_FOUND})
install(TARGETS perfecttin-gui DESTINATION bin)
install(FILES ${qm_files} DESTINATION share/perfecttin)
install(FILES perfecttin.png DESTINATION share/perfecttin)
install(FILES ${PROJECT_BINARY_DIR}/perfecttin.desktop DESTINATION share/applications)
endif ()

if (WIN32)
include(windeployqt)
windeployqt(perfecttin-gui bin)
endif (WIN32)

test_big_endian(BIGENDIAN)
check_type_size("int" INT)
check_type_size("int *" INT_POINTER)

set(PERFECTTIN_MAJOR_VERSION 1)
set(PERFECTTIN_MINOR_VERSION 0)
set(PERFECTTIN_PATCH_VERSION 0~beta)
set(PERFECTTIN_VERSION ${PERFECTTIN_MAJOR_VERSION}.${PERFECTTIN_MINOR_VERSION}.${PERFECTTIN_PATCH_VERSION})
set(COPY_YEAR 2023)

include_directories(${PROJECT_BINARY_DIR})
if (${Boost_FOUND})
include_directories(${Boost_INCLUDE_DIR})
endif (${Boost_FOUND})
if (${Plytapus_FOUND})
include_directories(${Plytapus_INCLUDE_DIR})
endif (${Plytapus_FOUND})
if (${Mitobrevno_FOUND})
include_directories(${Mitobrevno_INCLUDE_DIR})
endif (${Mitobrevno_FOUND})
configure_file (config.h.in config.h)
configure_file (perfecttin.desktop.in perfecttin.desktop)

set(CPACK_PACKAGE_VERSION_MAJOR ${PERFECTTIN_MAJOR_VERSION})
set(CPACK_PACKAGE_VERSION_MINOR ${PERFECTTIN_MINOR_VERSION})
set(CPACK_PACKAGE_VERSION_PATCH ${PERFECTTIN_PATCH_VERSION})
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_SOURCE_DIR}/COPYING)
set(CPACK_SOURCE_IGNORE_FILES /\\\\.git;.*~)
include(CPack)

include(CTest)
add_test(geom testptin area3 in)
add_test(arith testptin relprime manysum checksum)
add_test(random testptin random)
add_test(matrix testptin matrix)
add_test(angle testptin integertrig)
add_test(leastsquares testptin leastsquares adjelev adjblock)
add_test(fileio testptin csvline pnezd ldecimal)
add_test(edgeop testptin flip bend)
add_test(triop testptin split quarter)
add_test(stl testptin stl)
add_test(polyline testptin polyline)
