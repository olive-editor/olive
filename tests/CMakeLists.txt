# Olive - Non-Linear Video Editor
# Copyright (C) 2022 Olive Team
# Modifications Copyright (C) 2025 mikesolar
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

function(olive_add_test GROUP NAME SOURCE)
  file(READ "${SOURCE}" TEST_FILE_CONTENT)
  string(REGEX MATCHALL "OLIVE_ADD_TEST\(.[A-Za-z0-9_]+\)" TEST_FUNCTIONS ${TEST_FILE_CONTENT})
  set(TEST_BODY "int main(int argc, char** argv)\n{\n  int ret;(void)ret;")
  set(TEST_INDEX 1)
  list(LENGTH TEST_FUNCTIONS TEST_COUNT)
  foreach (TEST_FUNC ${TEST_FUNCTIONS})
    string(REPLACE "OLIVE_ADD_TEST(" "" TEST_FUNC "${TEST_FUNC}")
    string(APPEND TEST_BODY "  std::cout << \"[${TEST_INDEX}/${TEST_COUNT}] ${GROUP} - ${TEST_FUNC}\";\n")
    string(APPEND TEST_BODY "  if ((ret = olive::Test${TEST_FUNC}()) == OLIVE_TEST_SUCCESS) {std::cout << \" - PASSED\" << std::endl;}else{std::cout << \" - FAILED AT LINE \" << ret << std::endl;return 1;}\n")
    MATH(EXPR TEST_INDEX "${TEST_INDEX}+1")
  endforeach()
  string(APPEND TEST_BODY "  return 0;\n}")

  set_property(
    DIRECTORY
    APPEND
    PROPERTY CMAKE_CONFIGURE_DEPENDS ${SOURCE}
  )

  set(OUTPUT_FILE "${CMAKE_CURRENT_BINARY_DIR}/${SOURCE}")

  string(APPEND TEST_FILE_CONTENT "\n${TEST_BODY}")
  file(WRITE "${OUTPUT_FILE}" "${TEST_FILE_CONTENT}")

  add_executable(${NAME} ${OUTPUT_FILE} $<TARGET_OBJECTS:libolive-editor> $<TARGET_OBJECTS:olive-version-obj>)
  target_include_directories(
    ${NAME}
    PRIVATE
    ${CMAKE_SOURCE_DIR}/app
    ${CMAKE_SOURCE_DIR}/tests
    ${CMAKE_SOURCE_DIR}/third_party/openfx/include
    ${CMAKE_SOURCE_DIR}/third_party/openfx/HostSupport/include
    ${OLIVE_INCLUDE_DIRS}
  )
  target_link_libraries(
    ${NAME}
    PRIVATE
    ${OLIVE_LIBRARIES}
  )
  target_compile_definitions(
    ${NAME}
    PRIVATE
    ${OLIVE_DEFINITIONS}
  )
  target_compile_options(
    ${NAME}
    PRIVATE
    ${OLIVE_COMPILE_OPTIONS}
  )
  if (MSVC)
    add_test("Olive.${GROUP}.${NAME}" ${NAME})
  else()
    add_test(${NAME} ${NAME})
  endif()

  if (WIN32)
    # Windows has no RPATH: ffmpeg_bridge.dll must sit next to the executable
    add_custom_command(TARGET ${NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:ffmpeg_bridge> $<TARGET_FILE_DIR:${NAME}>)
  endif()
endfunction()

include(FetchContent)
find_package(GTest QUIET)
if (NOT GTest_FOUND)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
  )
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)
endif()

add_subdirectory(compositing)
add_subdirectory(general)
add_subdirectory(timeline)
add_subdirectory(gtest)
