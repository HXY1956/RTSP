# === Base 静态库 ===
set(file_fold
xml
global
mutex
)

foreach(fold ${file_fold})
    file(GLOB_RECURSE tmp_header
    ${SRC_DIR}/LibBASE/header/${fold}/*.h)
    source_group("Header Files\\${fold}" FILES ${tmp_header})
    list(APPEND BASE_ALL_HEADER ${tmp_header}) 
    list(APPEND BASE_HEADER_DIR  ${SRC_DIR}/LibBASE/header/${fold})
endforeach()

foreach(fold ${file_fold})
    file(GLOB_RECURSE tmp_source
     ${SRC_DIR}/LibBASE/source/${fold}/*.cpp)
    source_group("Source Files\\${fold}" FILES ${tmp_source})
    list(APPEND BASE_ALL_SRC ${tmp_source})
endforeach()

add_library(LibBASE STATIC ${BASE_ALL_SRC})
target_sources(LibBASE
PRIVATE
${BASE_ALL_HEADER}
)
	
target_include_directories(LibBASE PUBLIC
${BASE_HEADER_DIR}
${Third_Eigen_ROOT}
)