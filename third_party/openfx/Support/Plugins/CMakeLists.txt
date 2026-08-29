include(OpenFX)

set(PLUGINS
        Basic
        ChoiceParams
        Field
        Generator
        GPUGain
        Invert
        MultiBundle
        Retimer
        Tester
        Transition)

foreach(PLUGIN IN LISTS PLUGINS)
	file(GLOB_RECURSE PLUGIN_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${PLUGIN}/*.cpp")
	if (${PLUGIN} STREQUAL "GPUGain")
          if(APPLE)
            file(GLOB_RECURSE METAL_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${PLUGIN}/*.mm") # add Metal kernel
            list(APPEND PLUGIN_SOURCES ${METAL_SOURCES})
          endif()
          if (OFX_SUPPORTS_CUDARENDER)
            file(GLOB_RECURSE CUDA_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${PLUGIN}/*.cu")
            list(APPEND PLUGIN_SOURCES ${CUDA_SOURCES})
          endif()
          if (NOT OFX_SUPPORTS_OPENCLRENDER)
            list(FILTER PLUGIN_SOURCES EXCLUDE REGEX "OpenCLKernel")
          endif()
	endif()

	set(TGT example-${PLUGIN}-support)
	add_ofx_plugin(${TGT} ${PLUGIN})
	target_sources(${TGT} PUBLIC ${PLUGIN_SOURCES})
	target_link_libraries(${TGT} ${CONAN_LIBS} OfxSupport opengl::opengl)
	target_include_directories(${TGT} PUBLIC ${OFX_HEADER_DIR} ${OFX_SUPPORT_HEADER_DIR})
        if(APPLE)
          target_link_libraries(${TGT} "-framework Metal" "-framework Foundation" "-framework QuartzCore")
          if (OFX_SUPPORTS_OPENCLRENDER)
            target_link_libraries(${TGT} "-framework OpenCL")
          endif()
        else()
          if (OFX_SUPPORTS_OPENCLRENDER)
            target_link_libraries(${TGT} OpenCL::Headers OpenCL::OpenCL)
          endif()
        endif()


endforeach()
