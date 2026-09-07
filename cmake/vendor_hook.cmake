# Redirect the vendor's only generated source-tree header before config.cmake runs.
if(PROJECT_NAME STREQUAL "flash_bin")
  function(configure_file input output)
    if("${output}" MATCHES "/inc/build_info\\.h$")
      _configure_file("${input}" "${CLOUD_CONFIG_DIR}/build_info.h" ${ARGN})
    else()
      _configure_file("${input}" "${output}" ${ARGN})
    endif()
  endfunction()
endif()
if(NOT PROJECT_NAME STREQUAL "app" OR CLOUD_EXAMPLES_HOOK_APPLIED)
  return()
endif()
set(CLOUD_EXAMPLES_HOOK_APPLIED TRUE)
# The existing RTK overlay owns protocol source selection and platform flags.
set(RTK_AMEBA_BUILD_FIRMWARE_EXAMPLE OFF)
set(RTK_AMEBA_VENDOR_LINK_PROBE OFF)
include("${RTK_AMEBA_WEBRTC_ROOT}/platform/amebapro2/rtk_amebapro2_vendor_hook.cmake")
if(CLOUD_EXAMPLE STREQUAL "mqtt")
  list(FILTER out_sources EXCLUDE REGEX "rtk_ameba_(device|cloud|mmf|libdatachannel|libjuice_platform|service|newlib_runtime)\\.(c|cpp)$")
  list(FILTER out_sources EXCLUDE REGEX "rtk_ameba_device_example\\.c$")
  list(REMOVE_ITEM libs LibDataChannel::LibDataChannelStatic)
  list(APPEND out_sources "${CLOUD_EXAMPLES_ROOT}/common/mqtt_identity.c")
endif()
list(APPEND out_sources
  "${CLOUD_EXAMPLES_ROOT}/common/app.c"
  "${CLOUD_EXAMPLES_ROOT}/common/network.c"
  "${CLOUD_EXAMPLES_ROOT}/examples/${CLOUD_EXAMPLE}/main.c"
  "${CLOUD_CONFIG_DIR}/rtk_ameba_firmware_config.c")
list(APPEND app_inc_path "${CLOUD_EXAMPLES_ROOT}/common" "${CLOUD_CONFIG_DIR}"
  "${RTK_AMEBA_WEBRTC_ROOT}/examples/amebapro2_firmware")
list(APPEND libs "-Wl,--undefined=app_example")
if(NOT CLOUD_EXAMPLE STREQUAL "mqtt")
  list(APPEND out_sources "${CLOUD_EXAMPLES_ROOT}/common/video.c")
endif()
if(CLOUD_EXAMPLE STREQUAL "webrtc_test_video")
  list(APPEND out_sources "${CLOUD_EXAMPLES_ROOT}/assets/test_video.c")
  list(APPEND app_inc_path "${CLOUD_EXAMPLES_ROOT}/assets")
elseif(CLOUD_EXAMPLE STREQUAL "webrtc_camera")
  list(APPEND out_sources
    "${RTK_AMEBA_WEBRTC_ROOT}/examples/amebapro2_firmware/rtk_ameba_frame_pool.c"
    "${RTK_AMEBA_WEBRTC_ROOT}/examples/amebapro2_firmware/rtk_ameba_mmf_bridge.c")
  list(APPEND app_inc_path "${prj_root}/src/mmfv2_video_example")
endif()

function(cloud_finalize)
  foreach(target outsrc application.ntz bootfcs bootloader)
    if(TARGET ${target})
      target_compile_options(${target} PRIVATE "$<$<COMPILE_LANGUAGE:C>:-include${CLOUD_CONFIG_DIR}/sensor_selection.h>")
      target_include_directories(${target} BEFORE PRIVATE "${CLOUD_CONFIG_DIR}")
    endif()
  endforeach()
endfunction()
cmake_language(DEFER CALL cloud_finalize)
