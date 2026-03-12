#include "ge2/Core/Log.h"

#include <iostream>

namespace ge2
{
    void Log::Init(bool debugLogs)
    {
        s_DebugLogs = debugLogs;
    }

    void Log::Shutdown()
    {
        std::lock_guard<std::mutex> lock(s_Mutex);
        s_Buffer.clear();
        s_Callback = nullptr;
    }

    void Log::SetAppendCallback(AppendCallback cb)
    {
        std::lock_guard<std::mutex> lock(s_Mutex);
        s_Callback = cb;
    }

    std::vector<std::pair<LogLevel, std::string>> Log::GetBufferSnapshot()
    {
        std::lock_guard<std::mutex> lock(s_Mutex);
        return s_Buffer;
    }

    void Log::Trace(const std::string& msg)
    {
        if (!s_DebugLogs)
            return;
        Append(LogLevel::Trace, msg);
    }

    void Log::Info(const std::string& msg)
    {
        Append(LogLevel::Info, msg);
    }

    void Log::Warn(const std::string& msg)
    {
        Append(LogLevel::Warn, msg);
    }

    void Log::Error(const std::string& msg)
    {
        Append(LogLevel::Error, msg);
    }

    void Log::Append(LogLevel level, const std::string& msg)
    {
        std::lock_guard<std::mutex> lock(s_Mutex);
        s_Buffer.emplace_back(level, msg);

        const char* prefix = "[INFO]";
        switch (level)
        {
        case LogLevel::Trace: prefix = "[TRACE]"; break;
        case LogLevel::Info:  prefix = "[INFO]";  break;
        case LogLevel::Warn:  prefix = "[WARN]";  break;
        case LogLevel::Error: prefix = "[ERROR]"; break;
        }

        std::cout << prefix << " " << msg << std::endl;

        if (s_Callback)
        {
            s_Callback(level, msg);
        }
    }
}

