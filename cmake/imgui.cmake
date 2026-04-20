include(FetchContent)

function(target_add_imgui target_name)
    if(NOT TARGET imgui)
        FetchContent_Declare(
            imgui
            GIT_REPOSITORY https://github.com/ocornut/imgui.git
            GIT_TAG        v1.92.7
        )

        FetchContent_MakeAvailable(imgui)

        add_library(imgui STATIC
            ${imgui_SOURCE_DIR}/imgui.cpp
            ${imgui_SOURCE_DIR}/imgui_demo.cpp
            ${imgui_SOURCE_DIR}/imgui_draw.cpp
            ${imgui_SOURCE_DIR}/imgui_tables.cpp
            ${imgui_SOURCE_DIR}/imgui_widgets.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_dx12.cpp
        )

        target_include_directories(imgui PUBLIC
            ${imgui_SOURCE_DIR}
            ${imgui_SOURCE_DIR}/backends
        )

        target_link_libraries(imgui PUBLIC
            d3d12.lib
            dxgi.lib
        )
    endif()

    target_link_libraries(${target_name} PRIVATE imgui)
endfunction()
