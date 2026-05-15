#ifndef TEXT_DISPLAY_STEAMVR_OVERLAY_LOG_H
#define TEXT_DISPLAY_STEAMVR_OVERLAY_LOG_H

// ref: https://github.com/gabime/spdlog/wiki/FAQ#how-to-remove-all-debug-statements-at-compile-time-
#ifdef ENABLE_DEBUG_LOG
#    define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG
#endif

#include <spdlog/spdlog.h>

#ifndef ENABLE_DEBUG_LOG
#    undef SPDLOG_DEBUG
#    define SPDLOG_DEBUG(fmt, ...) spdlog::debug(fmt, ##__VA_ARGS__)
#endif

#endif // TEXT_DISPLAY_STEAMVR_OVERLAY_LOG_H
