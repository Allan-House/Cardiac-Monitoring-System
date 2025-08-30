#ifndef CARDIAC_LOGGER_H_
#define CARDIAC_LOGGER_H_

#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <ctime>
#include <mutex>
#include <thread>
#include <atomic>
#include <array>
#include <chrono>

namespace cardiac_logger {
  enum class Level : int {
    kCritical = 0,  // System failures, hardware errors, fatal
    kError    = 1,  // I2C failures, ADC errors, initialization failures
    kWarn     = 2,  // Configuration issues, data quality issues
    kInfo     = 3,  // System state change, successful initializations
    kDebug    = 4,  // Detailed I2C transactions, raw ADC values
    kTrace    = 5   // Function entry or exit, register operations
  };

  namespace config {
    constexpr size_t kBufferSize = 8192;      // 8KB
    constexpr size_t kMaxLogLength = 256;     // 256 bytes
    constexpr size_t kMaxFilenameLength = 64; // 64 bytes
    constexpr int kFlushIntervalMS = 1000;    // 1 second
    constexpr Level kDefaultLevel = Level::kInfo;
  }

  // Internal log entry structure
  struct LogEntry {
    Level level;
    uint64_t timestamp_us;
    char message[config::kMaxLogLength];
    
    LogEntry() : level(Level::kInfo), timestamp_us(0) {
      message[0] = '\0';
    }
  };

  class Logger {
    private:
    // Ring buffer for log entries
    std::array<LogEntry, config::kBufferSize / sizeof(LogEntry)> buffer_;
    std::atomic<size_t> write_index_{0};
    std::atomic<size_t> read_index_{0};
    
    // Settings
    std::atomic<Level> current_level_{config::kDefaultLevel};
    char log_filename_[config::kMaxFilenameLength];
    std::atomic<bool> initialized_{false};
    std::atomic<bool> shutdown_{false};
    std::atomic<bool> console_output_{true};  // Enable console output by default
      
    // Thread synchronization
    std::mutex file_mutex_;
    std::mutex console_mutex_;  // Separate mutex for console output
    std::thread flush_thread_;
      
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
          
      struct tm* tm_info = localtime(&seconds);
      strftime(buffer, buffer_size, "%Y-%m-%d %H:%M:%S", tm_info);
          
      // Add microseconds
      char temp[32];
      snprintf(temp, sizeof(temp), ".%06d", microseconds);
      strncat(buffer, temp, buffer_size - strlen(buffer) - 1);
    }
      
    const char* level_to_string(Level level) {
      switch (level) {
        case Level::kCritical: return "CRIT";
        case Level::kError:    return "ERR";
        case Level::kWarn:     return "WARN";
        case Level::kInfo:     return "INFO";
        case Level::kDebug:    return "DBG ";
        case Level::kTrace:    return "TRC ";
        default:               return "UNK ";
      }
    }

    // Get ANSI color codes for console output
    const char* level_to_color(Level level) {
      switch (level) {
        case Level::kCritical: return "\033[1;35m";  // Bright Magenta
        case Level::kError:    return "\033[1;31m";  // Bright Red
        case Level::kWarn:     return "\033[1;33m";  // Bright Yellow
        case Level::kInfo:     return "\033[1;32m";  // Bright Green
        case Level::kDebug:    return "\033[1;36m";  // Bright Cyan
        case Level::kTrace:    return "\033[1;37m";  // Bright White
        default:               return "\033[0m";     // Reset
      }
    }
      
    void flush_worker() {
      while (!shutdown_.load()) {
        flush_to_file();
        std::this_thread::sleep_for(std::chrono::milliseconds(config::kFlushIntervalMS));
      }
      // Final flush when shutting down
      flush_to_file();
    }
      
    void flush_to_file() {
      if (!initialized_.load()) {
        return;
      }

      std::lock_guard<std::mutex> lock(file_mutex_);
          
      FILE* file = fopen(log_filename_, "a");
      if (!file) {
        return;
      }

      size_t current_read = read_index_.load();
      size_t current_write = write_index_.load();
          
      while (current_read != current_write) {
        const LogEntry& entry = buffer_[current_read % buffer_.size()];
              
        // Format the time
        char timestamp_str[32];
        format_timestamp(entry.timestamp_us, timestamp_str, sizeof(timestamp_str));
              
        // Write formatted log entry to file only (console already handled)
        fprintf(file, "[%s] %s: %s\n",
                timestamp_str, 
                level_to_string(entry.level),
                entry.message);

        current_read = (current_read + 1) % buffer_.size();
      }
          
      read_index_.store(current_read);
      fflush(file);  // Force flush to disk
      fclose(file);
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
      shutdown_.store(false);
          
      // Initialize the buffer indices
      write_index_.store(0);
      read_index_.store(0);
      
      // Start the flushing thread
      try {
        flush_thread_ = std::thread(&Logger::flush_worker, this);
        initialized_.store(true);
            
        // Add a small delay to ensure thread is ready
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        log_internal(Level::kInfo,
                      "Cardiac Logger initialized: file=%s, level=%d",
                      log_filename_,
                      static_cast<int>(level));
            
        return true;
      } catch (...) {
        return false;
      }
    }
      
    void shutdown() {
      if (!initialized_.load()) {
        return;
      }

      log_internal(Level::kInfo, "Cardiac Logger shutting down...");
      
      // Give some time for the last message to be written
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
      
      shutdown_.store(true);
      
      if (flush_thread_.joinable()) {
        flush_thread_.join();
      }
      
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
      if (!is_level_enabled(level)) {
        return;
      }

      // Wait a bit if not initialized yet
      if (!initialized_.load()) {
        return;
      }

      LogEntry entry;
      entry.level = level;
      entry.timestamp_us = get_timestamp_us();
          
      va_list args;
      va_start(args, format);
      vsnprintf(entry.message, config::kMaxLogLength - 1, format, args);
      va_end(args);
      entry.message[config::kMaxLogLength - 1] = '\0';

      // Immediate console output for better responsiveness
      if (console_output_.load()) {
        char timestamp_str[32];
        format_timestamp(entry.timestamp_us, timestamp_str, sizeof(timestamp_str));
        
        std::lock_guard<std::mutex> console_lock(console_mutex_);
        printf("%s[%s] %s: %s\033[0m\n",
               level_to_color(entry.level),
               timestamp_str,
               level_to_string(entry.level),
               entry.message);
        fflush(stdout);  // Immediate console output
      }

      // Add to buffer for file logging
      size_t write_pos = write_index_.fetch_add(1) % buffer_.size();
      buffer_[write_pos] = entry;
    }

    // Add a force flush method for testing
    void force_flush() {
      flush_to_file();
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

  inline void log_kTrace(const char* format, ...) {
    if (!Logger::instance().is_level_enabled(Level::kTrace)) {
      return;
    }
    va_list args;
    va_start(args, format);
    char buffer[config::kMaxLogLength];
    vsnprintf(buffer, sizeof(buffer) - 1, format, args);
    va_end(args);
    Logger::instance().log_internal(Level::kTrace, "%s", buffer);
  }

} // namespace cardiac_logger

// Convenience macros
#define LOG_CRITICAL(...) cardiac_logger::log_kCritical(__VA_ARGS__)
#define LOG_ERROR(...)    cardiac_logger::log_kError(__VA_ARGS__)
#define LOG_WARN(...)     cardiac_logger::log_kWarn(__VA_ARGS__)
#define LOG_INFO(...)     cardiac_logger::log_kInfo(__VA_ARGS__)

// Conditional macros for different builds
#if defined(NDEBUG) || defined(CARDIAC_RELEASE)
  // PRODUCTION BUILD: Removes debug/trace logs completely
  #define LOG_DEBUG(...) ((void)0)
  #define LOG_TRACE(...) ((void)0)
#else
  // DEVELOPMENT BUILD: All logs active
  #define LOG_DEBUG(...) cardiac_logger::log_kDebug(__VA_ARGS__)
  #define LOG_TRACE(...) cardiac_logger::log_kTrace(__VA_ARGS__)
#endif

#endif