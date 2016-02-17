#pragma once

#include <deque>
#include <string>

using trace_type = std::deque< std::string >;

trace_type
backtrace();

