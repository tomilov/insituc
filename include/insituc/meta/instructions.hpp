#pragma once

#include <insituc/variant.hpp>
#include <insituc/meta/mnemocodes.hpp>
#include <insituc/utility/static_const.hpp>

#include <versatile.hpp>

#include <type_traits>
#include <utility>
#include <deque>

namespace insituc
{
namespace meta
{

struct st_type
{

    static constexpr size_type depth = 010;

    constexpr
    st_type() noexcept
        : offset_(0)
    { ; }

    constexpr
    st_type(st_type const &) noexcept = default;

    constexpr
    st_type(st_type &&) noexcept = default;

    template< typename offset >
    constexpr
    st_type
    operator () (offset && _offset) const noexcept
    {
        static_assert(std::is_integral_v< std::decay_t< offset > >);
        return static_cast< size_type >(_offset);
    }

    constexpr
    operator size_type () const noexcept
    {
        return offset_;
    }

private :

    size_type const offset_;

    constexpr
    st_type(size_type const _offset) noexcept
        : offset_(_offset)
    {
        assert(offset_ < depth);
    }

};

namespace
{

constexpr st_type const & st = static_const< st_type >;

}

enum class memory_layout
{
    stack, // relative addressing
    heap   // absolute addressing
};

constexpr
char_type const *
c_str(memory_layout const _memory_layout) noexcept
{
    switch (_memory_layout) {
    case memory_layout::stack : return "[l]";
    case memory_layout::heap  : return "[g]";
    }
}

struct instruction_nullary
{

    constexpr
    instruction_nullary(mnemocode const _mnemocode) noexcept
        : mnemocode_(_mnemocode)
    { ; }

    mnemocode const mnemocode_;

    constexpr
    bool
    operator == (instruction_nullary const & _other) const noexcept
    {
        if (!(mnemocode_ == _other.mnemocode_)) {
            return false;
        }
        return true;
    }

};

struct instruction_unary
{

    constexpr
    instruction_unary(mnemocode const _mnemocode,
                      size_type const _operand) noexcept
        : mnemocode_(_mnemocode)
        , operand_(_operand)
    { ; }

    mnemocode const mnemocode_;
    size_type const operand_;

    constexpr
    bool
    operator == (instruction_unary const & _other) const noexcept
    {
        if (!(mnemocode_ == _other.mnemocode_)) {
            return false;
        }
        if (!(operand_ == _other.operand_)) {
            return false;
        }
        return true;
    }

};

struct instruction_binary
{

    constexpr
    instruction_binary(mnemocode const _mnemocode,
                       size_type const _destination,
                       size_type const _source) noexcept
        : mnemocode_(_mnemocode)
        , destination_(_destination)
        , source_(_source)
    { ; }

    mnemocode const mnemocode_;
    size_type const destination_;
    size_type const source_;

    constexpr
    bool
    operator == (instruction_binary const & _other) const noexcept
    {
        if (!(mnemocode_ == _other.mnemocode_)) {
            return false;
        }
        if (!(destination_ == _other.destination_)) {
            return false;
        }
        if (!(source_ == _other.source_)) {
            return false;
        }
        return true;
    }

};

struct instruction_auxiliary
{

    constexpr
    instruction_auxiliary(mnemocode const _mnemocode,
                          size_type const _offset,
                          memory_layout const _memory_layout) noexcept
        : mnemocode_(_mnemocode)
        , offset_(_offset)
        , memory_layout_(_memory_layout)
    { ; }

    mnemocode const mnemocode_;
    size_type const offset_;
    memory_layout const memory_layout_;

    constexpr
    bool
    operator == (instruction_auxiliary const & _other) const noexcept
    {
        if (!(mnemocode_ == _other.mnemocode_)) {
            return false;
        }
        if (!(offset_ == _other.offset_)) {
            return false;
        }
        if (!(memory_layout_ == _other.memory_layout_)) {
            return false;
        }
        return true;
    }

};

using instruction = versatile<
instruction_nullary,
instruction_unary,
instruction_binary,
instruction_auxiliary
>;

using code_type = std::deque< instruction >;

}
}
