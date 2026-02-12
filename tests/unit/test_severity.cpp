#include <catch2/catch_test_macros.hpp>
#include "logstory/core/severity.hpp"

using namespace logstory::core;

TEST_CASE("Severity to_string conversion", "[severity]") {
    REQUIRE(to_string(Severity::UNKNOWN) == "UNKNOWN");
    REQUIRE(to_string(Severity::TRACE) == "TRACE");
    REQUIRE(to_string(Severity::DEBUG) == "DEBUG");
    REQUIRE(to_string(Severity::INFO) == "INFO");
    REQUIRE(to_string(Severity::WARN) == "WARN");
    REQUIRE(to_string(Severity::ERROR) == "ERROR");
    REQUIRE(to_string(Severity::FATAL) == "FATAL");
}

TEST_CASE("Severity from_string conversion - exact match", "[severity]") {
    REQUIRE(severity_from_string("TRACE") == Severity::TRACE);
    REQUIRE(severity_from_string("DEBUG") == Severity::DEBUG);
    REQUIRE(severity_from_string("INFO") == Severity::INFO);
    REQUIRE(severity_from_string("WARN") == Severity::WARN);
    REQUIRE(severity_from_string("ERROR") == Severity::ERROR);
    REQUIRE(severity_from_string("FATAL") == Severity::FATAL);
}

TEST_CASE("Severity from_string conversion - case insensitive", "[severity]") {
    REQUIRE(severity_from_string("trace") == Severity::TRACE);
    REQUIRE(severity_from_string("Debug") == Severity::DEBUG);
    REQUIRE(severity_from_string("info") == Severity::INFO);
    REQUIRE(severity_from_string("WARN") == Severity::WARN);
    REQUIRE(severity_from_string("error") == Severity::ERROR);
    REQUIRE(severity_from_string("Fatal") == Severity::FATAL);
}

TEST_CASE("Severity from_string conversion - aliases", "[severity]") {
    REQUIRE(severity_from_string("VERBOSE") == Severity::TRACE);
    REQUIRE(severity_from_string("DBG") == Severity::DEBUG);
    REQUIRE(severity_from_string("INFORMATION") == Severity::INFO);
    REQUIRE(severity_from_string("WARNING") == Severity::WARN);
    REQUIRE(severity_from_string("ERR") == Severity::ERROR);
    REQUIRE(severity_from_string("CRITICAL") == Severity::FATAL);
    REQUIRE(severity_from_string("SEVERE") == Severity::FATAL);
}

TEST_CASE("Severity from_string handles unknown", "[severity]") {
    REQUIRE(severity_from_string("") == Severity::UNKNOWN);
    REQUIRE(severity_from_string("INVALID") == Severity::UNKNOWN);
    REQUIRE(severity_from_string("XYZ") == Severity::UNKNOWN);
}

TEST_CASE("Severity round-trip conversion", "[severity]") {
    for (auto sev : {Severity::TRACE, Severity::DEBUG, Severity::INFO,
                     Severity::WARN, Severity::ERROR, Severity::FATAL}) {
        std::string str = to_string(sev);
        Severity result = severity_from_string(str);
        REQUIRE(result == sev);
    }
}
