#ifndef CARDIAC_LOGGER_H
#define CARDIAC_LOGGER_H

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
    
// Níveis de log
enum class Level : int {
    CRITICAL = 0,  // Falhas de sistema, erros de hardware, fatal
    ERROR    = 1,  // Falhas de I2C, erros de ADC, falhas de inicialização
    WARN     = 2,  // Problemas de configuração, problemas na qualidade de dados
    INFO     = 3,  // Mudança de estados no sistema, inicializações de sucesso
    DEBUG    = 4,  // Transações detalhadas do I2C, valores crus do ADC
    TRACE    = 5   // Entrada ou saída de funções, operações de registrador
};

// Configuração
namespace config {
    constexpr size_t BUFFER_SIZE = 8192;          // Ring buffer de 8KB
    constexpr size_t MAX_LOG_LENGTH = 256;        // Máximo tamanho de mensagem de log
    constexpr size_t MAX_FILENAME_LENGTH = 64;    // Máximo nome de arquivo de log
    constexpr int FLUSH_INTERVAL_MS = 1000;       // Escrever pro arquivo a cada segundo
    constexpr Level DEFAULT_LEVEL = Level::INFO;  // Nível padrão do log
}

// Estrutura interna de entrada de log
struct LogEntry {
    Level level;
    uint64_t timestamp_us;  // Microsecond timestamp for precision
    char message[config::MAX_LOG_LENGTH];
    
    LogEntry() : level(Level::INFO), timestamp_us(0) {
        message[0] = '\0';
    }
};

// Classe logger principal (padrão singleton pra acesso global)
class Logger {
private:
    // Ring buffer pra entradas de log
    std::array<LogEntry, config::BUFFER_SIZE / sizeof(LogEntry)> buffer_;
    std::atomic<size_t> write_index_{0};
    std::atomic<size_t> read_index_{0};
    
    // Configuração
    std::atomic<Level> current_level_{config::DEFAULT_LEVEL};
    char log_filename_[config::MAX_FILENAME_LENGTH];
    std::atomic<bool> initialized_{false};
    std::atomic<bool> shutdown_{false};
    
    // Sincronização de threads
    std::mutex file_mutex_;
    std::thread flush_thread_;
    
    // Construtor privado pra singleton
    Logger() {
        strcpy(log_filename_, "cardiac_monitor.log");
    }
    
    // Pega o tempo atual em microsegundos
    uint64_t get_timestamp_us() {
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = now.time_since_epoch();
        return std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
    }
    
    // Converte o tempo atual em uma string legível
    void format_timestamp(uint64_t timestamp_us, char* buffer, size_t buffer_size) {
        time_t seconds = timestamp_us / 1000000;
        int microseconds = timestamp_us % 1000000;
        
        struct tm* tm_info = localtime(&seconds);
        strftime(buffer, buffer_size, "%Y-%m-%d %H:%M:%S", tm_info);
        
        // Acrescenta microsegundos
        char temp[32];
        snprintf(temp, sizeof(temp), ".%06d", microseconds);
        strncat(buffer, temp, buffer_size - strlen(buffer) - 1);
    }
    
    // Converte nível para string
    const char* level_to_string(Level level) {
        switch (level) {
            case Level::CRITICAL: return "CRIT";
            case Level::ERROR:    return "ERR ";
            case Level::WARN:     return "WARN";
            case Level::INFO:     return "INFO";
            case Level::DEBUG:    return "DBG ";
            case Level::TRACE:    return "TRC ";
            default:              return "UNK ";
        }
    }
    
    // Função de background thread pra descarregar os logs em arquivo 
    void flush_worker() {
        while (!shutdown_.load()) {
            flush_to_file();
            std::this_thread::sleep_for(std::chrono::milliseconds(config::FLUSH_INTERVAL_MS));
        }
        // Descarrega logs no shutdown
        flush_to_file();
    }
    
    // Escreve os logs do buffer em arquivo
    void flush_to_file() {
        if (!initialized_.load()) return;
        
        std::lock_guard<std::mutex> lock(file_mutex_);
        
        FILE* file = fopen(log_filename_, "a");
        if (!file) return;
        
        size_t current_read = read_index_.load();
        size_t current_write = write_index_.load();
        
        while (current_read != current_write) {
            const LogEntry& entry = buffer_[current_read % buffer_.size()];
            
            // Formata o tempo
            char timestamp_str[32];
            format_timestamp(entry.timestamp_us, timestamp_str, sizeof(timestamp_str));
            
            // Escreve a entrada formatada do log
            fprintf(file, "[%s] %s: %s\n", 
                   timestamp_str,
                   level_to_string(entry.level),
                   entry.message);
            
            current_read++;
        }
        
        read_index_.store(current_read);
        fclose(file);
    }

public:
    // Getter de instância singleton
    static Logger& instance() {
        static Logger logger;
        return logger;
    }
    
    // Deleta operadores de construção e atribuição (padrão singleton)
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    // Inicializa o logger
    bool init(const char* filename = nullptr, Level level = config::DEFAULT_LEVEL) {
        if (initialized_.load()) return true;
        
        if (filename) {
            strncpy(log_filename_, filename, config::MAX_FILENAME_LENGTH - 1);
            log_filename_[config::MAX_FILENAME_LENGTH - 1] = '\0';
        }
        
        current_level_.store(level);
        shutdown_.store(false);
        
        // Começa a thread de descarga
        try {
            flush_thread_ = std::thread(&Logger::flush_worker, this);
            initialized_.store(true);
            
            // Sucesso na inicialização do logger
            log_internal(Level::INFO, "Cardiac Logger initialized: file=%s, level=%d", 
                        log_filename_, static_cast<int>(level));
            
            return true;
        } catch (...) {
            return false;
        }
    }
    
    // Shutdown
    void shutdown() {
        if (!initialized_.load()) return;
        
        log_internal(Level::INFO, "Cardiac Logger shutting down...");
        shutdown_.store(true);
        
        if (flush_thread_.joinable()) {
            flush_thread_.join();
        }
        
        initialized_.store(false);
    }
    
    // Destrutor
    ~Logger() {
        shutdown();
    }
    
    // Setter do nível do log
    void set_level(Level level) {
        current_level_.store(level);
    }
    
    // Getter do nível atual do log
    Level get_level() const {
        return current_level_.load();
    }
    
    // Checa se o nível do log está ativado pela configuração
    bool is_level_enabled(Level level) const {
        return static_cast<int>(level) <= static_cast<int>(current_level_.load());
    }
    
    // Função interna de logging
    void log_internal(Level level, const char* format, ...) {
        if (!is_level_enabled(level)) return;
        
        size_t write_pos = write_index_.fetch_add(1) % buffer_.size();
        LogEntry& entry = buffer_[write_pos];
        
        entry.level = level;
        entry.timestamp_us = get_timestamp_us();
        
        va_list args;
        va_start(args, format);
        vsnprintf(entry.message, config::MAX_LOG_LENGTH - 1, format, args);
        va_end(args);
        entry.message[config::MAX_LOG_LENGTH - 1] = '\0';
    }
};

// Funções de conveniência pra facilitar logging
inline bool init(const char* filename = nullptr, Level level = config::DEFAULT_LEVEL) {
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

// Funções principais de logging
inline void log_critical(const char* format, ...) {
    if (!Logger::instance().is_level_enabled(Level::CRITICAL)) return;
    va_list args;
    va_start(args, format);
    char buffer[config::MAX_LOG_LENGTH];
    vsnprintf(buffer, sizeof(buffer) - 1, format, args);
    va_end(args);
    Logger::instance().log_internal(Level::CRITICAL, "%s", buffer);
}

inline void log_error(const char* format, ...) {
    if (!Logger::instance().is_level_enabled(Level::ERROR)) return;
    va_list args;
    va_start(args, format);
    char buffer[config::MAX_LOG_LENGTH];
    vsnprintf(buffer, sizeof(buffer) - 1, format, args);
    va_end(args);
    Logger::instance().log_internal(Level::ERROR, "%s", buffer);
}

inline void log_warn(const char* format, ...) {
    if (!Logger::instance().is_level_enabled(Level::WARN)) return;
    va_list args;
    va_start(args, format);
    char buffer[config::MAX_LOG_LENGTH];
    vsnprintf(buffer, sizeof(buffer) - 1, format, args);
    va_end(args);
    Logger::instance().log_internal(Level::WARN, "%s", buffer);
}

inline void log_info(const char* format, ...) {
    if (!Logger::instance().is_level_enabled(Level::INFO)) return;
    va_list args;
    va_start(args, format);
    char buffer[config::MAX_LOG_LENGTH];
    vsnprintf(buffer, sizeof(buffer) - 1, format, args);
    va_end(args);
    Logger::instance().log_internal(Level::INFO, "%s", buffer);
}

inline void log_debug(const char* format, ...) {
    if (!Logger::instance().is_level_enabled(Level::DEBUG)) return;
    va_list args;
    va_start(args, format);
    char buffer[config::MAX_LOG_LENGTH];
    vsnprintf(buffer, sizeof(buffer) - 1, format, args);
    va_end(args);
    Logger::instance().log_internal(Level::DEBUG, "%s", buffer);
}

inline void log_trace(const char* format, ...) {
    if (!Logger::instance().is_level_enabled(Level::TRACE)) return;
    va_list args;
    va_start(args, format);
    char buffer[config::MAX_LOG_LENGTH];
    vsnprintf(buffer, sizeof(buffer) - 1, format, args);
    va_end(args);
    Logger::instance().log_internal(Level::TRACE, "%s", buffer);
}

} // namespace cardiac_logger

// Macros de conveniência
#define LOG_CRITICAL(...) cardiac_logger::log_critical(__VA_ARGS__)
#define LOG_ERROR(...)    cardiac_logger::log_error(__VA_ARGS__)
#define LOG_WARN(...)     cardiac_logger::log_warn(__VA_ARGS__)
#define LOG_INFO(...)     cardiac_logger::log_info(__VA_ARGS__)
#define LOG_DEBUG(...)    cardiac_logger::log_debug(__VA_ARGS__)
#define LOG_TRACE(...)    cardiac_logger::log_trace(__VA_ARGS__)

// Pra build final, se pode desativar os logs de trace e debug
#ifdef NDEBUG
#undef LOG_DEBUG
#undef LOG_TRACE
#define LOG_DEBUG(...) do {} while(0)
#define LOG_TRACE(...) do {} while(0)
#endif

#endif // CARDIAC_LOGGER_H