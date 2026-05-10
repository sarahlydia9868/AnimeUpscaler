#pragma once

#include <atomic>
#include <functional>
#include <string>

namespace Upscaler {
    class Process {
    public:
        using OutputHandler = std::function<void(const std::string&)>;

        explicit Process(const std::string& command);

        void SetStdoutHandler(const OutputHandler& handler);
        void SetStderrHandler(const OutputHandler& handler);

        int Run();

        static void KillCurrent();

    private:
        std::string m_Command;
        OutputHandler m_StdoutHandler;
        OutputHandler m_StderrHandler;

        static std::atomic<long> s_CurrentPid;
    };
}
