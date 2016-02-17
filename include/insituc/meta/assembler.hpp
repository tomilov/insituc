#pragma once

#include <insituc/meta/function.hpp>

#include <insituc/variant.hpp>

#include <type_traits>
#include <map>
#include <set>
#include <unordered_set>
#include <deque>
#include <functional>
#include <algorithm>
#include <utility>
#include <limits>
#include <iterator>
#include <stdexcept>
#include <experimental/optional>

#include <cassert>

namespace insituc
{
namespace meta
{

struct assembler
{

    using result_type = bool;

    using data_type = std::deque< G >;

    assembler();

    template< typename F >
    bool
    for_each_function(F f) const
    {
        for (function const & function_ : functions_) {
            if (!f(function_)) {
                return false;
            }
        }
        return true;
    }

    template< typename symbol, typename ...arguments >
    result_type
    enter(symbol && _symbol, size_type const _input, arguments &&... _arguments)
    {
        static_assert(std::is_constructible_v< symbol_type, symbol >, "can't construct function name");
        static_assert(std::is_constructible_v< symbols_type, arguments... >, "can't construct function argument names");
        assert(_symbol.valid()); // TODO: check exact matching using proper regexp
        if (is_dummy_placeholder(_symbol)) {
            return false; // "function name parameter is dummy placeholder"
        }
        if (is_global_variable(_symbol)) {
            return false; // "function name parameter value S is global variable name"
        }
        if (is_function(_symbol)) {
            return false; // "function name parameter value S is name of already defined function"
        }
        assert(local_variables_.empty());
        symbol_ = std::forward< symbol >(_symbol);
        local_variables_ = symbols_type(std::forward< arguments >(_arguments)...);
        monitor_.enter(local_variables_.size(), _input);
        return true;
    }

    template< typename ...assembly >
    result_type
    operator () (assembly &&... _assembly)
    {
        return assemble(std::forward< assembly >(_assembly)...);
    }

    void
    leave()
    {
        assert(brackets_.empty());
        assert(monitor_.arity() == local_variables_.size());
        monitor_.leave(std::move(symbol_), std::move(local_variables_));
        assert(monitor_.check());
        size_type const function_ = functions_.size();
        functions_.push_back(std::move(monitor_));
        export_table_.emplace(functions_.back().symbol_, function_);
    }

    bool
    is_function(symbol_type const & _symbol) const
    {
        return (export_table_.find(_symbol) != export_table_end_);
    }

    bool
    is_current_function(symbol_type const & _symbol) const
    {
        assert(symbol_.valid());
        return (_symbol == symbol_);
    }

    symbol_mapping_type const &
    get_export_table() const
    {
        return export_table_;
    }

    function const &
    get_function(size_type const _function) const
    {
        return functions_.at(_function);
    }

    function const &
    get_function(symbol_type const & _symbol) const
    {
        return get_function(export_table_.at(_symbol));
    }

    size_type
    excess() const
    {
        return monitor_.excess();
    }

    symbol_type const &
    get_dummy_placeholder() const
    {
        return dummy_placeholder_;
    }

    bool
    is_dummy_placeholder(symbol_type const & _symbol) const
    {
        return (_symbol == get_dummy_placeholder());
    }

    bool
    is_reserved_symbol(symbol_type const & _symbol) const
    {
        return (reserved_symbols_.find(_symbol) != std::cend(reserved_symbols_));
    }

    template< typename symbol, typename X = G >
    size_type
    add_global_variable(symbol && _symbol, X && _value = std::numeric_limits< X >::quiet_NaN())
    {
        static_assert(std::is_constructible_v< G, X >);
        assert(!is_global_variable(_symbol));
        assert(!is_reserved_symbol(_symbol));
        assert(!is_dummy_placeholder(_symbol));
        assert(!is_function(_symbol));
        size_type position_ = heap_.size();
        heap_.emplace_back(std::forward< X >(_value));
        global_offsets_.emplace(position_, (*global_varibles_.emplace(std::forward< symbol >(_symbol), position_).first).first);
        return position_;
    }

    bool
    is_global_variable(symbol_type const & _symbol) const
    {
        return (global_varibles_.find(_symbol) != std::cend(global_varibles_));
    }

    bool
    is_global_variable(size_type const _offset) const
    {
        return (global_offsets_.find(_offset) != std::cend(global_offsets_));
    }

    template< typename X = G >
    size_type
    set_globlal_variable(symbol_type const & _symbol, X && _value = X{})
    {
        static_assert(std::is_constructible_v< G, X >);
        size_type const offset_ = global_varibles_.at(_symbol);
        assert(is_global_variable(_symbol));
        heap_[offset_] = std::forward< X >(_value);
        return offset_;
    }

    template< typename X = G >
    void
    set_globlal_variable(size_type const _offset, X && _value = X{})
    {
        static_assert(std::is_constructible_v< G, X >);
        assert(is_global_variable(_offset));
        heap_[_offset] = std::forward< X >(_value);
    }

    symbol_mapping_type const &
    get_heap_symbols() const
    {
        return global_varibles_;
    }

    G &
    get_global_variable(symbol_type const & _symbol) const
    {
        assert(is_global_variable(_symbol));
        return heap_[global_varibles_.at(_symbol)];
    }

    G &
    get_global_variable(size_type const _offset) const
    {
        assert(is_global_variable(_offset));
        return heap_[_offset];
    }

    size_type
    get_heap_size() const
    {
        return heap_.size();
    }

    G const &
    get_heap_element(size_type const _offset) const
    {
        assert(_offset < heap_.size());
        return heap_[_offset];
    }

    std::unordered_set< size_type > const &
    get_callies(size_type const _caller) const
    {
        return get_function(_caller).callies_;
    }

    size_type
    get_stack_size() const
    {
        assert(!functions_.empty());
        size_type stack_size_ = 0;
        for (function const & function_ : functions_) {
            if (stack_size_ < function_.frame_clobbered_) {
                stack_size_ = function_.frame_clobbered_;
            }
        }
        return stack_size_;
    }

    bool
    clear()
    {
        functions_.clear();
        export_table_.clear();
        heap_.clear();
        literals_.clear();
        global_varibles_.clear();
        global_offsets_.clear();
        symbol_.clear();
        brackets_.clear();
        local_variables_.clear();
        return monitor_.clear();
    }

    bool
    empty() const
    {
        return functions_.empty();
    }

private :

    symbol_type dummy_placeholder_;
    symbol_set_type reserved_symbols_;

    functions_type functions_;
    symbol_mapping_type export_table_;
    typename symbol_mapping_type::iterator export_table_end_ = std::end(export_table_);

    mutable data_type heap_; // actually literals algorithmically write protected in `get_global_variable(...) const`
    std::map< std::reference_wrapper< G const >, size_type const, std::less< G > > literals_;
    symbol_mapping_type global_varibles_;
    std::map< size_type, std::reference_wrapper< symbol_type const > > global_offsets_;

    symbol_type symbol_;
    std::deque< size_type > brackets_;
    symbols_type local_variables_;

    template< typename symbol >
    bool
    add_reserved_symbol(symbol && _symbol)
    {
        static_assert(std::is_assignable_v< string_type &, symbol >, "can't construct reserved symbol name");
        assert(functions_.empty());
        symbol_type reserved_symbol_;
        reserved_symbol_.symbol_.name_ = std::forward< symbol >(_symbol);
        assert(!is_dummy_placeholder(reserved_symbol_));
        return reserved_symbols_.insert(std::move(reserved_symbol_)).second;
    }

    template< typename symbol >
    size_type
    add_local_variable(symbol && _symbol)
    {
        static_assert(std::is_constructible_v< symbol_type, symbol >, "can't construct local variable name");
        assert(!is_dummy_placeholder(_symbol)); // "cannot add local variable, because specified name is already used as dummy placeholder"
        assert(!is_reserved_symbol(_symbol));   // "cannot add local variable, because specified name is reserved symbol"
        assert(!is_top_level_local_variable(_symbol)); // "cannot add local variable, because specified name is already used in current scope"
        size_type offset_ = local_variables_.size();
        local_variables_.emplace_back(std::forward< symbol >(_symbol));
        return offset_;
    }

    bool
    is_local_variable(symbol_type const & _symbol) const;

    bool
    is_top_level_local_variable(symbol_type const & _symbol) const;

    size_type
    local_variable_offset(symbol_type const & _symbol) const;

    result_type
    call(symbol_type const & _symbol);

    result_type
    memory_rw_access(mnemocode const _mnemocode, symbol_type const & _symbol);

    template< typename symbol >
    result_type
    resolve_symbol(mnemocode const _mnemocode, symbol && _symbol)
    {
        static_assert(std::is_constructible_v< symbol_type, symbol >);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-enum"
        switch (_mnemocode) {
#pragma clang diagnostic pop
        case mnemocode::call : {
            return call(_symbol);
        }
        case mnemocode::fst :
        case mnemocode::fstp :
        case mnemocode::fld :
        case mnemocode::fadd :
        case mnemocode::fsub :
        case mnemocode::fsubr :
        case mnemocode::fmul :
        case mnemocode::fdiv :
        case mnemocode::fdivr :
        case mnemocode::fcom :
        case mnemocode::fcomp : {
            return memory_rw_access(_mnemocode, _symbol);
        }
        case mnemocode::alloca_ : {
            if (is_reserved_symbol(_symbol)) {
                return false;
            }
            if (is_top_level_local_variable(_symbol)) {
                return false;
            } else if (is_dummy_placeholder(_symbol)) {
                if (!monitor_(mnemocode::fstp, st)) {
                    return false;
                }
                return true;
            } else {
                size_type const offset_ = add_local_variable(std::forward< symbol >(_symbol));
                if (!monitor_(mnemocode::alloca_, offset_, memory_layout::stack)) {
                    return false;
                }
                return true;
            }
        }
        default : {
            break;
        }
        }
        return false;
    }

    template< typename X >
    size_type
    literal_offset(X && _literal) // appends to heap if not exists
    {
        static_assert(std::is_constructible_v< G, X >);
        auto literal_ = literals_.find(_literal);
        if (literal_ == std::end(literals_)) {
            size_type const position_ = heap_.size();
            heap_.emplace_back(std::forward< X >(_literal));
            literal_ = literals_.emplace(heap_.back(), position_).first;
        }
        return literal_->second;
    }

    template< typename X >
    result_type
    resolve_literal(mnemocode const _mnemocode, X && _literal)
    {
        static_assert(std::is_constructible_v< G, X >);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wfloat-equal"
        if (_literal == zero) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-enum"
            switch (_mnemocode) {
#pragma clang diagnostic pop
            case mnemocode::fld : {
                if (!monitor_(mnemocode::fldz)) {
                    return false;
                }
                return true;
            }
            case mnemocode::fadd :
            case mnemocode::fsub :
            case mnemocode::fsubr :
            case mnemocode::fmul :
            case mnemocode::fdiv :
            case mnemocode::fdivr : {
                if (!monitor_(mnemocode::fldz)) {
                    return false;
                }
                if (!monitor_(_mnemocode)) {
                    return false;
                }
                return true;
            }
            case mnemocode::fcom : {
                if (!monitor_(mnemocode::ftst)) {
                    return false;
                }
                return true;
            }
            case mnemocode::fcomp : {
                if (!monitor_(mnemocode::ftst)) {
                    return false;
                }
                if (!monitor_(mnemocode::fstp, st)) {
                    return false;
                }
                return true;
            }
            default : {
                return false;
            }
            }
        } else if (_literal == one) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-enum"
            switch (_mnemocode) {
#pragma clang diagnostic pop
            case mnemocode::fld : {
                if (!monitor_(mnemocode::fld1)) {
                    return false;
                }
                return true;
            }
            case mnemocode::fadd :
            case mnemocode::fsub :
            case mnemocode::fsubr :
            case mnemocode::fmul :
            case mnemocode::fdiv :
            case mnemocode::fdivr :
            case mnemocode::fcom :
            case mnemocode::fcomp : {
                if (!monitor_(mnemocode::fld1)) {
                    return false;
                }
                if (!monitor_(_mnemocode)) {
                    return false;
                }
                return true;
            }
            default : {
                return false;
            }
            }
        } else {
            size_type const offset_ = literal_offset(std::forward< X >(_literal));
            if (!monitor_(_mnemocode, offset_, memory_layout::heap)) {
                return false;
            }
            return true;
        }
#pragma clang diagnostic pop
    }

    result_type
    filter_brackets(mnemocode const _mnemocode);

    result_type
    assemble() const
    {
        return true;
    }

    template< typename ...tail >
    result_type
    assemble(mnemocode const _mnemocode,
             tail &&... _tail)
    {
        if (!filter_brackets(_mnemocode)) {
            return false;
        }
        return assemble(std::forward< tail >(_tail)...);
    }

    template< typename ...tail >
    result_type
    assemble(mnemocode const _mnemocode,
             size_type const _operand,
             tail &&... _tail)
    {
        if (!monitor_(_mnemocode, _operand)) {
            return false;
        }
        return assemble(std::forward< tail >(_tail)...);
    }

    template< typename ...tail >
    result_type
    assemble(mnemocode const _mnemocode,
             size_type const _destination,
             size_type const _source,
             tail &&... _tail)
    {
        if (!monitor_(_mnemocode, _destination, _source)) {
            return false;
        }
        return assemble(std::forward< tail >(_tail)...);
    }

    template< typename ...tail >
    result_type
    assemble(mnemocode const _mnemocode,
             st_type const _operand,
             tail &&... _tail)
    {
        if (!monitor_.access_top(_mnemocode, _operand)) {
            return false;
        }
        return assemble(std::forward< tail >(_tail)...);
    }

    template< typename ...tail >
    result_type
    assemble(mnemocode const _mnemocode,
             st_type const _destination,
             st_type const _source,
             tail &&... _tail)
    {
        if (!monitor_(_mnemocode, _destination, _source)) {
            return false;
        }
        return assemble(std::forward< tail >(_tail)...);
    }

    template< typename symbol, typename ...tail >
    std::enable_if_t< std::is_same_v< symbol_type, std::decay_t< symbol > >, result_type >
    assemble(mnemocode const _mnemocode,
             symbol && _symbol,
             tail &&... _tail)
    {
        if (!resolve_symbol(_mnemocode, std::forward< symbol >(_symbol))) {
            return false;
        }
        return assemble(std::forward< tail >(_tail)...);
    }

    template< typename X, typename ...tail >
    std::enable_if_t< std::is_same_v< G, std::decay_t< X > >, result_type >
    assemble(mnemocode const _mnemocode,
             X && _value,
             tail &&... _tail)
    {
        static_assert(std::is_constructible_v< G, X >);
        if (!resolve_literal(_mnemocode, std::forward< X >(_value))) {
            return false;
        }
        return assemble(std::forward< tail >(_tail)...);
    }

    struct monitor
    {

        monitor(assembler const & _assembler)
            : assembler_(_assembler)
        { ; }

        bool
        clear()
        {
            return function_.clear();
        }

        void
        enter(size_type const _arity, size_type const _input);

        size_type
        excess() const;

        void
        leave(symbol_type _symbol, symbols_type _arguments)
        {
            assert(!function_.empty());
            assert(function_.compiled());
            return function_.leave(std::move(_symbol), std::move(_arguments));
        }

        operator function () &&
        {
            assert(!function_.empty());
            assert(function_.compiled());
            return std::move(function_);
        }

        result_type
        check();

        template< typename ...operands >
        result_type
        operator () (mnemocode const _mnemocode,
                     operands const... _operands)
        {
            if (!verify(_mnemocode, _operands...)) {
                return false;
            }
            function_.code_.emplace_back(in_place<>, _mnemocode, _operands...);
            return true;
        }

        result_type
        operator () (instruction const & _instruction)
        {
            if (!visit([&] (auto const & i) -> result_type { return verify(i); }, _instruction)) {
                return false;
            }
            function_.code_.push_back(_instruction);
            return true;
        }

        size_type
        local_variables_count() const
        {
            return local_variables_count_;
        }

        size_type
        arity() const
        {
            return arity_;
        }

        result_type
        access_top(mnemocode const _mnemocode, size_type const _offset);

    private :

        assembler const & assembler_;

        function function_;
        size_type arity_;

        size_type used_;                  // current number of used floating-point registers

        size_type local_variables_count_; // current number of local variables placed onto stack
        size_type frame_used_;            // number of values (temporary values inclusive) placed onto stack at the moment
        size_type stack_pointer_;         // offset of the stack pointer in the stack frame of a current function

        bool
        is_valid_offset(size_type const _offset) const
        {
            return (_offset < excess());
        }

        bool
        is_near_offset(size_type const _offset) const
        {
            return (_offset < used_);
        }

        size_type
        get_far_offset(size_type const _offset) const
        {
            assert(is_valid_offset(_offset));
            assert(!is_near_offset(_offset));
            return (frame_used_ + used_) - (_offset + 1);
        }

        result_type
        verify(instruction_nullary const & _instruction)
        {
            return verify(_instruction.mnemocode_);
        }

        result_type
        verify(instruction_unary const & _instruction)
        {
            return verify(_instruction.mnemocode_,
                          _instruction.operand_);
        }

        result_type
        verify(instruction_binary const & _instruction)
        {
            return verify(_instruction.mnemocode_,
                          _instruction.destination_,
                          _instruction.source_);
        }

        result_type
        verify(instruction_auxiliary const & _instruction)
        {
            return verify(_instruction.mnemocode_,
                          _instruction.offset_,
                          _instruction.memory_layout_);
        }

        result_type
        verify(mnemocode const _mnemocode);

        result_type
        verify(mnemocode const _mnemocode,
               size_type const _operand);

        result_type
        verify(mnemocode const _mnemocode,
               size_type const _destination,
               size_type const _source);

        result_type
        verify(mnemocode const _mnemocode,
               size_type const _offset,
               memory_layout const _memory_layout);

        result_type
        verify_heap_access(mnemocode const _mnemocode, size_type const _offset);

        result_type
        verify_stack_access(mnemocode const _mnemocode, size_type const _offset);

        result_type
        verify_memory_access(mnemocode const _mnemocode);

        result_type
        update_stack_depth(size_type const _additional = 0);

        result_type
        adjust_stack_pointer(function const & _callee);

        result_type
        adjust(size_type const _consumed,
               size_type const _produced,
               size_type const _clobbered);

        result_type
        adjust(size_type const _consumed,
               size_type const _produced)
        {
            return adjust(_consumed, _produced, std::max(_consumed, _produced));
        }

        result_type
        adjust(size_type const _clobbered)
        {
            return adjust(_clobbered, _clobbered, _clobbered);
        }

        result_type
        adjust(function const & _callee)
        {
            size_type const callee_clobbered_ = _callee.frameless() ? _callee.clobbered_ : st.depth; // if the callee uses frame, than we should to deny the using of the function as frameless in any context in a callers further
            if (!adjust(_callee.input_, _callee.output_, callee_clobbered_)) {
                return false;
            }
            if (!adjust_stack_pointer(_callee)) {
                return false;
            }
            return true;
        }

    } monitor_;

};

}
}
