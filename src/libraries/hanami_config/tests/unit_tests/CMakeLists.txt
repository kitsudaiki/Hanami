add_executable(hanami_config_unit_tests
    config_handler_test.cpp
    config_handler_test.h
    main.cpp
)

target_include_directories(hanami_config_unit_tests PRIVATE
    ../../include/
    ../../src/
    ../../../../third-party-libs/json/include
)

target_link_libraries(hanami_config_unit_tests PRIVATE
    hanami_config
    hanami_common
)
