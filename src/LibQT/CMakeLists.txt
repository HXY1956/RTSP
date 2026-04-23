# === Qt 静态库 ===
set(file_fold
vis
audio
main
)

set(QT_MAIN ${SRC_DIR}/LibQT/source/main/qt_main.cpp)
foreach(fold ${file_fold})
    file(GLOB_RECURSE tmp_header
    ${SRC_DIR}/LibQT/header/${fold}/*.h)
    source_group("Header Files\\${fold}" FILES ${tmp_header})
    list(APPEND QT_ALL_HEADER ${tmp_header}) 
    list(APPEND QT_HEADER_DIR  ${SRC_DIR}/LibQT/header/${fold})
endforeach()

foreach(fold ${file_fold})
    file(GLOB_RECURSE tmp_source
     ${SRC_DIR}/LibQT/source/${fold}/*.cpp)
    source_group("Source Files\\${fold}" FILES ${tmp_source})
    list(APPEND QT_ALL_SRC ${tmp_source})
endforeach()
list(REMOVE_ITEM QT_ALL_SRC ${QT_MAIN}) 

add_library(LibQT STATIC ${QT_ALL_SRC})
target_sources(LibQT
    PRIVATE
    ${QT_ALL_HEADER} 
)

target_link_libraries(LibQT
PUBLIC
LibVIS
LibAUDIO
LibRTSP
LibSMOKE
Qt5::Widgets 
Qt5::Core 
Qt5::Gui
Threads::Threads
)
target_include_directories(LibQT 
PUBLIC
${QT_HEADER_DIR} 
)
set_target_properties(LibQT PROPERTIES
AUTOMOC ON         
AUTORCC ON
AUTOUIC ON
)
if(MSVC)
    target_compile_definitions(LibQT PRIVATE
      WIN32_LEAN_AND_MEAN   
      NOMINMAX
    )
    target_compile_options(LibQT PRIVATE /bigobj)
endif()

add_executable(AppQT ${QT_MAIN})
target_link_libraries(AppQT
LibQT
)
