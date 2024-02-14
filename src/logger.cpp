#include "logger.hpp"
#include "error-codes.hpp"

using namespace serv;

Logger* Logger::logger = nullptr;

uint64_t Logger::sys_time() const {
    using namespace std::chrono;
    const auto st = system_clock::now();
    const auto duration = st.time_since_epoch();
    return duration_cast<milliseconds>(duration).count();
}

std::string Logger::format(const std::pair<uint64_t, std::string>& msg, int err_code) const {
    auto formatted = std::to_string(msg.first);
    formatted.append(" - ");

    if (err_code) {
        formatted.append("ERR ").append(std::to_string(err_code)).append(" - ");
    }

    return formatted.append(msg.second);
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
    if (buf.size() > 100) {
    }
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

void Logger::error(int err_code, bool flush) {
    buf.emplace_back(sys_time(), error_messages[err_code]);
    if (buf.size() > 100) {
        buf.pop_front();
    }
    
    *err << format(top(), err_code);
    
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

std::ostream* const Logger::get_log_stream() const {
    return out;
}

std::ostream* const Logger::get_err_stream() const {
    return err;
}

std::vector<std::pair<uint64_t, std::string>> Logger::search_buf(const std::string& substr) const {
    std::vector<std::pair<uint64_t, std::string>> results;

    for (auto it = buf.begin(); it != buf.end(); ++it) {
        if (it->second.find(substr) != std::string::npos) {
            results.emplace_back(*it);
        }
    }

    return results;
}

std::vector<std::pair<uint64_t, std::string>> Logger::search_buf(int err_code) const {
    std::vector<std::pair<uint64_t, std::string>> results;

    for (auto it = buf.begin(); it != buf.end(); ++it) {
        if (it->second.find(error_messages[err_code]) != std::string::npos) {
            results.emplace_back(*it);
        }
    }

    return results;
}

void Logger::clear_buf() {
    buf.clear();
}