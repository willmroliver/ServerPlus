#ifndef INCLUDE_LOGGER_H
#define INCLUDE_LOGGER_H

#include <string>
#include <vector>
#include <list>
#include <iostream>
#include <chrono>
#include <utility>

namespace serv {

class Logger {
    private:
        static Logger* logger;
        std::list<std::pair<uint64_t, std::string>> buf;
        std::ostream* out;
        std::ostream* err;
        uint64_t sys_time() const;
        std::string format(const std::pair<uint64_t, std::string>& msg, int err_code=0) const;
        void flush();
        Logger();
    
    public:
        static Logger& get() {
            if (!logger) {
                logger = new Logger();
            }

            return *logger;
        }
        
        static void set(std::ostream* out, std::ostream* err) {
            if (!logger) {
                return;
            }

            logger->out = out;
            logger->err = err;
        }

        void operator=(const Logger&) = delete;
        void operator=(const Logger&&) = delete;

        void log(const std::string& msg, bool flush=true);
        void error(const std::string& msg, bool flush=true);
        void error(int err_code, bool flush=true);
        const std::pair<uint64_t, std::string> top() const;
        const std::pair<uint64_t, std::string> pop();
        std::ostream* const get_log_stream() const;
        std::ostream* const get_err_stream() const;
        std::vector<std::pair<uint64_t, std::string>> search_buf(const std::string& match) const;
        std::vector<std::pair<uint64_t, std::string>> search_buf(int err_code) const;
        void clear_buf();
};

}

#endif