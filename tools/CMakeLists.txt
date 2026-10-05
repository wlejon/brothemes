# bpk: the archive codec the iTerm2-Color-Schemes oracle data ships in
# (tools/pack/bpk.h). Used by the tests and by brothemes_pack, which rebuilds
# tests/data/iterm2-color-schemes.bpk from an upstream checkout.
add_library(brothemes_bpk STATIC
    pack/bpk_encode.cpp
    pack/bpk_decode.cpp
)
target_include_directories(brothemes_bpk PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
if(MSVC)
    target_compile_options(brothemes_bpk PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(brothemes_bpk PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(BROTHEMES_BUILD_TOOLS)
    add_executable(brothemes_pack pack/pack_schemes.cpp)
    target_link_libraries(brothemes_pack PRIVATE brothemes_bpk)
    if(MSVC)
        target_compile_options(brothemes_pack PRIVATE /W4 /permissive- /utf-8)
    else()
        target_compile_options(brothemes_pack PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()
