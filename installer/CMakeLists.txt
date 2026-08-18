cmake_minimum_required(VERSION 3.20)

if(WIN32)
    set(INSTALLER_RC ${CMAKE_CURRENT_SOURCE_DIR}/resource.rc)
    set_source_files_properties(${INSTALLER_RC} PROPERTIES
        COMPILE_FLAGS "--include-dir=${CMAKE_CURRENT_SOURCE_DIR}"
    )

    add_executable(nova_installer WIN32
        src/main.cpp
        ${INSTALLER_RC}
    )

    set_target_properties(nova_installer PROPERTIES
        OUTPUT_NAME "Install"
    )

    target_compile_features(nova_installer PRIVATE cxx_std_20)
    target_link_libraries(nova_installer PRIVATE
        comctl32
        shlwapi
        ole32
        shell32
        uuid
    )

    # Automatically copy Install.exe to repository root after build
    add_custom_command(TARGET nova_installer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:nova_installer>
            "${CMAKE_SOURCE_DIR}/Install.exe"
        COMMENT "Copying Install.exe to repository root"
    )
endif()
