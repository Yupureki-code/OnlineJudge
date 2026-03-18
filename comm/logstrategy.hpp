#pragma once

#include <iostream>
#include <memory>
#include <unistd.h>
#include "lock.hpp"
#include <fstream>
#include <sstream>
#include <ctime>

namespace ns_log
{
    #define DEFUALT_PATH "./log/"
    #define DEFUALT_NAME "log.txt"
    #define ERR_EXIT(m)         \
        do                      \
        {                       \
            perror(m);          \
            exit(EXIT_FAILURE); \
        } while (0)

    enum LogLevel
    {
        DEBUG = 0,
        INFO,
        WARNING,
        ERROR,
        FATAL
    };

    inline std::string LogLevelToString(const LogLevel level)
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

    inline std::string get_current_time()
    {
        time_t tm = time(nullptr);
        struct tm curr;
        localtime_r(&tm, &curr);
        char timebuffer[64];
        snprintf(timebuffer, sizeof(timebuffer), "%4d-%02d-%02d% 02d:%02d:%02d",
                curr.tm_year + 1900,
                curr.tm_mon + 1,
                curr.tm_mday,
                curr.tm_hour,
                curr.tm_min,
                curr.tm_sec);
        return timebuffer;
    }

    class LogStrategy
    {
    public:
        virtual ~LogStrategy() = default;
        virtual void synclog(const std::string message) = 0;
    };

    class ConsoleLogStrategy : public LogStrategy
    {
    public:
        virtual void synclog(const std::string message)
        {
            _lock.lock();
            std::cout << message << std::endl;
            _lock.unlock();
        }
        virtual ~ConsoleLogStrategy()
        {
        }

    private:
        mylock _lock;
    };

    class FileLogStrategy : public LogStrategy
    {
    public:
        FileLogStrategy(const std::string path = DEFUALT_PATH, const std::string name = DEFUALT_NAME)
            : _path(path), _name(name)
        {
            _file = _path + (_path.back() == '/' ? "" : "/") + _name;
            std::fstream file;
            file.open(_file.c_str(), std::ios::in | std::ios::out);
            if (!file.is_open())
            {
                file.open(_file.c_str(), std::ios::out);
                file.close();
            }
        }
        virtual void synclog(const std::string message)
        {
            _lock.lock();
            std::ofstream fout(_file.c_str(), std::ios::app);
            if (!fout.is_open())
            {
                ERR_EXIT("fout");
                return;
            }
            fout << message << "\n";
            fout.close();
            _lock.unlock();
        }
        ~FileLogStrategy()
        {}

    private:
        std::string _path;
        std::string _name;
        std::string _file;
        mylock _lock;
    };

    class Logger
    {
    private:
        Logger()
        {
            _strategy = std::make_unique<ConsoleLogStrategy>();
        }
        Logger(const Logger&) = delete;
    public:
        void enable_console_log_strategy()
        {
            _strategy = std::make_unique<ConsoleLogStrategy>();
        }
        void enable_file_log_strategy(const std::string path = DEFUALT_PATH, const std::string name = DEFUALT_NAME)
        {
            _strategy = std::make_unique<FileLogStrategy>(path, name);
        }
        static Logger& GetInstance()
        {
            return Logger::_inst;
        }
        class LogMessage
        {
        public:
            LogMessage(const LogLevel type, const std::string filename, const int line, Logger &logger)
                : _type(type), _curr_time(get_current_time()), _pid(getpid()), _filename(filename), _line(line), _logger(logger)
            {
                std::stringstream ssbuffer;
                ssbuffer << "[" << _curr_time << "]"
                        << "[" << LogLevelToString(type) << "]"
                        << "[" << _pid << "]"
                        << "[" << _filename << "]"
                        << "[" << _line << "]"
                        << " - ";
                _loginfo = ssbuffer.str();
            }
            template<class T>
            LogMessage &operator<<(const T& str)
            {
                std::stringstream ss;
                ss << str;
                _loginfo += ss.str();
                return *this;
            }
            ~LogMessage()
            {
                _logger._strategy->synclog(_loginfo);
            }
        private:
            LogLevel _type;
            std::string _curr_time;
            pid_t _pid;
            std::string _filename;
            int _line;
            Logger &_logger;
            std::string _loginfo;
        };

        LogMessage operator()(const LogLevel level,const int line,const std::string filename)
        {
            return LogMessage(level,filename,line,*this);
        }
    private:
        std::unique_ptr<LogStrategy> _strategy;
        static Logger _inst;
    };

    inline Logger Logger::_inst;
    #define logger(level) Logger::GetInstance()(level, __LINE__, __FILE__)
}
