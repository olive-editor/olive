# Olive - Non-Linear Video Editor
# Copyright (C) 2021 Olive Team
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# Set Olive sources and resources
set(OLIVE_SOURCES
  core.h
  core.cpp
)

#set(OLIVE_RESOURCES)

# Add subdirectories, which will populate the above variables
add_subdirectory(audio)
add_subdirectory(cli)
add_subdirectory(codec)
add_subdirectory(common)
add_subdirectory(config)
add_subdirectory(dialog)
add_subdirectory(node)
add_subdirectory(packaging)
add_subdirectory(panel)
add_subdirectory(render)
add_subdirectory(shaders)
add_subdirectory(task)
add_subdirectory(threading)
add_subdirectory(timeline)
add_subdirectory(ts)
add_subdirectory(tool)
add_subdirectory(ui)
add_subdirectory(undo)
add_subdirectory(widget)
add_subdirectory(window)

# Add translations
qt5_add_translation(OLIVE_QM_FILES ${OLIVE_TS_FILES})

set(QRC_BODY "")
foreach(QM_FILE ${OLIVE_QM_FILES})
  get_filename_component(QM_FILENAME_COMPONENT ${QM_FILE} NAME_WE)
  string(APPEND QRC_BODY "<file alias=\"${QM_FILENAME_COMPONENT}\">${QM_FILE}</file>\n")
endforeach()
configure_file(ts/translations.qrc.in ts/translations.qrc @ONLY)

set(OLIVE_RESOURCES
  ${OLIVE_RESOURCES}
  ${CMAKE_CURRENT_BINARY_DIR}/ts/translations.qrc
)

# Add version object
add_library(olive-version-obj
  OBJECT
  version.cpp
  version.h
)
target_link_libraries(olive-version-obj PRIVATE Qt5::Core)
target_compile_options(olive-version-obj PRIVATE -DAPPVERSION="${PROJECT_VERSION}" -DAPPVERSIONLONG="${PROJECT_LONG_VERSION}" )

# Add main library
add_library(libolive-editor
  OBJECT
  ${OLIVE_SOURCES}
  ${OLIVE_RESOURCES}
)

# Remove prefix - prevents CMake calling it "liblibolive-editor"
set_target_properties(libolive-editor PROPERTIES PREFIX "")

# Add application
add_executable(olive-editor
  main.cpp
  $<TARGET_OBJECTS:libolive-editor>
  $<TARGET_OBJECTS:olive-version-obj>
)

# Create docs if doxygen was found
if(DOXYGEN_FOUND)
  set(DOXYGEN_PROJECT_NAME "Olive")
  set(DOXYGEN_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/docs")
  set(DOXYGEN_EXTRACT_ALL "YES")
  set(DOXYGEN_EXTRACT_PRIVATE "YES")
  doxygen_add_docs(docs ALL ${OLIVE_SOURCES})
endif()

# Platform-specific deployment preferences
if (WIN32)
  # Set Windows application icon
  target_sources(olive-editor PRIVATE packaging/windows/resources.rc)

  # Preserve folder structure in visual studio 
  source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${OLIVE_SOURCES})

elseif(APPLE)
  # Set Mac application icon
  set(OLIVE_ICON packaging/macos/olive.icns)
  target_sources(olive-editor PRIVATE ${OLIVE_ICON})

  # Set Mac bundle properties
  set_target_properties(olive-editor PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/packaging/macos/MacOSXBundleInfo.plist.in
    MACOSX_BUNDLE_GUI_IDENTIFIER org.olivevideoeditor.Olive
    MACOSX_BUNDLE_ICON_FILE olive.icns
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
    MACOSX_BUNDLE_BUNDLE_NAME "Olive"
    MACOSX_BUNDLE_INFO_STRING "Olive ${PROJECT_LONG_VERSION}"
    MACOSX_BUNDLE_COPYRIGHT "©2018-2021 Olive Studios LLC and others. Published under the GNU General Public License version 3.0."
    RESOURCE "${OLIVE_ICON}"
    OUTPUT_NAME "Olive"
  )
elseif(UNIX)
  # Set Linux-specific properties for application
  install(TARGETS olive-editor RUNTIME DESTINATION bin)
endif()

# Set link libraries
target_link_libraries(olive-editor PRIVATE ${OLIVE_LIBRARIES})
target_link_libraries(libolive-editor PRIVATE ${OLIVE_LIBRARIES})

# Set compile options
target_compile_options(olive-editor PRIVATE ${OLIVE_COMPILE_OPTIONS})
target_compile_options(libolive-editor PRIVATE ${OLIVE_COMPILE_OPTIONS})

# Set global definitions
target_compile_definitions(olive-editor PRIVATE ${OLIVE_DEFINITIONS})
target_compile_definitions(libolive-editor PRIVATE ${OLIVE_DEFINITIONS})

# Set include dirs
target_include_directories(olive-editor PRIVATE ${OLIVE_INCLUDE_DIRS})
target_include_directories(libolive-editor PRIVATE ${OLIVE_INCLUDE_DIRS})

# Add crash handler
if (GoogleCrashpad_FOUND AND Qt5Network_FOUND)
  add_subdirectory(crashhandler)
endif()
