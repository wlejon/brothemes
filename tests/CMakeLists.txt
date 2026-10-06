function(bro_add_test name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE brothemes brothemes_bpk)
    target_include_directories(${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_compile_definitions(${name} PRIVATE BROTHEMES_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    if(MSVC)
        target_compile_options(${name} PRIVATE /W4 /permissive- /utf-8)
    else()
        target_compile_options(${name} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME ${name} COMMAND ${name})
endfunction()

bro_add_test(test_color)
bro_add_test(test_color_spaces)
bro_add_test(test_contrast)
bro_add_test(test_adjustment)
bro_add_test(test_formats)
bro_add_test(test_matrix)
bro_add_test(test_schemes_oracle)
bro_add_test(test_pack)

if(BROTHEMES_ENABLE_API)
    add_executable(brothemes_test_api
        test_themes_api.cpp
    )
    target_include_directories(brothemes_test_api PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/api
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
    target_link_libraries(brothemes_test_api PRIVATE
        brothemes_api
        brothemes
        bronze_runtime_shared
    )
    target_compile_definitions(brothemes_test_api PRIVATE
        BROTHEMES_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    )
    if(MSVC)
        target_compile_options(brothemes_test_api PRIVATE /W4 /permissive- /utf-8)
    else()
        target_compile_options(brothemes_test_api PRIVATE -Wall -Wextra)
    endif()

    if(WIN32)
        add_custom_command(TARGET brothemes_test_api POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:bronze_runtime_shared>
                $<TARGET_FILE_DIR:brothemes_test_api>
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_LINKER_FILE:bronze_runtime_shared>
                $<TARGET_FILE_DIR:brothemes_test_api>
        )
    endif()

    if(NOT BRASS_HOST_BACKEND)
        message(STATUS "brothemes: brothemes_test_api not registered (brass has no code generator for this target)")
    else()
        add_test(NAME brothemes_test_api COMMAND brothemes_test_api)
        add_test(NAME brothemes_test_api_gc_stress COMMAND brothemes_test_api)
        set_tests_properties(brothemes_test_api_gc_stress PROPERTIES
            ENVIRONMENT "BRONZE_GC_STRESS=1;BRONZE_GC_POISON=1")
        if(WIN32)
            set_tests_properties(brothemes_test_api PROPERTIES
                ENVIRONMENT "PATH=$<TARGET_FILE_DIR:bronze_runtime_shared>;$<TARGET_FILE_DIR:brothemes_test_api>;$ENV{PATH}")
            set_property(TEST brothemes_test_api_gc_stress APPEND PROPERTY
                ENVIRONMENT "PATH=$<TARGET_FILE_DIR:bronze_runtime_shared>;$<TARGET_FILE_DIR:brothemes_test_api>;$ENV{PATH}")
        endif()
    endif()
endif()

# Every test executable links no_dialogs.cpp (see the file).
if(WIN32)
    get_property(_test_targets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
    foreach(_t IN LISTS _test_targets)
        get_target_property(_type ${_t} TYPE)
        if(_type STREQUAL "EXECUTABLE")
            target_sources(${_t} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/no_dialogs.cpp)
        endif()
    endforeach()
endif()
