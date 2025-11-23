#ifndef CARDIAC_LOGGER_H_
#define CARDIAC_LOGGER_H_

#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <ctime>
#include <mutex>
#include <atomic>
#include <chrono>

namespace logger {
  enum class Level : int {
    kCritical = 0,
    kError    = 1,
    kWarn     = 2,
    kSuccess  = 3,
    kInfo     = 4,
    kDebug    = 5
  };

  namespace config {
    constexpr size_t kMaxLogLength = 256;     // 256 bytes
    constexpr size_t kMaxFilenameLength = 64; // 64 bytes
    constexpr Level kDefaultLevel = Level::kInfo;
  }

  class Logger {
    private:
    // Settings
    std::atomic<Level> current_level_{config::kDefaultLevel};
    char log_filename_[config::kMaxFilenameLength];
    std::atomic<bool> initialized_{false};
    std::atomic<bool> console_output_{true};
      
    // Thread synchronization - only for file access
    std::mutex file_mutex_;
    std::mutex console_mutex_;
      
    Logger() {
      strcpy(log_filename_, "cardiac_monitor.log");
    }
      
    uint64_t get_timestamp_us() {
      auto now = std::chrono::high_resolution_clock::now();
      auto duration = now.time_since_epoch();
      return std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
    }
      
    void format_timestamp(uint64_t timestamp_us, char* buffer, size_t buffer_size) {
      time_t seconds = timestamp_us / 1000000;
      int microseconds = timestamp_us % 1000000;

      struct tm tm_info;
      localtime_r(&seconds, &tm_info);

      int len = std::strftime(buffer, buffer_size, "%Y-%m-%d %H:%M:%S", &tm_info);
      if (len > 0 && (size_t)len < buffer_size) {
        std::snprintf(buffer + len, buffer_size - len, ".%06d", microseconds);
      }
    }

    const char* level_to_string(Level level) {
      switch (level) {
        case Level::kCritical: return "CRIT";
        case Level::kError:    return "ERR";
        case Level::kWarn:     return "WARN";
        case Level::kSuccess:  return "SCSS";
        case Level::kInfo:     return "INFO";
        case Level::kDebug:    return "DBG ";
        default:               return "UNK ";
      }
    }

    // Get ANSI color codes for console output
    const char* level_to_color(Level level) {
      switch (level) {
        case Level::kCritical: return "\033[1;35m";  // Bright Magenta
        case Level::kError:    return "\033[1;31m";  // Bright Red
        case Level::kWarn:     return "\033[1;33m";  // Bright Yellow
        case Level::kSuccess:  return "\033[1;32m";  // Bright Green
        case Level::kInfo:     return "\033[1;37m";  // Bright White
        case Level::kDebug:    return "\033[1;36m";  // Bright Cyan
        default:               return "\033[0m";     // Reset
      }
    }

    public:
    // Singleton instance getter
    static Logger& instance() {
      static Logger logger;
      return logger;
    }
      
    // Delete construction and assignment operators (singleton pattern)
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
      
    bool init(const char* filename = nullptr, Level level = config::kDefaultLevel) {
      if (initialized_.load()) {
        return true;
      }

      if (filename) {
        strncpy(log_filename_, filename, config::kMaxFilenameLength - 1);
        log_filename_[config::kMaxFilenameLength - 1] = '\0';
      }
          
      current_level_.store(level);
      initialized_.store(true);
            
      log_internal(Level::kInfo,
                    "Logger initialized: file=%s, level=%d",
                    log_filename_,
                    static_cast<int>(level));
            
      return true;
    }
      
    void shutdown() {
      if (!initialized_.load()) {
        return;
      }

      log_internal(Level::kInfo, "Cardiac Logger shutting down...");
      initialized_.store(false);
    }
      
    ~Logger() {
      shutdown();
    }
      
    void set_level(Level level) {
      current_level_.store(level);
    }
      
    Level get_level() const {
      return current_level_.load();
    }
      
    bool is_level_enabled(Level level) const {
      return static_cast<int>(level) <= static_cast<int>(current_level_.load());
    }

    void enable_console_output(bool enable) {
      console_output_.store(enable);
    }

    bool is_console_output_enabled() const {
      return console_output_.load();
    }
      
    void log_internal(Level level, const char* format, ...) {
      if (!is_level_enabled(level) || !initialized_.load()) {
        return;
      }

      char message[config::kMaxLogLength];
      va_list args;
      va_start(args, format);
      vsnprintf(message, config::kMaxLogLength - 1, format, args);
      va_end(args);
      message[config::kMaxLogLength - 1] = '\0';

      uint64_t timestamp_us = get_timestamp_us();
      char timestamp_str[32];
      format_timestamp(timestamp_us, timestamp_str, sizeof(timestamp_str));

      // Console output (immediate)
      if (console_output_.load()) {
        std::lock_guard<std::mutex> console_lock(console_mutex_);
        printf("%s[%s] %s: %s\033[0m\n",
               level_to_color(level),
               timestamp_str,
               level_to_string(level),
               message);
        fflush(stdout);
      }

      // File output (immediate)
      {
        std::lock_guard<std::mutex> file_lock(file_mutex_);
        FILE* file = fopen(log_filename_, "a");
        if (file) {
          fprintf(file, "[%s] %s: %s\n",
                  timestamp_str, 
                  level_to_string(level),
                  message);
          fflush(file);
          fclose(file);
        }
      }
    }

    // Force flush method (now does nothing since we write immediately)
    void force_flush() {
      // No-op since we write immediately to file
    }
  };

  // Convenience functions to facilitate logging
  inline bool init(const char* filename = nullptr,
                  Level level = config::kDefaultLevel) {
    return Logger::instance().init(filename, level);
  }

  inline void shutdown() {
    Logger::instance().shutdown();
  }

  inline void set_level(Level level) {
    Logger::instance().set_level(level);
  }

  inline Level get_level() {
    return Logger::instance().get_level();
  }

  inline void force_flush() {
    Logger::instance().force_flush();
  }

  inline void enable_console_output(bool enable) {
    Logger::instance().enable_console_output(enable);
  }

  inline bool is_console_output_enabled() {
    return Logger::instance().is_console_output_enabled();
  }

  // Main logging functions
  inline void log_kCritical(const char* format, ...) {
    if (!Logger::instance().is_level_enabled(Level::kCritical)) {
      return;
    }
    va_list args;
    va_start(args, format);
    char buffer[config::kMaxLogLength];
    vsnprintf(buffer, sizeof(buffer) - 1, format, args);
    va_end(args);
    Logger::instance().log_internal(Level::kCritical, "%s", buffer);
  }

  inline void log_kError(const char* format, ...) {
    if (!Logger::instance().is_level_enabled(Level::kError)) {
      return;
    }
    va_list args;
    va_start(args, format);
    char buffer[config::kMaxLogLength];
    vsnprintf(buffer, sizeof(buffer) - 1, format, args);
    va_end(args);
    Logger::instance().log_internal(Level::kError, "%s", buffer);
  }

  inline void log_kWarn(const char* format, ...) {
    if (!Logger::instance().is_level_enabled(Level::kWarn)) {
      return;
    }
    va_list args;
    va_start(args, format);
    char buffer[config::kMaxLogLength];
    vsnprintf(buffer, sizeof(buffer) - 1, format, args);
    va_end(args);
    Logger::instance().log_internal(Level::kWarn, "%s", buffer);
  }
  
  inline void log_kSuccess(const char* format, ...) {
    if (!Logger::instance().is_level_enabled(Level::kSuccess)) {
      return;
    }
    va_list args;
    va_start(args, format);
    char buffer[config::kMaxLogLength];
    vsnprintf(buffer, sizeof(buffer) - 1, format, args);
    va_end(args);
    Logger::instance().log_internal(Level::kSuccess, "%s", buffer);
  }

  inline void log_kInfo(const char* format, ...) {
    if (!Logger::instance().is_level_enabled(Level::kInfo)) {
      return;
    }
    va_list args;
    va_start(args, format);
    char buffer[config::kMaxLogLength];
    vsnprintf(buffer, sizeof(buffer) - 1, format, args);
    va_end(args);
    Logger::instance().log_internal(Level::kInfo, "%s", buffer);
  }

  inline void log_kDebug(const char* format, ...) {
    if (!Logger::instance().is_level_enabled(Level::kDebug)) {
      return;
    }
    va_list args;
    va_start(args, format);
    char buffer[config::kMaxLogLength];
    vsnprintf(buffer, sizeof(buffer) - 1, format, args);
    va_end(args);
    Logger::instance().log_internal(Level::kDebug, "%s", buffer);
  }

}

// Convenience macros
#define LOG_CRITICAL(...) logger::log_kCritical(__VA_ARGS__)
#define LOG_ERROR(...)    logger::log_kError(__VA_ARGS__)
#define LOG_WARN(...)     logger::log_kWarn(__VA_ARGS__)
#define LOG_SUCCESS(...)  logger::log_kSuccess(__VA_ARGS__)
#define LOG_INFO(...)     logger::log_kInfo(__VA_ARGS__)

// Conditional macros for different builds
#if defined(NDEBUG) || defined(CARDIAC_RELEASE)
  // PRODUCTION BUILD: Removes debug logs completely
  #define LOG_DEBUG(...) ((void)0)
#else
  // DEVELOPMENT BUILD: All logs active
  #define LOG_DEBUG(...) logger::log_kDebug(__VA_ARGS__)
#endif

#endif