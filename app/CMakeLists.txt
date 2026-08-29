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
add_subdirectory(config)
add_subdirectory(dialog)
add_subdirectory(node)
add_subdirectory(packaging)
add_subdirectory(panel)
add_subdirectory(render)
add_subdirectory(shaders)
add_subdirectory(task)
add_subdirectory(timeline)
add_subdirectory(ts)
add_subdirectory(tool)
add_subdirectory(ui)
add_subdirectory(undo)
add_subdirectory(widget)
add_subdirectory(window)

# Add translations
qt_add_translation(OLIVE_QM_FILES ${OLIVE_TS_FILES})

set(QRC_BODY "")
foreach (QM_FILE ${OLIVE_QM_FILES})
    get_filename_component(QM_FILENAME_COMPONENT ${QM_FILE} NAME_WE)
    string(APPEND QRC_BODY "<file alias=\"${QM_FILENAME_COMPONENT}\">${QM_FILE}</file>\n")
endforeach ()
configure_file(ts/translations.qrc.in ts/translations.qrc @ONLY)

set(OLIVE_RESOURCES
        ${OLIVE_RESOURCES}
        ${CMAKE_CURRENT_BINARY_DIR}/ts/translations.qrc
        render/job/pluginjob.cpp
        render/job/pluginjob.h
        widget/nodeparamview/nodeparambutton.cpp
        widget/nodeparamview/nodeparambutton.h
)

# Add version object
add_library(olive-version-obj
        OBJECT
        version.cpp
        version.h
)
target_link_libraries(olive-version-obj PRIVATE Qt${QT_VERSION_MAJOR}::Core)
target_compile_options(olive-version-obj PRIVATE -DAPPVERSION="${PROJECT_VERSION}" -DAPPVERSIONLONG="${PROJECT_LONG_VERSION}")

# Add main library
add_library(libolive-editor
        OBJECT
        ${OLIVE_SOURCES}
        ${OLIVE_RESOURCES}
)
add_subdirectory(common)
add_subdirectory(pluginSupport)


target_compile_features(libolive-editor PUBLIC cxx_std_23)
include_directories(../third_party/openfx/include
        ../third_party/openfx/HostSupport/include)
# Remove prefix - prevents CMake calling it "liblibolive-editor"
set_target_properties(libolive-editor PROPERTIES PREFIX "")

option(OAK_ENABLE_DYNAMIC_RENDER_BACKEND "Build and use the dynamic render backend adapter" ON)
if (OAK_ENABLE_DYNAMIC_RENDER_BACKEND)
    set_target_properties(libolive-editor PROPERTIES POSITION_INDEPENDENT_CODE ON)
    target_compile_definitions(libolive-editor PRIVATE OAK_ENABLE_DYNAMIC_RENDER_BACKEND)

    foreach (target olivecore kddockwidgets)
        if (TARGET ${target})
            set_target_properties(${target} PROPERTIES POSITION_INDEPENDENT_CODE ON)
        endif ()
    endforeach ()

    # Render core library: the minimal set of code required by the OpenGL/Vulkan backend
    # libraries. Keeping this separate from libolive-editor prevents the backends from
    # dragging in editor-wide state (project, task, cache, UI, etc.).
    add_library(libolive-rendercore STATIC
            common/avframeptr.h
            common/define.h
            common/filefunctions.cpp
            common/filefunctions.h
            common/qtutils.cpp
            common/qtutils.h
            common/xmlutils.cpp
            common/xmlutils.h
            node/param.cpp
            node/param.h
            node/splitvalue.h
            node/value.cpp
            node/value.h
            node/valuedatabase.cpp
            node/valuedatabase.h
            render/backend/dynamicrenderer.cpp
            render/backend/dynamicrenderer.h
            render/backend/renderbackend_c.h
            render/job/acceleratedjob.cpp
            render/job/acceleratedjob.h
            render/job/shaderjob.h
            render/renderer.cpp
            render/renderer.h
            render/shadercode.h
            render/texture.cpp
            render/texture.h
            render/videoparams.cpp
            render/videoparams.h
    )
    set_target_properties(libolive-rendercore PROPERTIES POSITION_INDEPENDENT_CODE ON)
    target_include_directories(libolive-rendercore PUBLIC
            ${CMAKE_SOURCE_DIR}/app
            ${CMAKE_SOURCE_DIR}/third_party/openfx/include
            ${CMAKE_SOURCE_DIR}/third_party/openfx/HostSupport/include
            ${OLIVE_INCLUDE_DIRS}
    )
    target_link_libraries(libolive-rendercore PUBLIC ${OLIVE_LIBRARIES} OfxHost)
    target_compile_definitions(libolive-rendercore PUBLIC ${OLIVE_DEFINITIONS})
    target_compile_options(libolive-rendercore PUBLIC ${OLIVE_COMPILE_OPTIONS})

    add_library(oakgl SHARED
            render/opengl/openglbackend_c.cpp
            render/opengl/openglrenderer.cpp
            render/opengl/openglrenderer.h
    )
    target_link_libraries(oakgl PRIVATE libolive-rendercore)
    target_compile_definitions(oakgl PRIVATE OAK_RENDER_BACKEND_PLUGIN)
    set_target_properties(oakgl PROPERTIES
            OUTPUT_NAME oakgl
            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
    if (WIN32)
        set_target_properties(oakgl PROPERTIES PREFIX "")
    endif ()
    install(TARGETS oakgl
            RUNTIME DESTINATION bin
            LIBRARY DESTINATION lib
            ARCHIVE DESTINATION lib
    )

    if (Vulkan_FOUND)
        add_library(oakvulkan SHARED
                render/vulkan/vulkanbackend_c.cpp
                render/vulkan/vulkanrenderer.cpp
                render/vulkan/vulkanrenderer.h
        )
        target_link_libraries(oakvulkan PRIVATE libolive-rendercore)
        target_link_libraries(oakvulkan PRIVATE Vulkan::Vulkan)
        target_compile_definitions(oakvulkan PRIVATE OAK_HAS_VULKAN)
        if (SHADERC_FOUND)
            target_link_libraries(oakvulkan PRIVATE ${SHADERC_LIBRARIES})
            target_include_directories(oakvulkan PRIVATE ${SHADERC_INCLUDE_DIRS})
            target_compile_definitions(oakvulkan PRIVATE OAK_HAS_SHADERC)
        endif ()
        set_target_properties(oakvulkan PROPERTIES
                OUTPUT_NAME oakvulkan
                LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        )
        if (WIN32)
            set_target_properties(oakvulkan PROPERTIES PREFIX "")
        endif ()
        install(TARGETS oakvulkan
                RUNTIME DESTINATION bin
                LIBRARY DESTINATION lib
                ARCHIVE DESTINATION lib
        )
    endif ()
endif ()

add_library(oakgl-cabi-check OBJECT
        render/opengl/openglbackend_c.cpp
)
target_include_directories(oakgl-cabi-check PRIVATE
        ${CMAKE_SOURCE_DIR}/app
        ${CMAKE_SOURCE_DIR}/third_party/openfx/include
        ${CMAKE_SOURCE_DIR}/third_party/openfx/HostSupport/include
        ${OLIVE_INCLUDE_DIRS}
)
target_link_libraries(oakgl-cabi-check PRIVATE ${OLIVE_LIBRARIES} OfxHost)
target_compile_definitions(oakgl-cabi-check PRIVATE ${OLIVE_DEFINITIONS})
target_compile_options(oakgl-cabi-check PRIVATE ${OLIVE_COMPILE_OPTIONS})

if (Vulkan_FOUND)
    add_library(oakvulkan-cabi-check OBJECT
            render/vulkan/vulkanbackend_c.cpp
            render/vulkan/vulkanrenderer.cpp
            render/vulkan/vulkanrenderer.h
    )
    target_include_directories(oakvulkan-cabi-check PRIVATE
            ${CMAKE_SOURCE_DIR}/app
            ${CMAKE_SOURCE_DIR}/third_party/openfx/include
            ${CMAKE_SOURCE_DIR}/third_party/openfx/HostSupport/include
            ${OLIVE_INCLUDE_DIRS}
    )
    target_link_libraries(oakvulkan-cabi-check PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Gui Qt${QT_VERSION_MAJOR}::Widgets)
    target_link_libraries(oakvulkan-cabi-check PRIVATE Vulkan::Vulkan)
    target_compile_definitions(oakvulkan-cabi-check PRIVATE OAK_HAS_VULKAN)
    if (SHADERC_FOUND)
        target_link_libraries(oakvulkan-cabi-check PRIVATE ${SHADERC_LIBRARIES})
        target_include_directories(oakvulkan-cabi-check PRIVATE ${SHADERC_INCLUDE_DIRS})
        target_compile_definitions(oakvulkan-cabi-check PRIVATE OAK_HAS_SHADERC)
    endif ()
    target_compile_definitions(oakvulkan-cabi-check PRIVATE ${OLIVE_DEFINITIONS})
    target_compile_options(oakvulkan-cabi-check PRIVATE ${OLIVE_COMPILE_OPTIONS})
endif ()

# Add application
add_executable(olive-editor
        main.cpp
        $<TARGET_OBJECTS:libolive-editor>
        $<TARGET_OBJECTS:olive-version-obj>
)
target_include_directories(olive-editor PUBLIC pluginSupport)
target_link_libraries(olive-editor PUBLIC OfxHost)
add_dependencies(olive-editor oakgl-cabi-check)
if (OAK_ENABLE_DYNAMIC_RENDER_BACKEND)
    add_dependencies(olive-editor oakgl)
    if (TARGET oakvulkan)
        add_dependencies(olive-editor oakvulkan)
    endif ()
endif ()
# Ensure the render worker is always built together with the editor on every platform.
add_dependencies(olive-editor olive-render-worker)

set_target_properties(olive-editor PROPERTIES OUTPUT_NAME "oak-editor")

# Add render worker process (oak-render-worker).
# Reuses the libolive-editor object library so the worker shares the exact same render/node/codec
# code as the editor. It is a headless app that owns its own offscreen GL context. The link set is
# currently the full OLIVE_LIBRARIES for simplicity; trimming UI-only dependencies is a later-phase
# cleanup (see render-process-isolation plan).
add_executable(olive-render-worker
        render/worker/workermain.cpp
        $<TARGET_OBJECTS:libolive-editor>
        $<TARGET_OBJECTS:olive-version-obj>
)
set_target_properties(olive-render-worker PROPERTIES OUTPUT_NAME "oak-render-worker")
if (APPLE)
    target_sources(olive-render-worker PRIVATE render/worker/workermain_mac.mm)
    target_link_libraries(olive-render-worker PRIVATE "-framework Cocoa")
endif ()
target_include_directories(olive-render-worker PUBLIC pluginSupport)
target_link_libraries(olive-render-worker PUBLIC OfxHost)
if (OAK_ENABLE_DYNAMIC_RENDER_BACKEND)
    target_compile_definitions(olive-render-worker PRIVATE OAK_ENABLE_DYNAMIC_RENDER_BACKEND)
    add_dependencies(olive-render-worker oakgl)
    if (TARGET oakvulkan)
        add_dependencies(olive-render-worker oakvulkan)
    endif ()
endif ()
# Create docs if doxygen was found
if (DOXYGEN_FOUND)
    set(DOXYGEN_PROJECT_NAME "Oak Video Editor")
    set(DOXYGEN_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/docs")
    set(DOXYGEN_EXTRACT_ALL "YES")
    set(DOXYGEN_EXTRACT_PRIVATE "YES")
    doxygen_add_docs(docs ALL ${OLIVE_SOURCES})
endif ()

# Platform-specific deployment preferences
if (WIN32)
    # Set Windows application icon
    target_sources(olive-editor PRIVATE packaging/windows/resources.rc)

    # Preserve folder structure in visual studio
    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${OLIVE_SOURCES})

elseif (APPLE)
    # Set Mac application icon
    set(OLIVE_ICON packaging/macos/oak.icns)
    target_sources(olive-editor PRIVATE ${OLIVE_ICON})

    # Set Mac bundle properties
    set_target_properties(olive-editor PROPERTIES
            MACOSX_BUNDLE TRUE
            MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/packaging/macos/MacOSXBundleInfo.plist.in
            MACOSX_BUNDLE_GUI_IDENTIFIER org.oakvideoeditor.Oak
            MACOSX_BUNDLE_ICON_FILE oak.icns
            MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
            MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
            MACOSX_BUNDLE_BUNDLE_NAME "Oak Video Editor"
            MACOSX_BUNDLE_INFO_STRING "Oak Video Editor ${PROJECT_LONG_VERSION}"
            MACOSX_BUNDLE_COPYRIGHT "©2018-2021 Olive Studios LLC and others. Fork maintained by Oak Video Editor Team."
            RESOURCE "${OLIVE_ICON}"
            OUTPUT_NAME "Oak"
    )

    # Copy the render worker, dynamic render backends, and the FFmpeg bridge
    # library into the app bundle. They are looked up in
    # QCoreApplication::applicationDirPath(), which on macOS points to
    # Oak.app/Contents/MacOS.
    add_custom_command(TARGET olive-editor POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_BUNDLE_DIR:olive-editor>/Contents/MacOS
            COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:olive-render-worker> $<TARGET_BUNDLE_DIR:olive-editor>/Contents/MacOS/
            COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:oakgl> $<TARGET_BUNDLE_DIR:olive-editor>/Contents/MacOS/
            COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:ffmpeg_bridge> $<TARGET_BUNDLE_DIR:olive-editor>/Contents/MacOS/
            COMMENT "Copying oak-render-worker, render backends, and ffmpeg_bridge into Oak.app"
    )
    if (TARGET oakvulkan)
        add_custom_command(TARGET olive-editor POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:oakvulkan> $<TARGET_BUNDLE_DIR:olive-editor>/Contents/MacOS/
        )
    endif ()
elseif (UNIX)
    # Set Linux-specific properties for application
    # The render worker must ship next to oak-editor: RenderWorkerPool spawns
    # it from QCoreApplication::applicationDirPath()
    install(TARGETS olive-editor olive-render-worker RUNTIME DESTINATION bin)
endif ()

# Set link libraries
target_link_libraries(olive-editor PRIVATE ${OLIVE_LIBRARIES})
target_link_libraries(libolive-editor PRIVATE ${OLIVE_LIBRARIES})
target_link_libraries(olive-render-worker PRIVATE ${OLIVE_LIBRARIES})

# The ffmpeg_bridge shared library ships next to the binaries: inside the
# macOS app bundle (Contents/MacOS, resolved via @loader_path), and in the
# standard lib directory on other platforms.
if (APPLE)
    # macOS bundles are distributed via POST_BUILD copies (not install()), so
    # @loader_path must already be in the build-tree binaries' RPATH.
    set(OLIVE_FB_RPATH "@loader_path")
    set_target_properties(olive-editor olive-render-worker PROPERTIES
            BUILD_RPATH "@loader_path")
elseif (UNIX)
    set(OLIVE_FB_RPATH "$ORIGIN/../lib")
endif ()
if (OLIVE_FB_RPATH)
    set_target_properties(olive-editor olive-render-worker PROPERTIES
            INSTALL_RPATH "${OLIVE_FB_RPATH}")
    if (TARGET oakgl)
        set_target_properties(oakgl PROPERTIES INSTALL_RPATH "${OLIVE_FB_RPATH}")
    endif ()
    if (TARGET oakvulkan)
        set_target_properties(oakvulkan PROPERTIES INSTALL_RPATH "${OLIVE_FB_RPATH}")
    endif ()
endif ()

# Set compile options
target_compile_options(olive-editor PRIVATE ${OLIVE_COMPILE_OPTIONS})
target_compile_options(libolive-editor PRIVATE ${OLIVE_COMPILE_OPTIONS})
target_compile_options(olive-render-worker PRIVATE ${OLIVE_COMPILE_OPTIONS})

# Set global definitions
target_compile_definitions(olive-editor PRIVATE ${OLIVE_DEFINITIONS})
target_compile_definitions(libolive-editor PRIVATE ${OLIVE_DEFINITIONS})
target_compile_definitions(olive-render-worker PRIVATE ${OLIVE_DEFINITIONS})

# Set include dirs
target_include_directories(olive-editor PRIVATE ${OLIVE_INCLUDE_DIRS})
target_include_directories(libolive-editor PRIVATE ${OLIVE_INCLUDE_DIRS})
target_include_directories(olive-render-worker PRIVATE ${OLIVE_INCLUDE_DIRS})

# Install the render worker alongside the editor on Linux.
if (UNIX AND NOT APPLE)
    install(TARGETS olive-render-worker RUNTIME DESTINATION bin)
endif ()

# Add crash handler
if (GoogleCrashpad_FOUND AND Qt${QT_VERSION_MAJOR}Network_FOUND)
    add_subdirectory(crashhandler)
endif ()
