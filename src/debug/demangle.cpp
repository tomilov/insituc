#include <insituc/debug/demangle.hpp>

#include <memory>
#include <mutex>
#include <string>

#include <cstdlib>

#include <cxxabi.h>

namespace
{

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wglobal-constructors"
#pragma clang diagnostic ignored "-Wexit-time-destructors"
std::mutex m;
std::unique_ptr< char, decltype(std::free) & > demangled_name{nullptr, std::free};
std::size_t length = 0;
#pragma clang diagnostic pop

}

std::string
get_demangled_name(char const * const symbol) noexcept
{
    if (!symbol) {
        return "<null>";
    }
    std::lock_guard< std::mutex > lock(m);
    int status = -4;
    demangled_name.reset(abi::__cxa_demangle(symbol, demangled_name.release(), &length, &status));
    return ((status == 0) ? demangled_name.get() : symbol);
}
