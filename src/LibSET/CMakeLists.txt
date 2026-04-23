# === SET 静态库 ===
set(file_fold
base
out
audio
vis
rstp
)

foreach(fold ${file_fold})
    file(GLOB_RECURSE tmp_header
    ${SRC_DIR}/LibSET/header/${fold}/*.h)
    source_group("Header Files\\${fold}" FILES ${tmp_header})
    list(APPEND SET_ALL_HEADER ${tmp_header}) 
    list(APPEND SET_HEADER_DIR  ${SRC_DIR}/LibSET/header/${fold})
endforeach()

foreach(fold ${file_fold})
    file(GLOB_RECURSE tmp_source
     ${SRC_DIR}/LibSET/source/${fold}/*.cpp)
    source_group("Source Files\\${fold}" FILES ${tmp_source})
    list(APPEND SET_ALL_SRC ${tmp_source})
endforeach()

add_library(LibSET STATIC ${SET_ALL_SRC})

target_sources(LibSET
PRIVATE
${SET_ALL_HEADER}
)

target_include_directories(LibSET PUBLIC
${SET_HEADER_DIR}
${Third_Eigen_ROOT}
)
target_link_libraries(LibSET
PUBLIC
LibBASE)
if(MSVC)
    target_compile_definitions(LibSET PUBLIC
    WIN32_LEAN_AND_MEAN   
    NOMINMAX
    )
endif()
