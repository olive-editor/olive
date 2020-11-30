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

if (WIN32)
  set(OLIVE_RESOURCES
    ${OLIVE_RESOURCES}
    packaging/windows/resources.rc
  )
endif()

add_subdirectory(audio)
add_subdirectory(cli)
add_subdirectory(codec)
add_subdirectory(common)
add_subdirectory(config)
add_subdirectory(dialog)
add_subdirectory(node)
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

if(APPLE)
  set(OLIVE_ICON packaging/macos/olive.icns)

  set(OLIVE_RESOURCES
    ${OLIVE_RESOURCES}
    ${OLIVE_ICON}
  )
endif()

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

if(APPLE)
  set_target_properties(olive-editor PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/packaging/macos/MacOSXBundleInfo.plist.in
    MACOSX_BUNDLE_GUI_IDENTIFIER org.olivevideoeditor.Olive
    MACOSX_BUNDLE_ICON_FILE olive.icns
    RESOURCE "${OLIVE_ICON}"
    OUTPUT_NAME "Olive"
  )
endif()

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

if(UNIX AND NOT APPLE)
  target_link_options(
    olive-editor
    PRIVATE
    -rdynamic
  )
endif()

# Set include directories
target_include_directories(
  olive-editor
  PRIVATE
  ${FFMPEG_INCLUDE_DIRS}
  ${OCIO_INCLUDE_DIRS}
  ${OIIO_INCLUDE_DIRS}
  ${OPENEXR_INCLUDES}

  # HACK: Brew on macOS separates OpenEXR and IlmBase into two folders even though they seem to
  #       expect to be in one. This includes the IlmBase files as if they were in the same folders
  #       as the OpenEXR headers.
  ${ILMBASE_INCLUDES}/OpenEXR
)

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

if (WIN32)
  target_link_libraries(
    olive-editor
    PRIVATE
    DbgHelp
  )
elseif (APPLE)
  target_link_libraries(
    olive-editor
    PRIVATE
    "-framework ApplicationServices"
  )
endif()

# Enable OTIO if found
if (OpenTimelineIO_FOUND)
  set(OLIVE_DEFINITIONS ${OLIVE_DEFINITIONS} USE_OTIO)

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

if(UNIX AND NOT APPLE)
  install(TARGETS olive-editor RUNTIME DESTINATION bin)
endif()

# Enable Crashpad if found
if (GoogleCrashpad_FOUND)
  set(OLIVE_DEFINITIONS ${OLIVE_DEFINITIONS} USE_CRASHPAD)

  target_include_directories(
    olive-editor
    PRIVATE
    ${CRASHPAD_INCLUDE_DIRS}
  )

  target_link_libraries(
    olive-editor
    PRIVATE
    ${CRASHPAD_LIBRARIES}
  )

  set(OLIVE_CRASH_TARGET "olive-crashhandler")

  set(OLIVE_CRASH_SOURCES
    dialog/crashhandler/crashhandler.h
    dialog/crashhandler/crashhandler.cpp
    dialog/crashhandler/crashhandlermain.cpp
  )

  if (WIN32)
    add_executable(
      ${OLIVE_CRASH_TARGET}
      WIN32
      ${OLIVE_CRASH_SOURCES}
    )
  else()
    add_executable(
      ${OLIVE_CRASH_TARGET}
      ${OLIVE_CRASH_SOURCES}
    )
  endif()

  target_include_directories(
    ${OLIVE_CRASH_TARGET}
    PRIVATE
    ${CRASHPAD_INCLUDE_DIRS}
    )

  target_link_libraries(
    ${OLIVE_CRASH_TARGET}
    PRIVATE
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Network
    ${CRASHPAD_LIBRARIES}
  )

  set(CRASHPAD_HANDLER "crashpad_handler${CMAKE_EXECUTABLE_SUFFIX}")
  set(MINIDUMP_STACKWALK "minidump_stackwalk${CMAKE_EXECUTABLE_SUFFIX}")

  if(UNIX AND NOT APPLE)
    install(TARGETS ${OLIVE_CRASH_TARGET} RUNTIME DESTINATION bin)
    install(PROGRAMS ${CRASHPAD_LIBRARY_DIRS}/${CRASHPAD_HANDLER} DESTINATION bin)
    install(PROGRAMS ${BREAKPAD_BIN_DIR}/${MINIDUMP_STACKWALK} DESTINATION bin)
  endif()

  if(APPLE)
    # Move crash handler executables inside Mac app bundle
    add_custom_command(TARGET ${OLIVE_CRASH_TARGET} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${OLIVE_CRASH_TARGET} $<TARGET_FILE_DIR:olive-editor>
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CRASHPAD_LIBRARY_DIRS}/${CRASHPAD_HANDLER} $<TARGET_FILE_DIR:olive-editor>
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${BREAKPAD_BIN_DIR}/${MINIDUMP_STACKWALK} $<TARGET_FILE_DIR:olive-editor>
    )
  endif()
endif()

# Set compiler definitions
target_compile_definitions(olive-editor PRIVATE ${OLIVE_DEFINITIONS})

add_subdirectory(packaging)

if(DOXYGEN_FOUND)
  set(DOXYGEN_PROJECT_NAME "Olive")
  set(DOXYGEN_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/docs")
  set(DOXYGEN_EXTRACT_ALL "YES")
  set(DOXYGEN_EXTRACT_PRIVATE "YES")
  doxygen_add_docs(docs ALL ${OLIVE_SOURCES})
endif()
