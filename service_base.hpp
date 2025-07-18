#pragma once
// std
#include <Windows.h>
#include <string>
#include <thread>

using string = std::string;
namespace svc {

class service_base {
  public:
    static service_base* instance;
    service_base(const string& name) : service_name(name) {}
    service_base(const string& name, bool s_mode) : service_name(name), service_mode(s_mode) {}
    virtual ~service_base() = default;

    void run() {
        SERVICE_TABLE_ENTRY service_table[] = {
          {const_cast<LPSTR>(service_name.c_str()), ServiceMain}, {nullptr, nullptr}};

        instance = this;

        if (!StartServiceCtrlDispatcherA(service_table)) {
            log_event(EVENTLOG_ERROR_TYPE, "Failed to connect to SCM");
        }
    }

    bool is_running() const { return running; }

    void on_start(int argc, char* argv[]) { on_start((DWORD)argc, (LPSTR*)argv); }

    void on_control(int ctrl_code) { on_control((DWORD)ctrl_code); }

    void set_service_mode(bool flag) { service_mode = flag; }

  protected:
    int argc    = 0;
    char** argv = nullptr;
    std::string service_name;
    SERVICE_STATUS_HANDLE status_handle = nullptr;
    SERVICE_STATUS service_status       = {};
    bool service_mode                   = true;
    bool running                        = false;

    virtual void main_loop()            = 0;

    static void WINAPI ServiceMain(DWORD argc, LPSTR* argv) {
        if (instance) {
            instance->on_start(argc, argv);
        }
    }

    static void WINAPI ServiceCtrlHandler(DWORD ctrl_code) {
        if (instance) {
            instance->on_control(ctrl_code);
        }
    }

    void on_start(DWORD argc, LPSTR* argv) {
        this->argc                        = argc;
        this->argv                        = argv;

        service_status.dwServiceType      = SERVICE_WIN32_OWN_PROCESS;
        service_status.dwControlsAccepted = SERVICE_ACCEPT_STOP;
        service_status.dwCurrentState     = SERVICE_START_PENDING;

        if (service_mode) {
            status_handle = RegisterServiceCtrlHandlerA(service_name.c_str(), ServiceCtrlHandler);
            if (!status_handle)
                return;

            service_status.dwCurrentState = SERVICE_RUNNING;
            SetServiceStatus(status_handle, &service_status);
        }

        running = true;
        log_event(EVENTLOG_INFORMATION_TYPE, "Service started");

        main_loop();

        if (service_mode) {
            service_status.dwCurrentState = SERVICE_STOPPED;
            SetServiceStatus(status_handle, &service_status);
        }
    }

    void on_control(DWORD ctrl_code) {
        switch (ctrl_code) {
        case SERVICE_CONTROL_STOP:
            log_event(EVENTLOG_INFORMATION_TYPE, "Service stop requested");
            service_status.dwCurrentState = SERVICE_STOP_PENDING;
            SetServiceStatus(status_handle, &service_status);
            running = false;
            break;
        default:
            break;
        }
    }

    void log_event(WORD type, const string& message) {
        HANDLE hEventLog = RegisterEventSource(nullptr, service_name.c_str());
        if (hEventLog) {
            LPCSTR strings[1] = {message.c_str()};
            ReportEventA(hEventLog, type, 0, 0, nullptr, 1, 0, strings, nullptr);
            DeregisterEventSource(hEventLog);
        }
    }
};

service_base* service_base::instance = nullptr;

} // namespace svc