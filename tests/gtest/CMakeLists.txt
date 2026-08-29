add_executable(olive-gtest
  main.cpp
  common_current_test.cpp
  common_xmlutils_test.cpp
  common_range_test.cpp
  common_filefunctions_test.cpp
  common_qtutils_test.cpp
  common_commandlineparser_test.cpp
  common_debug_test.cpp
  common_decibel_test.cpp
  common_digit_test.cpp
  common_jobtime_test.cpp
  common_ffmpegutils_test.cpp
  ffmpeg_bridge_test.cpp
  config_test.cpp
  audio_level_meter_test.cpp
  audio_synchronizer_test.cpp
  audio_waveform_sync_test.cpp
  color_lut_test.cpp
  node_value_test.cpp
  node_keyframe_test.cpp
  node_serialization_test.cpp
  node_globals_test.cpp
  node_project_test.cpp
  render_videoparams_test.cpp
  core_stringutils_test.cpp
  core_timecode_test.cpp
  core_timerange_test.cpp
  core_bezier_test.cpp
  core_color_test.cpp
  core_rational_test.cpp
  core_samplebuffer_test.cpp
  render_videoparams_branch_test.cpp
  render_audioparams_test.cpp
  render_audioparams_branch_test.cpp
  render_sampleformat_test.cpp
  render_pixelformat_test.cpp
  render_loopmode_test.cpp
  render_alphaassoc_test.cpp
  render_ipc_test.cpp
  render_ticket_test.cpp
  render_worker_footage_test.cpp
  render_direct_connection_test.cpp
  render_clip_buffer_hint_test.cpp
  viewer_display_repro_test.cpp
  project_serializer_test.cpp
  proxy_manager_test.cpp
  proxy_dialog_test.cpp
  lut_file_field_test.cpp
  node_math_test.cpp
  node_undo_test.cpp
  track_test.cpp
  render_diskcache_test.cpp
  node_group_test.cpp
  plugin_paraminstance_test.cpp
  audio_waveform_test.cpp
  node_value_extended_test.cpp
  render_projectcopier_test.cpp
  node_core_test.cpp
  clip_traverser_test.cpp
  audio_manager_viewer_test.cpp
  footage_test.cpp
  render_workerpool_ipc_test.cpp
  node_inputimmediate_test.cpp
  project_factory_test.cpp
  render_processor_test.cpp
  node_time_test.cpp
  node_color_test.cpp
  plugin_node_test.cpp
  sequence_test.cpp
  node_generator_test.cpp
  render_misc_test.cpp
  multicam_serializer_test.cpp
  node_audio_test.cpp
  node_distort_test.cpp
  node_filter_keying_test.cpp
  node_math_transition_test.cpp
  node_save_load_test.cpp
  node_polygon_folder_test.cpp
  footage_probe_test.cpp
  render_tail_test.cpp
  timeline_marker_test.cpp
  undo_stack_test.cpp
  plugin_support_test.cpp
  plugin_support_image_test.cpp
  plugin_support_clip_test.cpp
  plugin_support_param_test.cpp
  plugin_smoke_test.cpp
  plugin_ofx_misc_test.cpp
  plugin_format_conversion_test.cpp
  audio_smoke_test.cpp
  viewer_smoke_test.cpp
  dynamic_render_backend_test.cpp
  opengl_readback_guard_test.cpp
  plugin_render_pipeline_test.cpp
  plugin_renderer_readback_test.cpp
  plugin_ofx_integration_test.cpp
  preferences_behavior_tab_test.cpp
  node_view_test.cpp
  preview_autocacher_test.cpp
  codec_frame_test.cpp
  codec_decoder_test.cpp
  ffmpeg_decoder_hw_test.cpp
  codec_exportcodec_test.cpp
  codec_exportformat_test.cpp
  codec_encoder_test.cpp
  task_taskmanager_test.cpp
  module_smoke_test.cpp
  shader_resources_test.cpp
  timecode_metadata_test.cpp
  timebased_widget_test.cpp
  timeline_coordinate_test.cpp
  timeline_waveform_sync_test.cpp
  timeline_workarea_test.cpp
  common_html_test.cpp
  common_oiioutils_test.cpp
  ui_humanstrings_test.cpp
  cli_test.cpp
  timeline_undo_test.cpp
  timeline_undo_general_test.cpp
  task_project_test.cpp
  task_cache_test.cpp
  codec_oiio_test.cpp
  codec_planarfiledevice_test.cpp
  codec_ffmpegencoder_test.cpp
  ui_icons_test.cpp
  ui_style_test.cpp
  dialog_editing_test.cpp
  dialog_misc_test.cpp
  dialog_export_test.cpp
  widget_slider_test.cpp
  widget_layout_test.cpp
  widget_combos_test.cpp
  widget_misc_test.cpp
  widget_curve_keyframe_test.cpp
  widget_timeruler_playback_test.cpp
  widget_projectexplorer_test.cpp
  widget_panels_model_test.cpp
  panel_test.cpp
  mainwindow_test.cpp
  common_ratiodialog_test.cpp
)

find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Test)

target_sources(olive-gtest PRIVATE $<TARGET_OBJECTS:libolive-editor> $<TARGET_OBJECTS:olive-version-obj>)

target_include_directories(
  olive-gtest
  PRIVATE
  ${CMAKE_SOURCE_DIR}/app
  ${CMAKE_SOURCE_DIR}/tests
  ${CMAKE_SOURCE_DIR}/third_party/openfx/include
  ${CMAKE_SOURCE_DIR}/third_party/openfx/HostSupport/include
  ${OLIVE_INCLUDE_DIRS}
)

target_link_libraries(
  olive-gtest
  PRIVATE
  ${OLIVE_LIBRARIES}
  GTest::gtest
  Qt${QT_VERSION_MAJOR}::Test
)

target_compile_definitions(
  olive-gtest
  PRIVATE
  ${OLIVE_DEFINITIONS}
  OAK_TEST_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

target_compile_options(
  olive-gtest
  PRIVATE
  ${OLIVE_COMPILE_OPTIONS}
)

if (OAK_ENABLE_DYNAMIC_RENDER_BACKEND)
  target_compile_definitions(olive-gtest PRIVATE OAK_ENABLE_DYNAMIC_RENDER_BACKEND)
  add_dependencies(olive-gtest oakgl)
  if (TARGET oakvulkan)
    add_dependencies(olive-gtest oakvulkan)
  endif()
endif()

if (MSVC)
  add_test("Olive.gtest" olive-gtest)
else()
  add_test(olive-gtest olive-gtest)
endif()

if (WIN32)
  # Windows has no RPATH: ffmpeg_bridge.dll must sit next to the executable
  add_custom_command(TARGET olive-gtest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      $<TARGET_FILE:ffmpeg_bridge> $<TARGET_FILE_DIR:olive-gtest>)
endif()
