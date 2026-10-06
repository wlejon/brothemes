add_library(brothemes
    color.cpp
    color_spaces.cpp
    theme.cpp
    contrast.cpp
    format.cpp
    formats/json_util.cpp
    formats/xml_plist.cpp
    formats/kv_util.cpp
    formats/toml_yaml_util.cpp
    formats/iterm.cpp
    formats/windows_terminal.cpp
    formats/alacritty.cpp
    formats/kitty.cpp
    formats/ghostty.cpp
    formats/base16.cpp
    formats/vscode.cpp
)
add_library(brothemes::brothemes ALIAS brothemes)

target_include_directories(brothemes
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

if(MSVC)
    target_compile_options(brothemes PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(brothemes PRIVATE -Wall -Wextra -Wpedantic)
endif()
