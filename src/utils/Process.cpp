#include "Process.h"

#include <wx/process.h>
#include <wx/utils.h>

#include <chrono>
#include <thread>

namespace Upscaler {
    std::atomic<long> Process::s_CurrentPid = 0;

    Process::Process(const std::string& command) : m_Command(command) {}

    void Process::SetStdoutHandler(const OutputHandler& handler) {
        m_StdoutHandler = handler;
    }

    void Process::SetStderrHandler(const OutputHandler& handler) {
        m_StderrHandler = handler;
    }

    static void DrainStream(wxInputStream* stream,
                            std::string& buffer,
                            const Process::OutputHandler& handler) {
        if (!stream) {
            return;
        }

        char temp[4096];
        while (stream->CanRead()) {
            stream->Read(temp, sizeof(temp));
            size_t read = stream->LastRead();
            if (read == 0) {
                break;
            }

            buffer.append(temp, read);

            size_t pos = 0;
            while ((pos = buffer.find_first_of("\r\n")) != std::string::npos) {
                std::string line = buffer.substr(0, pos);
                if (handler && !line.empty()) {
                    handler(line);
                }

                size_t separatorLength = 1;
                if (pos + 1 < buffer.size() && buffer[pos] == '\r' && buffer[pos + 1] == '\n') {
                    separatorLength = 2;
                }
                buffer.erase(0, pos + separatorLength);
            }
        }
    }


    int Process::Run() {
        class ProcessRunner : public wxProcess {
        public:
            ProcessRunner() { Redirect(); }

            void OnTerminate(int, int status) override {
                m_ExitCode.store(status);
                m_Finished.store(true);
            }

            std::atomic<int> m_ExitCode { -1 };
            std::atomic<bool> m_Finished { false };
        };

        auto* process = new ProcessRunner();

        long pid = wxExecute(m_Command, wxEXEC_ASYNC, process);
        if (pid == 0) {
            delete process;
            return -1;
        }

        s_CurrentPid = pid;

        std::string stdoutBuffer;
        std::string stderrBuffer;

        bool processExists = true;
        int noOutputCount = 0;
        while (processExists) {
            bool hadOutput = false;
            
            auto* inStream = process->GetInputStream();
            auto* errStream = process->GetErrorStream();
            
            if (inStream && inStream->CanRead()) {
                DrainStream(inStream, stdoutBuffer, m_StdoutHandler);
                hadOutput = true;
            }
            
            if (errStream && errStream->CanRead()) {
                DrainStream(errStream, stderrBuffer, m_StderrHandler);
                hadOutput = true;
            }
            
            processExists = wxProcess::Exists(pid);
            
            if (!processExists) {
                if (hadOutput) {
                    noOutputCount = 0;
                } else {
                    noOutputCount++;
                    if (noOutputCount > 10) {
                        break;
                    }
                }
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
        }

        if (!stdoutBuffer.empty() && m_StdoutHandler) {
            m_StdoutHandler(stdoutBuffer);
        }
        if (!stderrBuffer.empty() && m_StderrHandler) {
            m_StderrHandler(stderrBuffer);
        }

        for (int i = 0; i < 50 && !process->m_Finished.load(); i++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        int exitCode = process->m_ExitCode.load();
        
        if (exitCode == -1 && !wxProcess::Exists(pid)) {
            exitCode = 0;
        }
        
        if (m_StdoutHandler) {
            m_StdoutHandler("[DEBUG] Process finished with exit code: " + std::to_string(exitCode));
        }
        
        delete process;
        s_CurrentPid = 0;
        return exitCode;
    }

    void Process::KillCurrent() {
        long pid = s_CurrentPid.load();
        if (pid != 0) {
            wxKill(pid, wxSIGKILL, nullptr, wxKILL_CHILDREN);
        }
    }
}
