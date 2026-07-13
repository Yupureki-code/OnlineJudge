#pragma once
#include <memory>
#include <spdlog/common.h>
#include <spdlog/logger.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/async.h>

namespace ns_logger
{
    inline std::shared_ptr<spdlog::logger> g_logger;
    inline void initLogger(const std::string& logger_name,const std::string& logger_file,spdlog::level::level_enum logger_level)
    {
        if(logger_file  == "")
        {
            g_logger = spdlog::stdout_color_mt(logger_name);
        }
        else
        {
            g_logger = spdlog::basic_logger_mt<spdlog::async_factory>(logger_name, logger_file);
        }
        g_logger->set_level(logger_level);
        g_logger->set_pattern("%H:%M:%S [%n][%-7l]%v");
    }
    #define LOG_DEBUG(format, ...) g_logger->debug("[{:>10s}:{:<4d}] " format, __FILE__, __LINE__, ##__VA_ARGS__);
    #define LOG_INFO(format, ...) g_logger->info("[{:>10s}:{:<4d}] " format, __FILE__, __LINE__, ##__VA_ARGS__);
    #define LOG_WARNING(format, ...) g_logger->warn("[{:>10s}:{:<4d}] " format, __FILE__, __LINE__, ##__VA_ARGS__);
    #define LOG_ERROR(format, ...) g_logger->error("[{:>10s}:{:<4d}] " format, __FILE__, __LINE__, ##__VA_ARGS__);
    #define LOG_CRITICAL(format, ...) g_logger->critical("[{:>10s}:{:<4d}] " format, __FILE__, __LINE__, ##__VA_ARGS__);
}
