/* Definition for thread-local custom test flag used across judge chain */
#include "COP_hanlder.hpp"

namespace ns_runner {
    thread_local bool g_is_custom_test = false;
}
