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

# Create crash handler executable
add_executable(
  olive-crashhandler
  crashhandler.cpp
  crashhandler.h
  $<TARGET_OBJECTS:olive-version-obj>
)

# Disable console appearing on crash handler dialog
set_target_properties(olive-crashhandler PROPERTIES
  WIN32_EXECUTABLE TRUE
)

# Set crash handler includes
target_include_directories(
  olive-crashhandler
  PRIVATE
  ${CMAKE_SOURCE_DIR}/app
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

set(CRASHPAD_HANDLER "crashpad_handler${CMAKE_EXECUTABLE_SUFFIX}")
set(MINIDUMP_STACKWALK "minidump_stackwalk${CMAKE_EXECUTABLE_SUFFIX}")

if(APPLE)
  # Move crash handler executables inside Mac app bundle
  add_custom_command(TARGET olive-crashhandler POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different olive-crashhandler $<TARGET_FILE_DIR:olive-editor>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CRASHPAD_LIBRARY_DIRS}/${CRASHPAD_HANDLER} $<TARGET_FILE_DIR:olive-editor>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${BREAKPAD_BIN_DIR}/${MINIDUMP_STACKWALK} $<TARGET_FILE_DIR:olive-editor>
  )
elseif(UNIX)
  install(TARGETS olive-crashhandler RUNTIME DESTINATION bin)
  install(PROGRAMS ${CRASHPAD_LIBRARY_DIRS}/${CRASHPAD_HANDLER} DESTINATION bin)
  install(PROGRAMS ${BREAKPAD_BIN_DIR}/${MINIDUMP_STACKWALK} DESTINATION bin)
endif()

target_compile_definitions(olive-crashhandler PRIVATE ${OLIVE_DEFINITIONS})
