#include "logger.hpp"

using namespace serv;

Logger* Logger::logger = nullptr;

uint64_t Logger::sys_time() const {
    using namespace std::chrono;
    const auto st = system_clock::now();
    const auto duration = st.time_since_epoch();
    return duration_cast<milliseconds>(duration).count();
}

std::string Logger::format(const std::pair<uint64_t, std::string>& msg) const {
    return std::to_string(msg.first) + " - " + msg.second;
}

void Logger::flush() {
    *out << std::flush;
    *err << std::flush;
}

Logger::Logger(): 
    out { &std::cout },
    err { &std::cerr }
{}

void Logger::log(const std::string& msg, bool flush) {
    buf.emplace_back(sys_time(), msg);
    *out << format(top());

    if (flush) {
        *out << std::endl;
    } else {
        *out << '\n';
    }
}

void Logger::error(const std::string& msg, bool flush) {
    buf.emplace_back(sys_time(), msg);
    *err << format(top());
    
    if (flush) {
        *err << std::endl;
    } else {
        *err << '\n';
    }
}

const std::pair<uint64_t, std::string> Logger::top() const {
    return buf.back();
}

const std::pair<uint64_t, std::string> Logger::pop() {
    auto lts = buf.back();
    buf.pop_back();
    return lts;
}