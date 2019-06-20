project(bezitopo)
cmake_minimum_required(VERSION 3.1.0)
# Copyright 2019 Pierre Abbat.
# This file is part of Decisite.
# 
# Decisite is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Decisite is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Decisite. If not, see <http://www.gnu.org/licenses/>.

include(TestBigEndian)
include(CheckTypeSize)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set(SHARE_DIR ${CMAKE_INSTALL_PREFIX}/share/bezitopo)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
find_package(LibPLYXX REQUIRED)
find_package(Boost REQUIRED COMPONENTS program_options locale chrono)
add_executable(decisite adjelev.cpp angle.cpp binio.cpp
	       boundrect.cpp cloud.cpp cogo.cpp decisite.cpp
               dxf.cpp edgeop.cpp las.cpp ldecimal.cpp
               leastsquares.cpp manysum.cpp matrix.cpp
               neighbor.cpp octagon.cpp ply.cpp point.cpp pointlist.cpp ps.cpp
               qindex.cpp random.cpp relprime.cpp test.cpp tin.cpp triangle.cpp triop.cpp)
target_link_libraries(decisite ${LibPLYXX_LIBRARY} ${Boost_LIBRARIES})

include(CheckIncludeFiles)
check_include_files(time.h HAVE_TIME_H)
check_include_files(sys/time.h HAVE_SYS_TIME_H)
check_include_files(sys/resource.h HAVE_SYS_RESOURCE_H)
check_include_files(windows.h HAVE_WINDOWS_H)

install(TARGETS decisite DESTINATION bin)

test_big_endian(BIGENDIAN)
check_type_size("int" INT)
check_type_size("int *" INT_POINTER)

set(DECISITE_MAJOR_VERSION 0)
set(DECISITE_MINOR_VERSION 1)
set(DECISITE_PATCH_VERSION 0)
set(DECISITE_VERSION ${DECISITE_MAJOR_VERSION}.${DECISITE_MINOR_VERSION}.${DECISITE_PATCH_VERSION})
set(COPY_YEAR 2019)

include_directories(${PROJECT_BINARY_DIR})
include_directories(${LibPLYXX_INCLUDE_DIR})
configure_file (config.h.in config.h)

set(CPACK_PACKAGE_VERSION_MAJOR ${DECISITE_MAJOR_VERSION})
set(CPACK_PACKAGE_VERSION_MINOR ${DECISITE_MINOR_VERSION})
set(CPACK_PACKAGE_VERSION_PATCH ${DECISITE_PATCH_VERSION})
set(CPACK_SOURCE_IGNORE_FILES /\\\\.git;.*~)
include(CPack)

include(CTest)
