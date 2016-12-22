#pragma once

#if defined(NO_THREADS)
#include "coworker_sync.hpp"
#else
#include "coworker_async.hpp"
#endif
