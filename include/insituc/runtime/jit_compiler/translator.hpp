#pragma once

#include <insituc/runtime/jit_compiler/instance.hpp>
#include <insituc/meta/assembler.hpp>
#include <insituc/variant.hpp>

#include <type_traits>
#include <utility>
#include <functional>
#include <limits>

#include <cstdint>
#include <cassert>

namespace insituc
{
namespace runtime
{

constexpr
byte_type
operator ""_o (unsigned long long int _value) noexcept
{
    return static_cast< byte_type >(_value);
}

using meta::memory_layout;
using meta::mnemocode;

struct translator
{

    using result_type = bool;

    operator instance () &&
    {
        return std::move(instance_);
    }

    result_type
    operator () (meta::assembler const & _assembler)
    {
        assert(instance_.code_.empty());
        assert(instance_.heap_.empty());
        assert(instance_.stack_.empty());
        if (!_assembler.for_each_function([&] (meta::function const & _function) -> result_type { return translate_function(_function); })) {
            return false;
        }
        size_type const heap_size_ = _assembler.get_heap_size();
        instance_.heap_.reserve(heap_size_);
        for (size_type i = 0; i < heap_size_; ++i) {
            instance_.heap_.push_back(static_cast< F >(_assembler.get_heap_element(i)));
        }
        instance_.stack_.resize(_assembler.get_stack_size(), std::numeric_limits< F >::quiet_NaN());
        return true;
    }

private :

    instance instance_;
    size_type stack_pointer_;

    result_type
    translate_function(meta::function const & _function)
    {
        assert(_function.compiled());
        instance_.entry_points_.push_back(instance_.code_.size());
        stack_pointer_ = 0;
        if (!_function.for_each_instruction(visit([&] (auto const & i) -> result_type { return translate(i); }))) {
            return false;
        }
        assert(stack_pointer_ == _function.climbing_);
        return true;
    }

    using near_type = std::int8_t;
    using far_type = std::int32_t;

    using ufar_type = typename std::make_unsigned< far_type >::type;

    enum class register_name
    {
        a, c, d, b,
        sp, bp,
        si, di
    };

    constexpr
    byte_type
    rm(register_name const _base) noexcept
    {
        switch (_base) {
        case register_name::a  : return 0b000_o;
        case register_name::c  : return 0b001_o;
        case register_name::d  : return 0b010_o;
        case register_name::b  : return 0b011_o;
        case register_name::sp : return 0b100_o;
        case register_name::bp : return 0b101_o;
        case register_name::si : return 0b110_o;
        case register_name::di : return 0b111_o;
        }
    }

    result_type
    append(byte_type const _opcode)
    {
        instance_.code_.push_back(_opcode);
        return true;
    }

    template< typename ...tail >
    result_type
    append(byte_type const _opcode, tail &&... _tail)
    {
        if (!append(_opcode)) {
            return false;
        }
        return append(std::forward< tail >(_tail)...);
    }

    result_type
    translate(meta::instruction_nullary const & _instruction)
    {
        return translate(_instruction.mnemocode_);
    }

    result_type
    translate(meta::instruction_unary const & _instruction)
    {
        return translate(_instruction.mnemocode_,
                         _instruction.operand_);
    }

    result_type
    translate(meta::instruction_binary const & _instruction)
    {
        return translate(_instruction.mnemocode_,
                         _instruction.destination_,
                         _instruction.source_);
    }

    result_type
    translate(meta::instruction_auxiliary const & _instruction)
    {
        return translate(_instruction.mnemocode_,
                         _instruction.offset_,
                         _instruction.memory_layout_);
    }

    result_type
    translate(mnemocode const _mnemocode);

    template< typename displacement >
    result_type
    add_displacement(displacement const & _displacement)
    {
        static_assert(std::is_integral_v< displacement >);
        using U = typename std::make_unsigned< displacement >::type;
        constexpr size_type digits_ = std::numeric_limits< byte_type >::digits;
        U const & offset_ = reinterpret_cast< U const & >(_displacement);
        for (size_type i = 0; i < sizeof(U) / sizeof(byte_type); ++i) {
            instance_.code_.push_back(byte_type(offset_ >> (digits_ * i)));
        }
        return true;
    }

    result_type
    add(register_name const _base, ufar_type const _delta);
    result_type
    sub(register_name const _base, ufar_type const _delta);
    result_type
    affect_stack_pointer(mnemocode const _mnemocode,
                         size_type const _offset);
    result_type
    unary(mnemocode const _mnemocode,
          size_type const _operand);
    result_type
    translate(mnemocode const _mnemocode,
              size_type const _operand);

    result_type
    fsource(mnemocode const _mnemocode,
            size_type const _source);
    result_type
    fdestination(mnemocode const _mnemocode,
                 size_type const _destination);
    result_type
    fbinary(mnemocode const _mnemocode,
            size_type const _destination,
            size_type const _source);
    result_type
    translate(mnemocode const _mnemocode,
              size_type const _offset,
              memory_layout const _memory_layout);

    result_type
    translate(mnemocode const _mnemocode,
              size_type const _destination,
              size_type const _source);

    result_type
    inderect_address(register_name const _base, bool const _increase, size_type const _displacement);
    result_type
    heap_access(mnemocode const _mnemocode, size_type const _offset);
    result_type
    stack_access(mnemocode const _mnemocode, size_type const _offset);
    result_type
    memory_access(mnemocode const _mnemocode);

};

} // namespace insituc::runtime
} // namespace insituc
