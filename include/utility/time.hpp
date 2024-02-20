#ifndef INCLUDE_UTIL_H
#define INCLUDE_UTIL_H

#include <chrono>

namespace serv {
namespace util {

template <typename T>
static const uint64_t sys_timestamp() {
    using namespace std::chrono;
    const auto st = system_clock::now();
    const auto duration = st.time_since_epoch();
    return duration_cast<T>(duration).count();
}

}
}

#endif