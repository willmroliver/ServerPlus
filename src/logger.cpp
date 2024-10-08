#include <mutex>
#include "logger.hpp"
#include "utility/time.hpp"
#include "error-codes.hpp"

using namespace serv;

Logger* Logger::logger = nullptr;

std::pair<uint64_t, std::string> Logger::new_item(const std::string& msg) {
    return { sys_time(), msg };
}

std::pair<uint64_t, std::string> Logger::new_item(int err_code) {
    return { sys_time(), error_messages[err_code] };
}

void Logger::log_item(std::ostream* stream, const std::pair<uint64_t, std::string>& item, int err_code, bool flush) noexcept {
    try {
        std::lock_guard lock { log_item_mutex };

        buf.push_back(item);

        if (buf.size() > 100) {
            buf.pop_front();
        }

        *stream << format(item, err_code);

        if (flush) {
            *stream << std::endl;
        } else {
            *stream << '\n';
        }
    }
    catch (const std::exception& e) {
        std::cerr << "LOGGER EXCEPTION: " << e.what() << std::endl;
    }
}

std::string Logger::format(const std::pair<uint64_t, std::string>& item, int err_code) const {
    auto formatted = std::to_string(item.first);
    formatted.append(" - ");

    if (err_code) {
        formatted.append("ERR ").append(std::to_string(err_code)).append(" - ");
    }

    return formatted.append(item.second);
}

void Logger::flush() {
    *out << std::flush;
    *err << std::flush;
}

Logger::Logger(): 
    out { &std::cout },
    err { &std::cerr }
{}

uint64_t Logger::sys_time() const {
    using namespace std::chrono;
    return util::sys_timestamp<milliseconds>();
}

const std::pair<uint64_t, std::string> Logger::log(const std::string& msg, bool flush) {
    auto item = new_item(msg);
    log_item(out, item, 0, flush);
    return item;
}

const std::pair<uint64_t, std::string> Logger::error(const std::string& msg, bool flush) {
    auto item = new_item(msg);
    log_item(err, item, 0, flush);
    return item;
}

const std::pair<uint64_t, std::string> Logger::error(int err_code, const std::exception* e, bool flush) {
    auto item = new_item(err_code);
    log_item(err, item, err_code, flush);

    if (e) {
        error(e->what());
    }

    return item;
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