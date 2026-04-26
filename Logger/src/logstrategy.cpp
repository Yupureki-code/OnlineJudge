#include "../include/logstrategy.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#ifdef LOGGER_USE_SPDLOG
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#endif

namespace ns_log
{
#ifdef LOGGER_USE_SPDLOG
    namespace
    {
        int GetEnvIntOrDefault(const char* name, int default_value)
        {
            const char* raw = std::getenv(name);
            if (raw == nullptr || *raw == '\0')
            {
                return default_value;
            }
            char* end = nullptr;
            long parsed = std::strtol(raw, &end, 10);
            if (end == raw)
            {
                return default_value;
            }
            if (parsed > 2147483647L)
            {
                return 2147483647;
            }
            if (parsed < -2147483647L)
            {
                return -2147483647;
            }
            return static_cast<int>(parsed);
        }

        spdlog::level::level_enum ToSpdLevel(LogLevel level)
        {
            switch (level)
            {
            case LogLevel::DEBUG:
                return spdlog::level::debug;
            case LogLevel::INFO:
                return spdlog::level::info;
            case LogLevel::WARNING:
                return spdlog::level::warn;
            case LogLevel::ERROR:
                return spdlog::level::err;
            case LogLevel::FATAL:
                return spdlog::level::critical;
            default:
                return spdlog::level::info;
            }
        }

        std::string SanitizeLoggerName(const std::string& in)
        {
            std::string out = in;
            for (char& c : out)
            {
                if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-'))
                {
                    c = '_';
                }
            }
            return out;
        }
    }
#endif

    static void log_exit(const char* message)
    {
        perror(message);
        exit(EXIT_FAILURE);
    }

    std::string LogLevelToString(LogLevel level)
    {
        switch (level)
        {
        case LogLevel::DEBUG:
            return "DEBUG";
        case LogLevel::INFO:
            return "INFO";
        case LogLevel::WARNING:
            return "WARNING";
        case LogLevel::ERROR:
            return "ERROR";
        case LogLevel::FATAL:
            return "FATAL";
        default:
            return "UNKNOWN";
        }
    }

    std::string get_current_time()
    {
        time_t tm = time(nullptr);
        struct tm curr;
        localtime_r(&tm, &curr);
        char timebuffer[64];
        snprintf(timebuffer, sizeof(timebuffer), "%4d-%02d-%02d %02d:%02d:%02d",
                 curr.tm_year + 1900,
                 curr.tm_mon + 1,
                 curr.tm_mday,
                 curr.tm_hour,
                 curr.tm_min,
                 curr.tm_sec);
        return timebuffer;
    }

    void ConsoleLogStrategy::synclog(LogLevel level, const std::string& message)
    {
        std::lock_guard<std::mutex> guard(_mutex);
#ifdef LOGGER_USE_SPDLOG
        if (!_spd_logger)
        {
            auto existing = spdlog::get("oj_console");
            if (existing)
            {
                _spd_logger = existing;
            }
            else
            {
                _spd_logger = spdlog::stdout_color_mt("oj_console");
                _spd_logger->set_pattern("%v");
            }
            _spd_logger->set_level(spdlog::level::debug);
            _spd_logger->flush_on(spdlog::level::err);
        }
        _spd_logger->log(ToSpdLevel(level), message);
#else
        std::cout << message << std::endl;
#endif
    }

    FileLogStrategy::FileLogStrategy(const std::string path, const std::string name)
        : _path(path), _name(name)
    {
        _file = _path + (_path.back() == '/' ? "" : "/") + _name;
#ifdef LOGGER_USE_SPDLOG
        std::string logger_name = "oj_file_" + SanitizeLoggerName(_file);
        auto existing = spdlog::get(logger_name);
        if (existing)
        {
            _spd_logger = existing;
        }
        else
        {
            int rotate_mb = GetEnvIntOrDefault("LOGGER_ROTATE_MAX_MB", 10);
            int rotate_files = GetEnvIntOrDefault("LOGGER_ROTATE_FILES", 5);
            if (rotate_mb < 1)
            {
                rotate_mb = 1;
            }
            if (rotate_files < 1)
            {
                rotate_files = 1;
            }
            std::size_t max_bytes = static_cast<std::size_t>(rotate_mb) * 1024U * 1024U;
            _spd_logger = spdlog::rotating_logger_mt(logger_name, _file, max_bytes, static_cast<std::size_t>(rotate_files), true);
            _spd_logger->set_pattern("%v");
        }
        _spd_logger->set_level(spdlog::level::debug);
        _spd_logger->flush_on(spdlog::level::err);
#else
        std::fstream file;
        file.open(_file.c_str(), std::ios::in | std::ios::out);
        if (!file.is_open())
        {
            file.open(_file.c_str(), std::ios::out);
            file.close();
        }
#endif
    }

    void FileLogStrategy::synclog(LogLevel level, const std::string& message)
    {
        std::lock_guard<std::mutex> guard(_mutex);
#ifdef LOGGER_USE_SPDLOG
        if (!_spd_logger)
        {
            return;
        }
        _spd_logger->log(ToSpdLevel(level), message);
#else
        std::ofstream fout(_file.c_str(), std::ios::app);
        if (!fout.is_open())
        {
            log_exit("fout");
            return;
        }
        fout << message << "\n";
#endif
    }

    Logger::Logger()
        : _strategy(std::make_unique<ConsoleLogStrategy>())
    {
        _enabled_levels[LogLevel::DEBUG] = true;
        _enabled_levels[LogLevel::INFO] = true;
        _enabled_levels[LogLevel::WARNING] = true;
        _enabled_levels[LogLevel::ERROR] = true;
        _enabled_levels[LogLevel::FATAL] = true;
    }

    void Logger::enable_console_log_strategy()
    {
        _strategy = std::make_unique<ConsoleLogStrategy>();
    }

    void Logger::enable_file_log_strategy(const std::string path, const std::string name)
    {
        _strategy = std::make_unique<FileLogStrategy>(path, name);
    }

    Logger& Logger::GetInstance()
    {
        return Logger::_inst;
    }
    void Logger::EnableLogLevel(LogLevel level)
    {
        _enabled_levels[level] = true;
    }
    void Logger::DisableLogLevel(LogLevel level)
    {
        _enabled_levels[level] = false;
    }
    bool Logger::IsLogLevelEnabled(LogLevel level)
    {        
        return _enabled_levels[level];
    }
    Logger::LogMessage::LogMessage(const LogLevel type, const std::string filename, const int line, Logger& logger)
        : _type(type), _curr_time(get_current_time()), _pid(getpid()), _filename(filename), _line(line), _logger(logger)
    {
        if(!logger.IsLogLevelEnabled(type))
            return;
        _enabled = true;
        std::stringstream ssbuffer;
        ssbuffer << "[" << _curr_time << "]"
                << "[" << LogLevelToString(type) << "]"
                << "[" << _pid << "]"
                << "[" << _filename << "]"
                << "[" << _line << "]"
                << " - ";
        _loginfo = ssbuffer.str();
    }

    Logger::LogMessage::~LogMessage()
    {
        if (!_enabled)
        {
            return;
        }
        _logger._strategy->synclog(_type, _loginfo);
    }

    Logger::LogMessage Logger::operator()(const LogLevel level,const int line,const std::string filename)
    {
        return LogMessage(level,filename,line,*this);
    }

    Logger Logger::_inst;
}