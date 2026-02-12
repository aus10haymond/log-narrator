#include <catch2/catch_test_macros.hpp>
#include "logstory/core/error.hpp"

using namespace logstory::core;

TEST_CASE("Status indicates success correctly", "[status]") {
    Status s;
    REQUIRE(s.ok());
    REQUIRE(s);
    REQUIRE(s.code == ErrorCode::OK);
    REQUIRE(s.message.empty());
}

TEST_CASE("Status::OK() creates successful status", "[status]") {
    Status s = Status::OK();
    REQUIRE(s.ok());
    REQUIRE(s);
    REQUIRE(s.code == ErrorCode::OK);
}

TEST_CASE("Status indicates FILE_NOT_FOUND error", "[status]") {
    Status s(ErrorCode::FILE_NOT_FOUND, "File does not exist");
    REQUIRE_FALSE(s.ok());
    REQUIRE_FALSE(s);
    REQUIRE(s.code == ErrorCode::FILE_NOT_FOUND);
    REQUIRE(s.message == "File does not exist");
}

TEST_CASE("Status indicates FILE_UNREADABLE error", "[status]") {
    Status s(ErrorCode::FILE_UNREADABLE, "Cannot read file");
    REQUIRE_FALSE(s.ok());
    REQUIRE(s.code == ErrorCode::FILE_UNREADABLE);
    REQUIRE(s.message == "Cannot read file");
}

TEST_CASE("Status indicates DIRECTORY_NOT_FOUND error", "[status]") {
    Status s(ErrorCode::DIRECTORY_NOT_FOUND, "Directory missing");
    REQUIRE_FALSE(s.ok());
    REQUIRE(s.code == ErrorCode::DIRECTORY_NOT_FOUND);
    REQUIRE(s.message == "Directory missing");
}

TEST_CASE("Status indicates DIRECTORY_EMPTY error", "[status]") {
    Status s(ErrorCode::DIRECTORY_EMPTY, "No files found");
    REQUIRE_FALSE(s.ok());
    REQUIRE(s.code == ErrorCode::DIRECTORY_EMPTY);
    REQUIRE(s.message == "No files found");
}

TEST_CASE("Status indicates INVALID_INPUT error", "[status]") {
    Status s(ErrorCode::INVALID_INPUT, "Invalid input provided");
    REQUIRE_FALSE(s.ok());
    REQUIRE(s.code == ErrorCode::INVALID_INPUT);
    REQUIRE(s.message == "Invalid input provided");
}

TEST_CASE("Status indicates UNKNOWN_ERROR", "[status]") {
    Status s(ErrorCode::UNKNOWN_ERROR, "Something went wrong");
    REQUIRE_FALSE(s.ok());
    REQUIRE(s.code == ErrorCode::UNKNOWN_ERROR);
    REQUIRE(s.message == "Something went wrong");
}

TEST_CASE("Status can be created with error code only", "[status]") {
    Status s(ErrorCode::FILE_NOT_FOUND);
    REQUIRE_FALSE(s.ok());
    REQUIRE(s.code == ErrorCode::FILE_NOT_FOUND);
    REQUIRE(s.message.empty());
}
