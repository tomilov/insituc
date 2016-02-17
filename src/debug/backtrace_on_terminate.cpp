#include <insituc/debug/backtrace.hpp>
#include <insituc/debug/demangle.hpp>

#include <iostream>
#include <type_traits>
#include <exception>
#include <memory>
#include <memory>
#include <typeinfo>

#include <cstdlib>

#include <cxxabi.h>

namespace
{

[[noreturn]]
void
backtrace_on_terminate() noexcept;

static_assert(std::is_same_v< std::terminate_handler, decltype(&backtrace_on_terminate) >);

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wglobal-constructors"
#pragma clang diagnostic ignored "-Wexit-time-destructors"
std::mutex m;
std::unique_ptr< std::remove_pointer_t< std::terminate_handler >, decltype(std::set_terminate) & > terminate_handler{std::set_terminate(backtrace_on_terminate), std::set_terminate};
#pragma clang diagnostic pop

[[noreturn]]
void
backtrace_on_terminate() noexcept
{
    {
        std::lock_guard< std::mutex > lock(m);
        std::set_terminate(terminate_handler.release()); // to avoid infinite looping if any
    }
    for (auto const & backtrace_ : backtrace()) {
        std::clog << backtrace_ << std::endl;
    }
    if (std::exception_ptr ep = std::current_exception()) {
        try {
            std::rethrow_exception(ep);
        } catch (std::exception const & e) {
            std::clog << "backtrace: unhandled exception derived from std::exception: " << e.what() << std::endl;
        } catch (...) {
            if (std::type_info * et = abi::__cxa_current_exception_type()) {
                std::clog << "backtrace: unhandled exception type: " << get_demangled_name(et->name()) << std::endl;
            } else {
                std::clog << "backtrace: unhandled unknown exception" << std::endl;
            }
        }
    }
    std::_Exit(EXIT_FAILURE);
}

}
