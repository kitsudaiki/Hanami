qt_add_executable(unit_tests
    main.cpp
)
target_include_directories(unit_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ../../../../third-party-libs/json/include
    ../../../hanami_common/include
    ../../include
    ../../src
)

target_link_libraries(unit_tests PRIVATE
    # Remove: L../../../hanami_common/src
    # Remove: L../../../hanami_common/src/debug
    # Remove: L../../../hanami_common/src/release
    # Remove: L../../src
    # Remove: qt
    hanami_common
    hanami_files
    uuid
)

install(TARGETS unit_tests
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_generate_deploy_app_script(
    TARGET unit_tests
    FILENAME_VARIABLE deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
