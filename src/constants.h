#ifndef TEXT_DISPLAY_STEAMVR_OVERLAY_CONSTANTS_H
#define TEXT_DISPLAY_STEAMVR_OVERLAY_CONSTANTS_H

#define LOG_PATTERN "%^[%m-%d %T.%e] %L %-5t %-8!s:%-4#: %v%$"

#define APP_KEY "com.github.boholder.text_display_steamvr_overlay"
#define APP_NAME "Text Display Overlay"

#define SUBTITLE_KEY (APP_KEY ".window")
#define SUBTITLE_NAME (APP_NAME " Window")
#define SUBTITLE_WIDTH 1280
#define SUBTITLE_HEIGHT 960
#define SUBTITLE_INDEX 0

#define DASHBOARD_KEY (APP_KEY ".dashboard")
#define DASHBOARD_NAME APP_NAME
#define DASHBOARD_WIDTH 1280
#define DASHBOARD_HEIGHT 720
#define DASHBOARD_INDEX 1

#define TCP_SERVER_TIMEOUT std::chrono::seconds(1)
#define TCP_SERVER_BUFFER_SIZE 10240

#endif // TEXT_DISPLAY_STEAMVR_OVERLAY_CONSTANTS_H
