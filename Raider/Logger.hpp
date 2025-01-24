#pragma once
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#define LOGGER_NAME "RaiderLogger"
#if 0
#define LOG_DEBUG(...)                                            \
    if (spdlog::get(LOGGER_NAME) != nullptr)          \
    {                                                               \
        spdlog::get(LOGGER_NAME)->debug(std::format(__VA_ARGS__)); \
    }
#define LOG_INFO(...)                                            \
    if (spdlog::get(LOGGER_NAME) != nullptr)         \
    {                                                              \
        spdlog::get(LOGGER_NAME)->info(std::format(__VA_ARGS__)); \
    }
#define LOG_WARN(...)                                            \
    if (spdlog::get(LOGGER_NAME) != nullptr)         \
    {                                                              \
        spdlog::get(LOGGER_NAME)->warn(std::format(__VA_ARGS__)); \
    }
#define LOG_ERROR(...)                                            \
    if (spdlog::get(LOGGER_NAME) != nullptr)          \
    {                                                               \
        spdlog::get(LOGGER_NAME)->error(std::format(__VA_ARGS__)); \
    }
#define LOG_FATAL(...)                                               \
    if (spdlog::get(LOGGER_NAME) != nullptr)             \
    {                                                                  \
        spdlog::get(LOGGER_NAME)->critical(std::format(__VA_ARGS__)); \
    }
#endif
#define LOG_DEBUG(...) printf((std::format(__VA_ARGS__) + '\n').c_str());
#define LOG_INFO(...)  printf((std::format(__VA_ARGS__) + '\n').c_str());
#define LOG_WARN(...)  printf((std::format(__VA_ARGS__) + '\n').c_str());
#define LOG_ERROR(...) printf((std::format(__VA_ARGS__) + '\n').c_str());
#define LOG_FATAL(...) printf((std::format(__VA_ARGS__) + '\n').c_str());
namespace raider::utils
{
    class Logger
    {
    public:
        static void Initialize()
        {
            auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            sink->set_pattern("%^[%H:%M:%S.%e] %v%$");

            std::vector<spdlog::sink_ptr> sinks = {sink};
            auto logger = std::make_shared<spdlog::logger>("RaiderLogger", sinks.begin(), sinks.end());

            logger->set_level(spdlog::level::level_enum::info);
            logger->flush_on(spdlog::level::level_enum::info);
            spdlog::register_logger(logger);
        }

        static void Shutdown()
        {
            spdlog::shutdown();
        }
    };
}