################################################################################
#
# Copyright 2015-2017 Daniel Bulla	<d.bulla@fh-aachen.de>
#           2015-2016 Tobias Neumann	<t.neumann@fh-aachen.de>
#
################################################################################

#  This file is part of mapit.
#
#  Mapit is free software: you can redistribute it and/or modify
#  it under the terms of the GNU Lesser General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  Mapit is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Lesser General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public License
#  along with mapit.  If not, see <http://www.gnu.org/licenses/>.
#

project(layertype_octomap)
cmake_minimum_required(VERSION 2.8)

file(GLOB_RECURSE LAYERTYPE_OCTOMAP_SOURCES ${PROJECT_SOURCE_DIR}/src/*.cpp ${PROJECT_SOURCE_DIR}/src/*.h)

add_library(${PROJECT_NAME} SHARED ${LAYERTYPE_OCTOMAP_SOURCES} )

target_include_directories(${PROJECT_NAME} PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                                                   $<INSTALL_INTERFACE:include>
                                           PRIVATE ${PROJECT_SOURCE_DIR}/src/)

target_compile_definitions(${PROJECT_NAME} PRIVATE PROJECT_NAME="${PROJECT_NAME}"
                                           PUBLIC  OctomapEntitydata_TYPENAME="${PROJECT_NAME}")

if(UNIX)
   # tell CMake Prefix is "lib" otherwise it can not be queried
   set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "lib")
endif(UNIX)
get_target_property(OLD_PREFIX ${PROJECT_NAME} PREFIX)
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX ${OLD_PREFIX}${MAPIT_INSTALL_ENTITYTYPES})

include(GenerateExportHeader)

generate_export_header(${PROJECT_NAME})

set_property(TARGET ${PROJECT_NAME} PROPERTY
  INTERFACE_mapit_layertype_octomap_MAJOR_VERSION ${MAPIT_MAJOR_VERSION})
set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY
  COMPATIBLE_INTERFACE_STRING MAPIT_MAJOR_VERSION
)

install(TARGETS ${PROJECT_NAME} EXPORT mapit_layertype_octomap_targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)
install(DIRECTORY include/mapit DESTINATION include )

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/mapit_layertype_octomapConfigVersion.cmake"
  VERSION 1
  COMPATIBILITY AnyNewerVersion
)

export(EXPORT mapit_layertype_octomap_targets
  FILE "${CMAKE_CURRENT_BINARY_DIR}/mapit_layertype_octomapTargets.cmake"
  NAMESPACE mapit::
)
configure_file(cmake/mapit_layertype_octomapConfig.cmake
  "${CMAKE_CURRENT_BINARY_DIR}/mapit_layertype_octomapConfig.cmake"
  COPYONLY
)

set(ConfigPackageLocation lib/cmake/mapit)
install(EXPORT mapit_layertype_octomap_targets
  FILE
    mapit_layertype_octomapTargets.cmake
  NAMESPACE
    mapit::
  DESTINATION
    ${ConfigPackageLocation}
)
install(
  FILES
    cmake/mapit_layertype_octomapConfig.cmake
    "${CMAKE_CURRENT_BINARY_DIR}/mapit_layertype_octomapConfigVersion.cmake"
  DESTINATION
    ${ConfigPackageLocation}
  COMPONENT
    Devel
)
