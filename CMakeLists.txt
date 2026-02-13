cmake_minimum_required(VERSION 4.2.2)

project(log_narrator
    VERSION 0.1.0
    DESCRIPTION "Log Narrative and Failure Analysis Engine"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(log_narrator
    src/main.cpp
    src/core/source_ref.cpp
    src/core/severity.cpp
    src/io/file_reader.cpp
    src/io/dir_scanner.cpp
    src/io/stdin_reader.cpp
    src/io/record_framer.cpp
    src/io/multiline_framer.cpp
    src/parsing/timestamp_detector.cpp
    src/parsing/severity_detector.cpp
    src/parsing/kv_extractor.cpp
    src/parsing/event_parser.cpp
    src/analysis/correlation_extractor.cpp
    src/analysis/event_index.cpp
    src/analysis/episode_builder.cpp
)

target_include_directories(log_narrator
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

# Enable testing
enable_testing()
add_subdirectory(tests)
