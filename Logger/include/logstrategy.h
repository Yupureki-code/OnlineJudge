#pragma once

#include <ctime>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <unistd.h>
#include <unordered_map>
#ifdef LOGGER_USE_SPDLOG
#include <spdlog/logger.h>
#endif

namespace ns_log
{
    #define DEFUALT_PATH "./log/"
    #define DEFUALT_NAME "log.txt"

    enum LogLevel
    {
        DEBUG = 0,
        INFO,
        WARNING,
        ERROR,
        FATAL
    };

    std::string LogLevelToString(LogLevel level);
    std::string get_current_time();

    class LogStrategy
    {
    public:
        virtual ~LogStrategy() = default;
        virtual void synclog(LogLevel level, const std::string& message) = 0;
    };

    class ConsoleLogStrategy : public LogStrategy
    {
    public:
        ConsoleLogStrategy() = default;
        void synclog(LogLevel level, const std::string& message) override;

    private:
        std::mutex _mutex;
#ifdef LOGGER_USE_SPDLOG
        std::shared_ptr<spdlog::logger> _spd_logger;
#endif
    };

    class FileLogStrategy : public LogStrategy
    {
    public:
        FileLogStrategy(const std::string path = DEFUALT_PATH, const std::string name = DEFUALT_NAME);
        void synclog(LogLevel level, const std::string& message) override;

    private:
        std::string _path;
        std::string _name;
        std::string _file;
        std::mutex _mutex;
    #ifdef LOGGER_USE_SPDLOG
        std::shared_ptr<spdlog::logger> _spd_logger;
    #endif
    };

    class Logger
    {
    private:
        Logger();
        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;

    public:
        void enable_console_log_strategy();
        void enable_file_log_strategy(const std::string path = DEFUALT_PATH, const std::string name = DEFUALT_NAME);
        static Logger& GetInstance();
        void EnableLogLevel(LogLevel level);
        void DisableLogLevel(LogLevel level);
        bool IsLogLevelEnabled(LogLevel level);
        class LogMessage
        {
        public:
            LogMessage(const LogLevel type, const std::string filename, const int line, Logger& logger);
            template<class T>
            LogMessage &operator<<(const T& str)
            {
                std::stringstream ss;
                ss << str;
                _loginfo += ss.str();
                return *this;
            }
            ~LogMessage();
        private:
            LogLevel _type;
            std::string _curr_time;
            pid_t _pid;
            std::string _filename;
            int _line;
            Logger &_logger;
            std::string _loginfo;
            bool _enabled = false;
        };

        LogMessage operator()(const LogLevel level,const int line,const std::string filename);

    private:
        std::unique_ptr<LogStrategy> _strategy;
        std::unordered_map<LogLevel, bool> _enabled_levels;
        static Logger _inst;
    };

    #define logger(level) ns_log::Logger::GetInstance()(level, __LINE__, __FILE__)
}