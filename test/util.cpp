#include "util.hpp"

using namespace test;

static const uint64_t Util::current_timestamp()  {
    using namespace std::chrono;
    const auto st = system_clock::now();
    const auto duration = st.time_since_epoch();
    return duration_cast<milliseconds>(duration).count();
}