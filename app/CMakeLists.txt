# Olive - Non-Linear Video Editor
# Copyright (C) 2020 Olive Team
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
  ${OLIVE_SOURCES}
  core.h
  core.cpp
  main.cpp
)

# Include resource files for specific platforms
if (WIN32)
  set(OLIVE_RESOURCES
    ${OLIVE_RESOURCES}
    packaging/windows/resources.rc
  )
elseif(APPLE)
  set(OLIVE_ICON packaging/macos/olive.icns)

  set(OLIVE_RESOURCES
    ${OLIVE_RESOURCES}
    ${OLIVE_ICON}
  )
endif()

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
add_subdirectory(project)
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

# Add executable
add_executable(olive-editor
  ${OLIVE_SOURCES}
  ${OLIVE_RESOURCES}
)

# Set global definitions
target_compile_definitions(olive-editor PRIVATE ${OLIVE_DEFINITIONS})

# Set include directories
target_include_directories(
  olive-editor
  PRIVATE
  ${FFMPEG_INCLUDE_DIRS}
  ${OCIO_INCLUDE_DIRS}
  ${OIIO_INCLUDE_DIRS}
  ${OPENEXR_INCLUDES}
)

# Set compiler options
if(MSVC)
  target_compile_options(
    olive-editor
    PRIVATE
    /WX
    /wd4267
    /wd4244
    /experimental:external
    /external:anglebrackets
    /external:W0
    "$<$<CONFIG:RELEASE>:/O2>"
    "$<$<COMPILE_LANGUAGE:CXX>:/MP>"
  )
else()
  target_compile_options(
    olive-editor
    PRIVATE
    "$<$<CONFIG:RELEASE>:-O2>"
    -Werror
    -Wuninitialized
    -pedantic-errors
    -Wall
    -Wextra
    -Wno-unused-parameter
    -Wshadow
  )
endif()

# Set link libraries
target_link_libraries(
  olive-editor
  PRIVATE
  Qt5::Core
  Qt5::Gui
  Qt5::Widgets
  Qt5::Multimedia
  Qt5::OpenGL
  Qt5::Svg
  Qt5::Concurrent
  OpenGL::GL
  FFMPEG::avutil
  FFMPEG::avcodec
  FFMPEG::avformat
  FFMPEG::avfilter
  FFMPEG::swscale
  FFMPEG::swresample
  ${OCIO_LIBRARIES}
  ${OIIO_LIBRARIES}
  ${OPENEXR_LIBRARIES}
)

# Set Mac-specific properties for application
if (APPLE)
  set_target_properties(olive-editor PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/packaging/macos/MacOSXBundleInfo.plist.in
    MACOSX_BUNDLE_GUI_IDENTIFIER org.olivevideoeditor.Olive
    MACOSX_BUNDLE_ICON_FILE olive.icns
    RESOURCE "${OLIVE_ICON}"
    OUTPUT_NAME "Olive"
  )

  # Enables use of special functions for slider dragging
  if (NOT GoogleCrashpad_FOUND)
    target_link_libraries(
      olive-editor
      PRIVATE
      "-framework ApplicationServices"
    )
  endif()

  # HACK: Brew on macOS separates OpenEXR and IlmBase into two folders even though they seem to
  #       expect to be in one. This includes the IlmBase files as if they were in the same folders
  #       as the OpenEXR headers. This would be better rolled into FindOpenEXR.cmake.
  target_include_directories(
    olive-editor
    PRIVATE
    ${ILMBASE_INCLUDES}/OpenEXR
  )
endif()

# Set Linux-specific properties for application
if(UNIX AND NOT APPLE)
  install(TARGETS olive-editor RUNTIME DESTINATION bin)
endif()

# Enable OTIO if found
if (OpenTimelineIO_FOUND)
  target_compile_definitions(olive-editor PRIVATE USE_OTIO)

  target_include_directories(
    olive-editor
    PRIVATE
    ${OTIO_INCLUDE_DIRS}
  )

  target_link_libraries(
    olive-editor
    PRIVATE
    ${OTIO_LIBRARIES}
  )
endif()

# Enable Crashpad if found
if (GoogleCrashpad_FOUND)
  # Signal code to use Crashpad
  target_compile_definitions(
    olive-editor
    PRIVATE
    USE_CRASHPAD
  )

  # Include Crashpad headers
  target_include_directories(
    olive-editor
    PRIVATE
    ${CRASHPAD_INCLUDE_DIRS}
  )

  # Include Crashpad libs
  target_link_libraries(
    olive-editor
    PRIVATE
    ${CRASHPAD_LIBRARIES}
  )

  # Create crash handler executable
  add_executable(
    olive-crashhandler
    dialog/crashhandler/crashhandler.h
    dialog/crashhandler/crashhandler.cpp
    dialog/crashhandler/crashhandlermain.cpp
  )

  # Disable console appearing on crash handler dialog
  set_target_properties(olive-crashhandler PROPERTIES
    WIN32_EXECUTABLE TRUE
  )

  # Set crash handler includes
  target_include_directories(
    olive-crashhandler
    PRIVATE
    ${CRASHPAD_INCLUDE_DIRS}
  )

  # Set crash handler libs
  target_link_libraries(
    olive-crashhandler
    PRIVATE
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Network
    ${CRASHPAD_LIBRARIES}
  )

  # Set global Olive compiler definitions
  target_compile_definitions(olive-crashhandler PRIVATE ${OLIVE_DEFINITIONS})

  set(CRASHPAD_HANDLER "crashpad_handler${CMAKE_EXECUTABLE_SUFFIX}")
  set(MINIDUMP_STACKWALK "minidump_stackwalk${CMAKE_EXECUTABLE_SUFFIX}")

  if(UNIX AND NOT APPLE)
    install(TARGETS olive-crashhandler RUNTIME DESTINATION bin)
    install(PROGRAMS ${CRASHPAD_LIBRARY_DIRS}/${CRASHPAD_HANDLER} DESTINATION bin)
    install(PROGRAMS ${BREAKPAD_BIN_DIR}/${MINIDUMP_STACKWALK} DESTINATION bin)
  endif()

  if(APPLE)
    # Move crash handler executables inside Mac app bundle
    add_custom_command(TARGET olive-crashhandler POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different olive-crashhandler $<TARGET_FILE_DIR:olive-editor>
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CRASHPAD_LIBRARY_DIRS}/${CRASHPAD_HANDLER} $<TARGET_FILE_DIR:olive-editor>
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${BREAKPAD_BIN_DIR}/${MINIDUMP_STACKWALK} $<TARGET_FILE_DIR:olive-editor>
    )
  endif()
endif()

# Create docs if doxygen was found
if(DOXYGEN_FOUND)
  set(DOXYGEN_PROJECT_NAME "Olive")
  set(DOXYGEN_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/docs")
  set(DOXYGEN_EXTRACT_ALL "YES")
  set(DOXYGEN_EXTRACT_PRIVATE "YES")
  doxygen_add_docs(docs ALL ${OLIVE_SOURCES})
endif()
