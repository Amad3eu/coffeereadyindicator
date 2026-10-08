#pragma once

#include <cstdarg>
#include <cstddef>
#include <cstdint>

namespace coffee {

enum class LogLevel : uint8_t { Debug, Info, Warn, Error };

const char* toString(LogLevel level);

/// Leveled, tagged logger. Formatting and level filtering happen here; adapters
/// only implement `write()` for their output (serial port, stdout, test buffer).
///
/// Output convention (see README "Logging"):
///   [TAG] message            for DEBUG and INFO
///   [WARN][TAG] message      for WARN
///   [ERROR][TAG] message     for ERROR
class Logger {
   public:
    static constexpr size_t kMaxLineLength = 160;

    explicit Logger(LogLevel minimumLevel = LogLevel::Info) : minimumLevel_(minimumLevel) {}
    virtual ~Logger() = default;

    void debug(const char* tag, const char* format, ...) __attribute__((format(printf, 3, 4)));
    void info(const char* tag, const char* format, ...) __attribute__((format(printf, 3, 4)));
    void warn(const char* tag, const char* format, ...) __attribute__((format(printf, 3, 4)));
    void error(const char* tag, const char* format, ...) __attribute__((format(printf, 3, 4)));

    void setMinimumLevel(LogLevel level) { minimumLevel_ = level; }
    LogLevel minimumLevel() const { return minimumLevel_; }
    bool isEnabled(LogLevel level) const { return level >= minimumLevel_; }

   protected:
    /// Receives one fully formatted line (without trailing newline).
    virtual void write(LogLevel level, const char* tag, const char* message) = 0;

   private:
    void log(LogLevel level, const char* tag, const char* format, va_list args);

    LogLevel minimumLevel_;
};

}  // namespace coffee
