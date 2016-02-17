#pragma once

#include <insituc/meta/instructions.hpp>

#include <algorithm>
#include <utility>
#include <deque>
#include <unordered_set>

#include <cassert>

namespace insituc
{
namespace meta
{

struct function
{

    symbol_type symbol_;        // function name
    symbols_type arguments_;    // function argument

    size_type frame_clobbered_; // the greatest depth of stack used by function overall without taking into account callees
    size_type climbing_;        // during the whole execution of the function stack pointer grows by this value maximal

    size_type input_;           // artificial function can use FPU registers as whole input or as a part of input, in suc a case it requires special handling
    size_type clobbered_;       // total number of clobbered floating-point registers
    size_type output_;          // number of returning values

    std::unordered_set< size_type > callies_;
    code_type code_;

    void
    enter(size_type const _arity,
          size_type const _input = 0)
    {
        assert(empty());
        assert(!(st.depth < _input));
        assert(!(_arity < _input));
        frame_clobbered_ = _arity - input_;
        climbing_ = 0;
        input_ = _input;
        clobbered_ = input_;
        output_ = 0;
    }

    void
    leave(symbol_type _symbol, symbols_type _arguments)
    {
        assert(!empty());
        assert(compiled());
        assert(!(st.depth < output_));
        assert(!(clobbered_ < output_));
        assert(!(clobbered_ < input_));
        assert(_symbol.valid());
        symbol_ = std::move(_symbol);
        arguments_ = std::move(_arguments);
        assert(!(frame_clobbered_ < arity()));
        assert(!(frame_clobbered_ < climbing_));
    }

    bool
    clear()
    {
        symbol_.clear();
        arguments_.clear();
        frame_clobbered_ = 0;
        climbing_ = 0;
        input_ = 0;
        clobbered_ = 0;
        output_ = 0;
        callies_.clear();
        code_.clear();
        return true;
    }

    template< typename F >
    bool
    for_each_instruction(F f) const
    {
        for (instruction const & instruction_ : code_) {
            if (!f(instruction_)) {
                return false;
            }
        }
        return true;
    }

    bool
    operator == (function const & _other) const
    {
        if (!(symbol_ == _other.symbol_)) {
            return false;
        }
        if (frame_clobbered_ != _other.frame_clobbered_) {
            return false;
        }
        if (climbing_ != _other.climbing_) {
            return false;
        }
        if (input_ != _other.input_) {
            return false;
        }
        if (clobbered_ != _other.clobbered_) {
            return false;
        }
        if (output_ != _other.output_) {
            return false;
        }
        if (!(arguments_ == _other.arguments_)) {
            return false;
        }
        if (!(code_ == _other.code_)) {
            return false;
        }
        return true;
    }

    size_type
    arity() const
    {
        return arguments_.size();
    }

    bool
    empty() const
    {
        if (symbol_.valid()) {
            return false;
        }
        if (!arguments_.empty()) {
            return false;
        }
        if (!code_.empty()) {
            return false;
        }
        return true;
    }

    bool
    compiled() const
    {
        return (0 != output_);
    }

    bool
    frameless() const
    {
        assert(!empty());
        assert(compiled());
        return (frame_clobbered_ == 0);
    }

};

using functions_type = std::deque< function >;

}
}
