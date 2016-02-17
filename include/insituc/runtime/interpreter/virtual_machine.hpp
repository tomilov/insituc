#pragma once

#include <insituc/runtime/interpreter/base_types.hpp>
#include <insituc/type_traits.hpp>
#include <insituc/utility/numeric/safe_convert.hpp>
#include <insituc/meta/assembler.hpp>

#include <experimental/optional>
#include <array>
#include <utility>
#include <limits>
#include <stack>
#include <algorithm>
#include <iterator>
#include <functional>
#include <deque>

#include <cassert>

namespace insituc
{
namespace runtime
{

using meta::st;
using meta::mnemocode;

struct virtual_machine
{

    using result_type = bool;

    using data_type = typename meta::assembler::data_type;

    virtual_machine(meta::assembler const & _assembler)
        : assembler_(_assembler)
        , fpregs_(st.depth, std::experimental::nullopt)
        , stack_pointer_(0)
        , frame_pointer_(0)
    { ; }

    template< typename ...arguments >
    result_type
    operator () (size_type const _function, arguments &&... _arguments)
    {
        meta::function const & function_ = assembler_.get_function(_function);
        assert(function_.compiled());
        constexpr size_type arity_ = sizeof...(arguments);
        if (arity_ != function_.arity()) {
            return false;
        }
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
        return operator () (function_, std::array< G, arity_ >{{std::forward< arguments >(_arguments)...}});
#pragma clang diagnostic pop
    }

    template< size_type _arity >
    result_type
    operator () (meta::function const & _function, std::array< G, _arity > && _parameters)
    {
        assert(_function.compiled());
        assert(_function.arity() == _parameters.size());
        assert(!(_parameters.size() < _function.input_));
        assert(!(st.depth < _function.input_));
        size_type const stack_input_ = _parameters.size() - _function.input_;
        auto const beg = std::begin(_parameters);
        assert(is_includes< difference_type >(stack_input_));
        auto const mid = std::next(beg, static_cast< difference_type >(stack_input_));
        auto const end = std::end(_parameters);
        {
            assert(stack_.empty());
            assert(!(stack_.max_size() < _function.frame_clobbered_));
            stack_.resize(_function.frame_clobbered_);
#ifndef NDEBUG
            std::fill(std::begin(stack_), std::end(stack_), std::numeric_limits< G >::quiet_NaN());
#endif
        }
        assert(sanity_check());
        std::move(beg, mid, std::begin(stack_));
        std::move(mid, end, std::begin(fpregs_));
        std::fill_n(std::rbegin(fpregs_), st.depth - _function.input_, std::experimental::nullopt);
        stack_pointer_ = 0;
        stack_used_ = stack_input_;
        if (!interpret_function(_function)) {
            return false;
        }
        assert(stack_pointer_ == _function.climbing_);
        assert(stack_used_ == 0);
        assert(sanity_check());
        {
            assert(stack_.size() == _function.frame_clobbered_);
            stack_.clear();
        }
        assert(!(st.depth < _function.output_));
        auto const first = std::cbegin(fpregs_);
        assert(is_includes< difference_type >(_function.output_));
        auto const last = std::next(first, static_cast< difference_type >(_function.output_));
        auto const bottom = std::cend(fpregs_);
        if (std::any_of(first, last, std::logical_not< fpreg_type >())) {
            return false;
        }
        if (!std::all_of(last, bottom, std::logical_not< fpreg_type >())) {
            return false;
        }
        return true;
    }

    G const &
    get_result(size_type const _offset = 0) const
    {
        return *fpregs_.at(_offset);
    }

private :

    meta::assembler const & assembler_;

    using fpreg_type = std::experimental::optional< G >;
    using fpregs_type = std::deque< fpreg_type >;

    G const indefinite_ = std::numeric_limits< G >::quiet_NaN();

    fpregs_type fpregs_;
    size_type stack_used_;
    size_type stack_pointer_;
    size_type frame_pointer_;
    size_type output_;
    data_type stack_;
    std::stack< size_type > frame_stack_;
    std::stack< meta::function > call_stack_;

    // condition code bits from status word
    bool C0_ = false; // corresponds to CF in flags register
    bool C1_ = false; // not mapped
    bool C2_ = false; // corresponds to PF in flags register
    bool C3_ = false; // corresponds to ZF in flags register

    void
    set_condition_codes(bool const _C0, bool const _C2, bool const _C3)
    {
        C0_ = _C0;
        C2_ = _C2;
        C3_ = _C3;
    }

    bool
    sanity_check() const
    {
        if (fpregs_.size() != st.depth) {
            return false;
        }
        if (frame_pointer_ != 0) {
            return false;
        }
        if (!call_stack_.empty()) {
            return false;
        }
        if (!frame_stack_.empty()) {
            return false;
        }
        return true;
    }

    result_type
    check_head_tail(meta::function const & _function, size_type const _head_size) const;

    result_type
    interpret_function(meta::function const & _function);

    result_type
    interpret(meta::instruction_nullary const & _instruction)
    {
        return interpret(_instruction.mnemocode_);
    }

    result_type
    interpret(meta::instruction_unary const & _instruction)
    {
        return interpret(_instruction.mnemocode_,
                         _instruction.operand_);
    }

    result_type
    interpret(meta::instruction_binary const & _instruction)
    {
        return interpret(_instruction.mnemocode_,
                         _instruction.destination_,
                         _instruction.source_);
    }

    result_type
    interpret(meta::instruction_auxiliary const & _instruction)
    {
        return interpret(_instruction.mnemocode_,
                         _instruction.offset_,
                         _instruction.memory_layout_);
    }

    result_type fxam();
    result_type fcom(mnemocode const _mnemocode, fpreg_type const & _destination, fpreg_type const & _source);
    result_type favoid(G const & _top) const; // return true;
    result_type fcircular(G const & _top);
    result_type fconst(mnemocode const _mnemocode);
    result_type fpremcheck(G const & _destination, G const & _source);
    result_type fsubcheck(G const & _destination, G const & _source) const;
    result_type fdivcheck(G const & _destination, G const & _source) const;
    result_type fxch(fpreg_type & _destination);
    result_type fpush(G const & _value);
    result_type fpush(G && _value);
    result_type fpop();
    result_type fnullary(mnemocode const _mnemocode);
    result_type fcmov(mnemocode const _mnemocode, fpreg_type & _destination, fpreg_type const & _source);
    result_type fbinary(mnemocode const _mnemocode, fpreg_type & _destination, fpreg_type const & _source);

    result_type
    interpret(mnemocode const _mnemocode);
    result_type
    interpret(mnemocode const _mnemocode,
              size_type const _operand);
    result_type
    interpret(mnemocode const _mnemocode,
              size_type const _destination,
              size_type const _source);
    result_type
    interpret(mnemocode const _mnemocode,
              size_type const _offset,
              meta::memory_layout const _memory_layout);

    result_type
    memory_rw_access(mnemocode const _mnemocode,
                     G & _destination);
    result_type
    memory_ro_access(mnemocode const _mnemocode,
                     G const & _source);

};

}
}
