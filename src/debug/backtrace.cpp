#include <insituc/debug/backtrace.hpp>

#include <insituc/debug/demangle.hpp>

#include <sstream>
#include <iomanip>
#include <deque>
#include <string>
#include <limits>

#include <cstdint>

#define UNW_LOCAL_ONLY
#include <libunwind.h>

namespace
{

char symbol[1024];

}

trace_type
backtrace()
{
    std::deque< std::string > backtrace_;
    unw_cursor_t cursor;
    unw_context_t context;
    unw_getcontext(&context);
    unw_init_local(&cursor, &context);
    constexpr std::size_t address_width = std::numeric_limits< std::uintptr_t >::digits / 4;
    std::ostringstream os_;
    os_.flags(std::ios_base::hex | std::ios_base::uppercase);
    os_.fill('0');
    while (0 < unw_step(&cursor)) {
        unw_word_t ip = 0;
        unw_get_reg(&cursor, UNW_REG_IP, &ip);
        if (ip == 0) {
            break;
        }
        unw_word_t sp = 0;
        unw_get_reg(&cursor, UNW_REG_SP, &sp);
        os_ << "0x" << std::setw(address_width) << ip;
        os_ << ": (SP:";
        os_ << "0x" << std::setw(address_width) << sp;
        os_ << ") ";
        unw_word_t offset = 0;
        if (unw_get_proc_name(&cursor, symbol, sizeof(symbol), &offset) == 0) {
            os_ << "(" << get_demangled_name(symbol) << " + 0x" << offset << ")\n\n";
        } else {
            os_ << "-- error: unable to obtain symbol name for this frame\n\n";
        }
        if (!!os_) {
            backtrace_.push_back(os_.str());
            backtrace_.back().resize(static_cast< std::size_t >(os_.tellp()));
        }
        os_.clear();
        os_.seekp(0);
    }
    return backtrace_;
}
