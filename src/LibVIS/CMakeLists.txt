# === VIS 静态库 ===
set(file_fold
online
offline
)

foreach(fold ${file_fold})
    file(GLOB_RECURSE tmp_header
    ${SRC_DIR}/LibVIS/header/${fold}/*.h)
    source_group("Header Files\\${fold}" FILES ${tmp_header})
    list(APPEND VIS_ALL_HEADER ${tmp_header}) 
    list(APPEND VIS_HEADER_DIR  ${SRC_DIR}/LibVIS/header/${fold})
endforeach()

foreach(fold ${file_fold})
    file(GLOB_RECURSE tmp_source
     ${SRC_DIR}/LibVIS/source/${fold}/*.cpp)
    source_group("Source Files\\${fold}" FILES ${tmp_source})
    list(APPEND VIS_ALL_SRC ${tmp_source})
endforeach()

add_library(LibVIS STATIC ${VIS_ALL_SRC})
target_sources(LibVIS
    PRIVATE
    ${VIS_ALL_HEADER} 
)

target_include_directories(LibVIS
    PUBLIC
    ${VIS_HEADER_DIR}
    ${OpenCV_INCLUDE_DIRS}
    ${GSTREAMER_INCLUDE_DIRS}
    ${GSTREAMER_APP_INCLUDE_DIRS}
    ${GSTREAMER_VIDEO_INCLUDE_DIRS}
    ${SRC_DIR}/LibVIS/header/HKincludes
)
target_link_directories(LibVIS PUBLIC
    ${GSTREAMER_LIBRARY_DIRS}
    ${GSTREAMER_APP_LIBRARY_DIRS}
    ${GSTREAMER_VIDEO_LIBRARY_DIRS}
)
target_link_libraries(LibVIS PUBLIC
    LibBASE
    ${OpenCV_LIBS}
    ${GSTREAMER_LIBRARIES}
    ${GSTREAMER_APP_LIBRARIES}
    ${GSTREAMER_VIDEO_LIBRARIES}
    ${SRC_DIR}/LibVIS/header/HKincludes/libMvCameraControl.so.4.7.0.3
)
