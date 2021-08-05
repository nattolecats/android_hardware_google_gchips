#include <string>
#include <thread>
#include <vector>

#include <mali_gralloc_log.h>

class DelayedLogger {
    public:
        void setup() {
            if (setupCalled) {
                ALOGW("Bug: Log setup called multiple times");
            }

            setupCalled = true;
        }

        void add(LogLevel level, std::string log) {
            if (setupCalled)
                pendingLogs.emplace_back(make_tuple(level, log));
            else
                outputLog(level, log);
        }

        void commit() {
            for (const auto& levelLog: pendingLogs) {
                const LogLevel level = std::get<0>(levelLog);
                const std::string& log = std::get<1>(levelLog);

                outputLog(level, log);
            }

            pendingLogs.clear();
            setupCalled = false;
            logIV = false;
        }

        void logInfoVerboseAsWarning() {
            logIV = true;
        }

    private:

        void outputLog(const LogLevel level, const std::string& log) {
            switch (level) {
                case LogLevel::INFO:
                    if (logIV)
                        ALOGW("INFO: %s", log.c_str());
                    else
                        ALOGI("%s", log.c_str());
                    break;
                case LogLevel::VERBOSE:
                    if (logIV)
                        ALOGW("VERBOSE: %s", log.c_str());
                    else
                        ALOGV("%s", log.c_str());
                    break;
                case LogLevel::WARNING:
                    ALOGW("%s", log.c_str());
                    break;
                case LogLevel::ERROR:
                    ALOGE("%s", log.c_str());
                    break;
            }
        }

        std::vector<std::tuple<LogLevel, std::string>> pendingLogs = {};
        bool setupCalled = false;
        bool logIV = false;
};

thread_local DelayedLogger delayedLogger;

void log_later(LogLevel level, const char* fmt, ...) {
    char buf[10000];

    va_list argptr;
    va_start(argptr, fmt);
    vsnprintf(buf, 10000, fmt, argptr);
    va_end(argptr);

    delayedLogger.add(level, std::string(buf));
}

void log_setup() {
    delayedLogger.setup();
}

void log_commit() {
    delayedLogger.commit();
}

void log_info_verbose_as_warning() {
    delayedLogger.logInfoVerboseAsWarning();
}
