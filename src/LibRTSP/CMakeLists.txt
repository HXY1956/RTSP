# === RTSP 静态库 ===
set(file_fold
online
offline
)

foreach(fold ${file_fold})
    file(GLOB_RECURSE tmp_header
    ${SRC_DIR}/LibRTSP/header/${fold}/*.h)
    source_group("Header Files\\${fold}" FILES ${tmp_header})
    list(APPEND RTSP_ALL_HEADER ${tmp_header}) 
    list(APPEND RTSP_HEADER_DIR  ${SRC_DIR}/LibRTSP/header/${fold})
endforeach()

foreach(fold ${file_fold})
    file(GLOB_RECURSE tmp_source
     ${SRC_DIR}/LibRTSP/source/${fold}/*.cpp)
    source_group("Source Files\\${fold}" FILES ${tmp_source})
    list(APPEND RTSP_ALL_SRC ${tmp_source})
endforeach()

add_library(LibRTSP STATIC ${RTSP_ALL_SRC})
target_sources(LibRTSP
    PRIVATE
    ${RTSP_ALL_HEADER} 
)

target_include_directories(LibRTSP
  PUBLIC
  ${RTSP_HEADER_DIR}
  ${OpenCV_INCLUDE_DIRS}
  ${GSTREAMER_RTSP_INCLUDE_DIRS}
  ${GSTREAMER_INCLUDE_DIRS}
  ${GSTREAMER_APP_INCLUDE_DIRS}
)
target_link_directories(LibRTSP 
    PUBLIC
    ${GSTREAMER_RTSP_LIBRARY_DIRS}
    ${GSTREAMER_APP_LIBRARY_DIRS}
    ${GSTREAMER_LIBRARY_DIRS}
)
target_link_libraries(LibRTSP
  PUBLIC
  LibBASE
  ${OpenCV_LIBS}
  ${GSTREAMER_RTSP_LIBRARIES}
  ${GSTREAMER_APP_LIBRARIES}
  ${GSTREAMER_LIBRARIES}
)