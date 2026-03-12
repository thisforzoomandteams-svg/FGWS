#pragma once

#include <string>
#include <functional>
#include <mutex>
#include <vector>

namespace ge2
{
    enum class LogLevel
    {
        Trace = 0,
        Info,
        Warn,
        Error
    };

    class Log
    {
    public:
        using AppendCallback = std::function<void(LogLevel, const std::string&)>;

        static void Init(bool debugLogs);
        static void Shutdown();

        static void SetAppendCallback(AppendCallback cb);

        static void Trace(const std::string& msg);
        static void Info(const std::string& msg);
        static void Warn(const std::string& msg);
        static void Error(const std::string& msg);

        static std::vector<std::pair<LogLevel, std::string>> GetBufferSnapshot();

    private:
        static void Append(LogLevel level, const std::string& msg);

    private:
        static inline std::mutex s_Mutex;
        static inline AppendCallback s_Callback;
        static inline std::vector<std::pair<LogLevel, std::string>> s_Buffer;
        static inline bool s_DebugLogs = false;
    };
}

