#include "logging.hpp"
#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <memory>
#include <vector>
#include <string_view>

namespace hnoker
{
    struct logger
    {
        logger()
        {
            spdlog::init_thread_pool(8192, HNOKER_LOGGER_THREAD_COUNT);
            //sinks.emplace_back(std::make_unique<spdlog::sinks::stdout_color_sink_mt>());
            sinks.emplace_back(std::make_unique<spdlog::sinks::daily_file_sink_mt>("logs/hnoker_log", 0, 0, false, 3));
            logger_instance = std::make_shared<spdlog::async_logger>(HNOKER_LOGGER_NAME, sinks.begin(), sinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);
            logger_instance->flush_on(spdlog::level::info);
            spdlog::register_logger(logger_instance);
            spdlog::set_pattern("[%H:%M:%S.%e] [%l] %v");
        }

    private:
        std::shared_ptr<spdlog::async_logger> logger_instance;
        std::vector<spdlog::sink_ptr>         sinks;
    };

    static logger s_logger {};

}
