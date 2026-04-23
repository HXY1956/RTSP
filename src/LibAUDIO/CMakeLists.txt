# === AUDIO 静态库 ===
set(file_fold
offline
online
)

foreach(fold ${file_fold})
    file(GLOB_RECURSE tmp_header
    ${SRC_DIR}/LibAUDIO/header/${fold}/*.h)
    source_group("Header Files\\${fold}" FILES ${tmp_header})
    list(APPEND AUDIO_ALL_HEADER ${tmp_header}) 
    list(APPEND AUDIO_HEADER_DIR  ${SRC_DIR}/LibAUDIO/header/${fold})
endforeach()

foreach(fold ${file_fold})
    file(GLOB_RECURSE tmp_source
     ${SRC_DIR}/LibAUDIO/source/${fold}/*.cpp)
    source_group("Source Files\\${fold}" FILES ${tmp_source})
    list(APPEND AUDIO_ALL_SRC ${tmp_source})
endforeach()

add_library(LibAUDIO STATIC ${AUDIO_ALL_SRC})
target_sources(LibAUDIO
    PRIVATE
    ${AUDIO_ALL_HEADER} 
)

target_include_directories(LibAUDIO
  PUBLIC
    ${AUDIO_HEADER_DIR}
    ${FFMPEG_INCLUDE_DIRS}
    ${RNNOISE_INCLUDE_DIRS}
)
target_link_directories(LibAUDIO
    PUBLIC
    ${FFMPEG_LIBRARY_DIRS}
)
target_link_libraries(LibAUDIO
    PUBLIC
    LibBASE
    ${FFMPEG_LIBRARIES}
    ${RNNOISE_LIBRARIES}
)
