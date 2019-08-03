project(perfecttin)
cmake_minimum_required(VERSION 3.1.0)
# Copyright 2019 Pierre Abbat.
# This file is part of PerfectTIN.
# 
# PerfectTIN is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# PerfectTIN is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with PerfectTIN. If not, see <http://www.gnu.org/licenses/>.

include(TestBigEndian)
include(CheckTypeSize)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set(SHARE_DIR ${CMAKE_INSTALL_PREFIX}/share/perfecttin)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
find_package(LibPLYXX)
find_package(Qt5 COMPONENTS Core Widgets Gui LinguistTools REQUIRED)
find_package(Boost REQUIRED COMPONENTS program_options locale chrono thread)
add_executable(perfecttin adjelev.cpp angle.cpp binio.cpp
	       boundrect.cpp cloud.cpp cogo.cpp
               dxf.cpp edgeop.cpp fileio.cpp las.cpp ldecimal.cpp
               leastsquares.cpp manysum.cpp matrix.cpp neighbor.cpp octagon.cpp
               perfecttin.cpp ply.cpp point.cpp pointlist.cpp ps.cpp
               qindex.cpp random.cpp relprime.cpp test.cpp
               threads.cpp tin.cpp tintext.cpp triangle.cpp triop.cpp)
add_executable(perfecttin-gui adjelev.cpp angle.cpp binio.cpp
	       boundrect.cpp cloud.cpp cogo.cpp configdialog.cpp
               dxf.cpp edgeop.cpp fileio.cpp gui.cpp las.cpp ldecimal.cpp
               leastsquares.cpp lissajous.cpp mainwindow.cpp
               manysum.cpp matrix.cpp neighbor.cpp octagon.cpp
               ply.cpp point.cpp pointlist.cpp ps.cpp
               qindex.cpp random.cpp relprime.cpp test.cpp
               threads.cpp tin.cpp tincanvas.cpp tintext.cpp triangle.cpp triop.cpp)
target_link_libraries(perfecttin ${Boost_LIBRARIES})
target_link_libraries(perfecttin-gui ${Boost_LIBRARIES} Qt5::Widgets Qt5::Core)
if (${LibPLYXX_FOUND})
target_link_libraries(perfecttin ${LibPLYXX_LIBRARY})
target_link_libraries(perfecttin-gui ${LibPLYXX_LIBRARY})
endif (${LibPLYXX_FOUND})

include(CheckIncludeFiles)
check_include_files(time.h HAVE_TIME_H)
check_include_files(sys/time.h HAVE_SYS_TIME_H)
check_include_files(sys/resource.h HAVE_SYS_RESOURCE_H)
check_include_files(windows.h HAVE_WINDOWS_H)

install(TARGETS perfecttin DESTINATION bin)

test_big_endian(BIGENDIAN)
check_type_size("int" INT)
check_type_size("int *" INT_POINTER)

set(PERFECTTIN_MAJOR_VERSION 0)
set(PERFECTTIN_MINOR_VERSION 2)
set(PERFECTTIN_PATCH_VERSION 0)
set(PERFECTTIN_VERSION ${PERFECTTIN_MAJOR_VERSION}.${PERFECTTIN_MINOR_VERSION}.${PERFECTTIN_PATCH_VERSION})
set(COPY_YEAR 2019)

include_directories(${PROJECT_BINARY_DIR} ${Boost_INCLUDE_DIR})
if (${LibPLYXX_FOUND})
include_directories(${LibPLYXX_INCLUDE_DIR})
endif (${LibPLYXX_FOUND})
configure_file (config.h.in config.h)

set(CPACK_PACKAGE_VERSION_MAJOR ${PERFECTTIN_MAJOR_VERSION})
set(CPACK_PACKAGE_VERSION_MINOR ${PERFECTTIN_MINOR_VERSION})
set(CPACK_PACKAGE_VERSION_PATCH ${PERFECTTIN_PATCH_VERSION})
set(CPACK_SOURCE_IGNORE_FILES /\\\\.git;.*~)
include(CPack)

include(CTest)
