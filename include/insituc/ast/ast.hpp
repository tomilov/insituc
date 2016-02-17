#pragma once

#include <insituc/base_types.hpp>
#include <insituc/variant.hpp>
#include <insituc/type_traits.hpp>
#include <insituc/ast/tokens.hpp>
#include <insituc/utility/head.hpp>

#include <boost/mpl/list.hpp>

#include <type_traits>
#include <utility>
#include <tuple>
#include <memory>
#include <functional>
#include <iterator>
#include <deque>
#include <list>
#include <set>
#include <limits>

#include <cstddef>
#include <cassert>

namespace insituc
{
namespace ast
{

template< typename visitable, typename type >
constexpr auto index_at = visitable::template index_at_t< type >::value;

template< typename type, typename visitable >
type
get(visitable && _visitable) noexcept
{
    assert(_visitable.template active< type >());
    return static_cast< type && >(static_cast< type & >(*_visitable));
}

template< typename type, typename visitable >
type *
get(visitable * _visitable) noexcept
{
    static_assert(!std::is_reference_v< type >);
    if (!_visitable->template active< type >()) {
        return nullptr;
    }
    return &static_cast< type & >(**_visitable);
}

template< typename type, typename visitable >
type const *
get(visitable const * _visitable) noexcept
{
    static_assert(!std::is_reference_v< type >);
    if (!_visitable->template active< type >()) {
        return nullptr;
    }
    return &static_cast< type const & >(**_visitable);
}

template< typename >
struct make_type_list; // for boost::spirit variant

template< template< typename ...types > typename visitable, typename ...types >
struct make_type_list< visitable< types... > >
{

    using type = typename boost::mpl::list< unwrap_type_t< types >... >::type;

};

template< typename type >
using make_type_list_t = typename make_type_list< type >::type;

struct program;
struct entry_definition;
struct statement_block;
struct statement;
struct assignment;
struct variable_declaration;
struct entry_substitution;
struct intrinsic_invocation;
struct expression;
struct unary_expression;
struct binary_expression;
struct lvalue_list;
struct rvalue_list;
struct operand;
struct identifier;
struct empty;

constexpr size_type ntag = std::numeric_limits< size_type >::max();

template< typename type, typename = void >
struct is_tagged
        : std::false_type
{

};

template< typename type >
struct is_tagged< type, std::void_t< decltype(type::tag_) > >
        : std::is_same< decltype(type::tag_), size_type >
{

};

template< typename type >
constexpr bool is_tagged_v = is_tagged< type >::value;

struct pragma
{

    string_type record_;

    size_type tag_ = ntag;

};

struct empty
{

};

struct symbol
{

    string_type name_;

    bool
    operator == (symbol const & _rhs) const
    {
        return (name_ == _rhs.name_);
    }

    bool
    operator < (symbol const & _rhs) const
    {
        return (name_ < _rhs.name_);
    }

};

using symbol_cref = std::reference_wrapper< symbol const >;

using symbols = std::deque< symbol >;

struct identifier
{

    symbol symbol_;
    symbols wrts_; // Variables, with respect to which the derivative is taken.

    void
    clear()
    {
        symbol_.name_.clear();
        wrts_.clear();
    }

    bool
    valid() const
    {
        if (symbol_.name_.empty()) {
            return false;
        }
        for (symbol const & wrt_ : wrts_) {
            if (wrt_.name_.empty()) {
                return false;
            }
        }
        return true;
    }

    void
    derive(symbol const & _wrt)
    {
        wrts_.push_back(_wrt);
    }

    void
    derive(symbols const & _wrts)
    {
        for (symbol const & wrt_ : _wrts) {
            derive(wrt_);
        }
    }

    bool
    is_original() const
    {
        return wrts_.empty();
    }

    bool
    operator == (identifier const & _rhs) const
    {
        if (!(symbol_ == _rhs.symbol_)) {
            return false;
        }
        if (!(wrts_ == _rhs.wrts_)) {
            return false;
        }
        return true;
    }

    bool
    operator < (identifier const & _rhs) const
    {
        if (symbol_ == _rhs.symbol_) {
            return (wrts_ < _rhs.wrts_);
        } else {
            return (symbol_ < _rhs.symbol_);
        }
    }

    size_type tag_ = ntag;

};

static_assert(is_tagged_v< identifier >, "identifier intended to be tagged");

using lvalue = identifier;

using lvalues = std::deque< lvalue >;

struct lvalue_list
{

    lvalues lvalues_;

    size_type tag_ = ntag;

};

struct operand;

struct intrinsic_invocation;
struct entry_substitution;
struct unary_expression;
struct expression;
struct binary_expression;

using rvalue = operand;

struct rvalue_list;

using operand_cptr = operand const *;

struct operand
        : variant<
        ast::empty,
        G,
        constant,
        recursive_wrapper< intrinsic_invocation >,
        recursive_wrapper< entry_substitution >,
        identifier,
        recursive_wrapper< unary_expression >,
        recursive_wrapper< binary_expression >,
        recursive_wrapper< expression >,
        recursive_wrapper< rvalue_list >,
        operand_cptr
        >
{

    using base = typename operand::types_t;

    // boost spirit contract
    struct adapted_variant_tag;
    using types = make_type_list_t< base >;

    using base::base;
    using base::operator =;

    operand() = default;

    operand(operand const &) = default;
    operand(operand &) = default;
    operand(operand && _rhs) = default;

    operand & operator = (operand const &) = default;
    operand & operator = (operand &) = default;
    operand & operator = (operand && _rhs) = default;

    base const &
    operator * () const
    {
        return *this;
    }

    base &
    operator * ()
    {
        return *this;
    }

    bool
    empty() const noexcept
    {
        return active< ast::empty >();
    }

};

inline
operand const &
unref(operand_cptr const _operand_cptr) noexcept;

inline
operand const &
unref(operand const & _operand) noexcept
{
    if (auto const * const operand_cptr_ = get< operand_cptr >(&_operand)) {
        return unref(*operand_cptr_);
    }
    return _operand;
}

inline
operand const &
unref(operand_cptr const _operand_cptr) noexcept
{
    return unref(*_operand_cptr);
}

inline
operand
unref(operand && _operand)
{
    if (auto const * const operand_cptr_ = get< operand_cptr >(&_operand)) {
        return unref(*operand_cptr_);
    }
    return std::move(_operand);
}

template< typename ...nodes >
bool
is_trivially_copy_constructible(variant< nodes... > const & _nodes) noexcept
{
    using A = bool const [sizeof...(nodes)];
    return A{std::is_trivially_copy_constructible_v< unwrap_type_t< nodes > >...}[sizeof...(nodes) - _nodes.which()];
}

inline
operand
clone_or_ref(operand const & _operand)
{
    operand const & operand_ = unref(_operand);
    if (is_trivially_copy_constructible(operand_)) { // copy-construction is cheap operation
        return operand_;
    }
    return &operand_;
}

struct unary_expression
{

    unary operator_;
    operand operand_;

};

using rvalues = std::deque< rvalue >;

struct rvalue_list
{

    rvalues rvalues_;
    pragma pragma_ = {}; // parse first

    size_type tag_ = ntag;

};

inline
void
append_rvalues(rvalue _from, rvalues & _to)
{
    if (auto * rvalue_list_ = get< rvalue_list >(&_from)) {
        for (rvalue & rvalue_ : rvalue_list_->rvalues_) {
            _to.push_back(std::move(rvalue_));
        }
    } else {
        _to.push_back(std::move(_from));
    }
}

struct operation
{

    binary operator_;
    operand operand_;

};

using operation_list = std::deque< operation >;

struct expression
{

    operand first_;
    operation_list rest_;

    size_type tag_ = ntag;

};

struct binary_expression
{

    operand lhs_;
    binary operator_;
    operand rhs_;

};

struct intrinsic_invocation
{

    intrinsic intrinsic_;
    rvalue_list argument_list_;

    size_type tag_ = ntag;

};

struct entry_substitution
{

    identifier entry_name_;
    rvalue_list argument_list_;

    size_type tag_ = ntag;

};

struct variable_declaration
{

    lvalue_list lhs_;
    rvalue_list rhs_;

    size_type tag_ = ntag;

};

struct assignment
{

    lvalue_list lhs_;
    assign operator_;
    rvalue_list rhs_;

    size_type tag_ = ntag;

};

struct statement_block;

struct statement
        : variant<
        ast::empty,
        variable_declaration,
        assignment,
        recursive_wrapper< statement_block >
        >
{

    using base = typename statement::types_t;

    // boost spirit contract
    struct adapted_variant_tag;
    using types = make_type_list_t< base >;

    using base::base;
    using base::operator =;

    statement() = default;

    statement(statement const &) = default;
    statement(statement &) = default;
    statement(statement && _rhs) = default;

    statement & operator = (statement const &) = default;
    statement & operator = (statement &) = default;
    statement & operator = (statement && _rhs) = default;

    base const &
    operator * () const
    {
        return *this;
    }

    base &
    operator * ()
    {
        return *this;
    }

    bool
    empty() const noexcept
    {
        return active< ast::empty >();
    }

};

using statements = std::deque< statement >;

struct statement_block
{

    statements statements_;

    size_type tag_ = ntag;

};

struct entry_definition
{

    identifier entry_name_;
    lvalue_list argument_list_;
    statement_block body_;
    rvalue_list return_statement_;

    size_type tag_ = ntag;

};

using entries = std::list< entry_definition >; // <forward_list> not supported in x3 container_traits.hpp

struct program
{

    entries entries_;

    void
    append(entry_definition const & _entry)
    {
        entries_.push_back(_entry);
    }

    void
    append(entry_definition && _entry)
    {
        entries_.push_back(std::move(_entry));
    }

    void
    append(program const & _program)
    {
        entries_.insert(std::cend(entries_), std::cbegin(_program.entries_), std::cend(_program.entries_));
    }

    void
    append(program && _program)
    {
        entries_.splice(std::cend(entries_), std::move(_program.entries_));
    }

    size_type tag_ = ntag;

};

using programs = std::deque< program >;

#ifndef NDEBUG
namespace test_tagged_nodes
{

using tagged_nodes = pack<
identifier,
rvalue_list,
lvalue_list,
expression,
intrinsic_invocation,
entry_substitution,
variable_declaration,
assignment,
statement_block,
entry_definition,
program
>;

template< typename = tagged_nodes >
struct test;

template< typename ...nodes >
struct test< pack< nodes... > >
{

    static
    constexpr
    bool
    run() noexcept
    {
        return (is_tagged_v< nodes > && ...);
    }

};

static_assert(test<>::run(), "not all tagged nodes are tagged");

} // namespace test_node_ptr
#endif

}

using ast::get;

}
