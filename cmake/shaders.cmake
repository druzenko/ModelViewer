function(target_add_shaders OUT_SHADERS)
    file(GLOB_RECURSE SHADERS CONFIGURE_DEPENDS
        "ModelViewer/Sources/shaders/*.hlsl"
    )

    foreach(SHADER ${SHADERS})
        if (SHADER MATCHES "VS.hlsl")
            set(SHADER_TYPE Vertex)
        elseif(SHADER MATCHES "PS.hlsl")
            set(SHADER_TYPE Pixel)
        elseif(SHADER MATCHES "CS.hlsl")
            set(SHADER_TYPE Compute)
        endif()

        set_source_files_properties(${SHADER} PROPERTIES
            VS_SHADER_TYPE ${SHADER_TYPE}
            VS_SHADER_MODEL 5.1
            VS_SHADER_ENTRYPOINT main
        )
    endforeach()

    set(${OUT_SHADERS} ${SHADERS} PARENT_SCOPE)
endfunction()