# === SMOKE 静态库 ===
set(file_fold
boxfilter
algo
util
)

foreach(fold ${file_fold})
    file(GLOB_RECURSE tmp_header
    ${SRC_DIR}/LibSMOKE/header/${fold}/*.h
    ${SRC_DIR}/LibSMOKE/header/${fold}/*.hpp)
    source_group("Header Files\\${fold}" FILES ${tmp_header})
    list(APPEND SMOKE_ALL_HEADER ${tmp_header}) 
    list(APPEND SMOKE_HEADER_DIR  ${SRC_DIR}/LibSMOKE/header/${fold})
endforeach()

foreach(fold ${file_fold})
    file(GLOB_RECURSE tmp_source
     ${SRC_DIR}/LibSMOKE/source/${fold}/*.cpp
     ${SRC_DIR}/LibSMOKE/source/${fold}/*.cu)
    source_group("Source Files\\${fold}" FILES ${tmp_source})
    list(APPEND SMOKE_ALL_SRC ${tmp_source})
endforeach()

add_library(LibSMOKE STATIC ${SMOKE_ALL_SRC})

find_package(CUDAToolkit REQUIRED)
target_compile_definitions(LibSMOKE PRIVATE 
EIGEN_HAS_CUDA_FP16=0
NOMINMAX)

target_sources(LibSMOKE
    PRIVATE
    ${SMOKE_ALL_HEADER} 
)
target_include_directories(LibSMOKE
    PUBLIC
    ${SMOKE_HEADER_DIR}
    ${OpenCV_INCLUDE_DIRS}
    ${CUDAToolkit_INCLUDE_DIRS}
    ${CUDAToolkit_INCLUDE_DIRS}/crt
)
target_link_libraries(LibSMOKE PUBLIC
    LibBASE
    CUDA::cudart
    ${OpenCV_LIBS}
)
set_target_properties(LibSMOKE PROPERTIES
   CUDA_SEPARABLE_COMPILATION ON
   CUDA_RESOLVE_DEVICE_SYMBOLS ON
   LINKER_LANGUAGE CUDA
)
