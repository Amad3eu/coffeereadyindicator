#pragma once

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "hardware/Logger.h"

namespace coffee {
namespace test {

/// Captures log lines so tests can assert on them. Set COFFEE_TEST_VERBOSE=1
/// in the environment to also echo them to stdout.
class MockLogger : public Logger {
   public:
    explicit MockLogger(LogLevel minimumLevel = LogLevel::Debug)
        : Logger(minimumLevel), echo_(std::getenv("COFFEE_TEST_VERBOSE") != nullptr) {}

    bool contains(const std::string& fragment) const {
        for (const std::string& line : lines) {
            if (line.find(fragment) != std::string::npos) {
                return true;
            }
        }
        return false;
    }

    size_t count(const std::string& fragment) const {
        size_t n = 0;
        for (const std::string& line : lines) {
            if (line.find(fragment) != std::string::npos) {
                ++n;
            }
        }
        return n;
    }

    std::vector<std::string> lines;

   protected:
    void write(LogLevel level, const char* tag, const char* message) override {
        std::string line;
        if (level >= LogLevel::Warn) {
            line += "[";
            line += toString(level);
            line += "]";
        }
        line += "[";
        line += tag;
        line += "] ";
        line += message;
        lines.push_back(line);
        if (echo_) {
            std::printf("%s\n", line.c_str());
        }
    }

   private:
    bool echo_;
};

}  // namespace test
}  // namespace coffee
