#pragma once

#include <insituc/memory/xallocator.hpp>

#include <insituc/runtime/jit_compiler/base_types.hpp>

#include <type_traits>
#include <utility>
#include <array>
#include <vector>
#include <deque>
#include <new>

#include <cstdint>
#include <cassert>

namespace insituc
{
namespace runtime
{

struct instance
{

    using code_type = std::vector< byte_type, xallocator< byte_type > >;
    using data_type = std::vector< F >;

    code_type code_;
    data_type heap_;
    data_type stack_;

    std::deque< size_type > entry_points_;

#if defined(__has_feature)
# if __has_feature(address_sanitizer)
    [[gnu::no_sanitize("address")]]
# endif
#endif
    F
    execute(size_type const _entry_point)
    {
        assert(_entry_point < code_.size());
        volatile F result_{};
        asm volatile ("call *%1"
                      : "=&t"(result_)
                      : "a"(code_.data() + _entry_point), "c"(stack_.data()), "d"(heap_.data())
                      : "memory", // stack_/heap_ access
                      "cc",       // sahf instruction
                      "%st(1)", "%st(2)", "%st(3)", "%st(4)", "%st(5)", "%st(6)", "%st(7)"
                      );
        return result_;
    }

    F
    operator () (size_type const _function)
    {
        return execute(entry_points_.at(_function));
    }

    template< typename ...arguments >
    F
    operator () (size_type const _function, arguments &&... _arguments)
    {
        constexpr size_type N = sizeof...(arguments);
        assert(!(stack_.size() < N));
        using A = F [N];
        ::new (static_cast< void * >(stack_.data())) A{static_cast< F >(std::forward< arguments >(_arguments))...};
        return operator () (_function);
    }

};

}
}
