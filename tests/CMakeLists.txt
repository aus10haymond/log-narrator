# Find Catch2 - will use FetchContent if not found
find_package(Catch2 3 QUIET)

if(NOT Catch2_FOUND)
    message(STATUS "Catch2 not found, fetching from GitHub")
    include(FetchContent)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.4.0
    )
    FetchContent_MakeAvailable(Catch2)
endif()

# Common test sources (reusable across test executables)
set(COMMON_SOURCES
    ${PROJECT_SOURCE_DIR}/src/core/source_ref.cpp
    ${PROJECT_SOURCE_DIR}/src/core/severity.cpp
    ${PROJECT_SOURCE_DIR}/src/io/file_reader.cpp
    ${PROJECT_SOURCE_DIR}/src/io/dir_scanner.cpp
    ${PROJECT_SOURCE_DIR}/src/io/stdin_reader.cpp
    ${PROJECT_SOURCE_DIR}/src/io/record_framer.cpp
    ${PROJECT_SOURCE_DIR}/src/io/multiline_framer.cpp
    ${PROJECT_SOURCE_DIR}/src/parsing/timestamp_detector.cpp
    ${PROJECT_SOURCE_DIR}/src/parsing/severity_detector.cpp
    ${PROJECT_SOURCE_DIR}/src/parsing/kv_extractor.cpp
    ${PROJECT_SOURCE_DIR}/src/parsing/event_parser.cpp
    ${PROJECT_SOURCE_DIR}/src/analysis/correlation_extractor.cpp
    ${PROJECT_SOURCE_DIR}/src/analysis/event_index.cpp
    ${PROJECT_SOURCE_DIR}/src/analysis/episode_builder.cpp
    ${PROJECT_SOURCE_DIR}/src/analysis/stats.cpp
    ${PROJECT_SOURCE_DIR}/src/analysis/stats_builder.cpp
    ${PROJECT_SOURCE_DIR}/src/analysis/anomaly_detector.cpp
    ${PROJECT_SOURCE_DIR}/src/rules/rule_registry.cpp
    ${PROJECT_SOURCE_DIR}/src/rules/builtin/crash_loop_rule.cpp
    ${PROJECT_SOURCE_DIR}/src/rules/builtin/retry_to_timeout_rule.cpp
    ${PROJECT_SOURCE_DIR}/src/rules/builtin/error_burst_after_change_rule.cpp
    ${PROJECT_SOURCE_DIR}/src/narrative/narrator.cpp
    ${PROJECT_SOURCE_DIR}/src/narrative/markdown_writer.cpp
    ${PROJECT_SOURCE_DIR}/src/narrative/json_writer.cpp
)

# Test executable
add_executable(unit_tests
    unit/test_dir_scanner.cpp
    unit/test_file_reader.cpp
    unit/test_stdin_reader.cpp
    unit/test_source_ref.cpp
    unit/test_status.cpp
    unit/test_record_framer.cpp
    unit/test_multiline_framer.cpp
    unit/test_severity.cpp
    unit/test_event.cpp
    unit/test_timestamp_detector.cpp
    unit/test_parsing.cpp
    unit/test_analysis.cpp
    unit/test_episode_builder.cpp
    unit/test_stats.cpp
    unit/test_rules.cpp
    unit/test_narrative.cpp
    ${COMMON_SOURCES}
)

target_include_directories(unit_tests
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(unit_tests
    PRIVATE
        Catch2::Catch2WithMain
)

# Add tests to CTest
include(CTest)
include(Catch)
catch_discover_tests(unit_tests)
