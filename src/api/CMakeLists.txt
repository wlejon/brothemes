add_library(brothemes_api STATIC
    api.cpp
    host_class.cpp
    themes_convert.cpp
    native_themes.cpp
    native_themes_contrast.cpp
    native_themes_format.cpp
    native_themes_color.cpp
)

target_include_directories(brothemes_api PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
)

target_link_libraries(brothemes_api PUBLIC
    brothemes
    bronze_runtime_shared
)

target_compile_features(brothemes_api PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(brothemes_api PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(brothemes_api PRIVATE -Wall -Wextra)
endif()
