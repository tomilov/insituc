#pragma once

#include <insituc/parser/parser.hpp>

#include <insituc/parser/skipper.hpp>
#include <insituc/ast/adaptation.hpp>
#include <insituc/parser/annotation.hpp>

#include <boost/spirit/home/x3.hpp>

#include <type_traits>
#include <deque>
#include <algorithm>
#include <iterator>
#include <utility>
#include <iterator>

#include <cassert>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wglobal-constructors"
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wunused-parameter"
namespace insituc
{
namespace parser
{

namespace
{

template< typename token >
decltype(auto)
lit(token const _token)
{
    static_assert(std::is_enum_v< token >, "enum expected");
    return x3::lit(c_str(_token));
}

template< typename token >
decltype(auto)
distinct_literal(token const _token)
{
    static_assert(std::is_enum_v< token >, "enum expected");
    return x3::lexeme[lit(_token) >> !(x3::alnum | '_')];
}

template< typename parser >
decltype(auto)
distinct_word(parser && _parser)
{
    return x3::lexeme[std::forward< parser >(_parser) >> !(x3::alnum | '_')];
}

x3::symbols< ast::assign > const assign_
({
     to_pair(ast::assign::assign),
     to_pair(ast::assign::plus_assign),
     to_pair(ast::assign::minus_assign),
     to_pair(ast::assign::times_assign),
     to_pair(ast::assign::divide_assign),
     to_pair(ast::assign::mod_assign),
     to_pair(ast::assign::raise_assign)
 },
 "assign");

x3::symbols< ast::unary > const unary_
({
     to_pair(ast::unary::plus),
     to_pair(ast::unary::minus)
 },
 "unary");

x3::symbols< ast::binary > const binary_
({
     to_pair(ast::binary::add),
     to_pair(ast::binary::sub),
     to_pair(ast::binary::mul),
     to_pair(ast::binary::div),
     to_pair(ast::binary::mod),
     to_pair(ast::binary::pow)
 },
 "binary");

x3::symbols< ast::constant > const constant_
({
     to_pair(ast::constant::zero),
     to_pair(ast::constant::one),
     to_pair(ast::constant::pi),
     to_pair(ast::constant::l2e),
     to_pair(ast::constant::l2t),
     to_pair(ast::constant::lg2),
     to_pair(ast::constant::ln2)
 },
 "constant");

x3::symbols< ast::intrinsic > const intrinsic_
({
     to_pair(ast::intrinsic::chs),
     to_pair(ast::intrinsic::abs),
     to_pair(ast::intrinsic::twice),
     to_pair(ast::intrinsic::sumsqr),
     to_pair(ast::intrinsic::sqrt),
     to_pair(ast::intrinsic::round),
     to_pair(ast::intrinsic::trunc),
     to_pair(ast::intrinsic::remainder),
     to_pair(ast::intrinsic::cos),
     to_pair(ast::intrinsic::sin),
     to_pair(ast::intrinsic::sincos),
     to_pair(ast::intrinsic::tg),
     to_pair(ast::intrinsic::ctg),
     to_pair(ast::intrinsic::arctg),
     to_pair(ast::intrinsic::atan2),
     to_pair(ast::intrinsic::poly),
     to_pair(ast::intrinsic::frac),
     to_pair(ast::intrinsic::intrem),
     to_pair(ast::intrinsic::fracint),
     to_pair(ast::intrinsic::pow),
     to_pair(ast::intrinsic::exp),
     to_pair(ast::intrinsic::pow2),
     to_pair(ast::intrinsic::sqr),
     to_pair(ast::intrinsic::log),
     to_pair(ast::intrinsic::ln),
     to_pair(ast::intrinsic::log2),
     to_pair(ast::intrinsic::lg),
     to_pair(ast::intrinsic::yl2xp1),
     to_pair(ast::intrinsic::scale2),
     to_pair(ast::intrinsic::extract),
     to_pair(ast::intrinsic::pow2m1),
     to_pair(ast::intrinsic::arcsin),
     to_pair(ast::intrinsic::arccos),
     to_pair(ast::intrinsic::max),
     to_pair(ast::intrinsic::min)
 },
 "intrinsic");

x3::symbols< ast::keyword > const keyword_
({
     to_pair(ast::keyword::local_),
     to_pair(ast::keyword::begin_),
     to_pair(ast::keyword::end_),
     to_pair(ast::keyword::function_),
     to_pair(ast::keyword::return_)
 },
 "keyword");

struct reserved_symbols_enumerator
{

    std::deque< string_type > & keywords_;

    template< typename token >
    void
    operator () (string_type const & _symbol, token const _token) const
    {
        assert(c_str(_token) == _symbol);
        keywords_.push_back(_symbol);
    }

};

}

std::deque< string_type >
get_constants()
{
    std::deque< string_type > constants_;
    constant_.for_each(reserved_symbols_enumerator{constants_});
    return constants_;
}

std::deque< string_type >
get_intrinsics()
{
    std::deque< string_type > instrinsics_;
    intrinsic_.for_each(reserved_symbols_enumerator{instrinsics_});
    return instrinsics_;
}

std::deque< string_type >
get_keywords()
{
    std::deque< string_type > keywords_;
    keyword_.for_each(reserved_symbols_enumerator{keywords_});
    return keywords_;
}

namespace
{

using real_parser = x3::real_parser< G >;

real_parser const real_number{};

}

std::experimental::optional< G >
parse_real_number(base_iterator_type const & first, base_iterator_type const & last)
{
    G ast_{};
    base_iterator_type it = first;
    if (x3::parse(it, last, real_number, ast_)) {
        if (it == last) {
            return std::move(ast_);
        }
    }
    return {};
}

namespace
{

struct pragma_class :               error_handler_base, annotation_base {};
struct symbol_class :               error_handler_base, annotation_base {};
struct identifier_class :           error_handler_base, annotation_base {};
struct operand_class :              error_handler_base, annotation_base {};
struct intrinsic_invocation_class : error_handler_base, annotation_base {};
struct entry_substitution_class :   error_handler_base, annotation_base {};
struct expression_class :           error_handler_base, annotation_base {};
struct lvalue_list_class :          error_handler_base, annotation_base {};
struct rvalue_list_class :          error_handler_base, annotation_base {};

using pragma_parser =               x3::rule< pragma_class,               ast::pragma >;
using symbol_parser =               x3::rule< symbol_class,               ast::symbol >;
using identifier_parser =           x3::rule< identifier_class,           ast::identifier >;
using operand_parser =              x3::rule< operand_class,              ast::operand >;
using intrinsic_invocation_parser = x3::rule< intrinsic_invocation_class, ast::intrinsic_invocation >;
using entry_substitution_parser =   x3::rule< entry_substitution_class,   ast::entry_substitution >;
using expression_parser =           x3::rule< expression_class,           ast::expression >;
using lvalue_list_parser =          x3::rule< lvalue_list_class,          ast::lvalue_list > ;
using rvalue_list_parser =          x3::rule< rvalue_list_class,          ast::rvalue_list > ;

pragma_parser const               pragma =               "pragma";
symbol_parser const               symbol =               "symbol";
identifier_parser const           identifier =           "identifier";
operand_parser const              operand =              "operand";
intrinsic_invocation_parser const intrinsic_invocation = "intrinsic invocation";
entry_substitution_parser const   entry_substitution =   "entry substitution";
expression_parser const           expression =           "expression";
lvalue_list_parser const          lvalue_list =          "lvalue list";
rvalue_list_parser const          rvalue_list =          "rvalue list";

auto const pragma_def =
        (x3::lit("[[") > (x3::raw[x3::lexeme[*(x3::char_ - ']')]] > x3::lit("]]")))
        ;

auto const symbol_def =
        !distinct_word(keyword_ | intrinsic_ | constant_)
        >> x3::raw[x3::lexeme[((x3::alpha | '_') >> *(x3::alnum | '_'))]]
        ;

auto const identifier_def =
        symbol > -('{' > (symbol % ',') > '}')
        ;

auto const lvalue_list_def =
        identifier % ','
        ;

auto const rvalue_list_def =
        -pragma
        >> (expression % ',')
        ;

auto const expression_def =
        operand > *(binary_ > operand)
        ;

auto const intrinsic_invocation_def =
        (distinct_word(intrinsic_) > '(')
        > (-rvalue_list > ')')
        ;

auto const entry_substitution_def =
        (identifier >> '(')
        > (-rvalue_list > ')')
        ;

auto const operand_def =
        real_number
        | distinct_word(constant_)
        | intrinsic_invocation
        | entry_substitution
        | identifier
        | (unary_ > (!unary_ > operand))
        | ('(' > expression > ')')
        ;

}

BOOST_SPIRIT_DEFINE(pragma,
                    symbol,
                    identifier,
                    operand,
                    intrinsic_invocation,
                    entry_substitution,
                    lvalue_list,
                    rvalue_list,
                    expression)

namespace
{

struct variable_declaration_class : error_handler_base, annotation_base {};
struct assignment_class :           error_handler_base, annotation_base {};
struct statement_class :            error_handler_base, annotation_base {};
struct statements_class :           error_handler_base, annotation_base {};
struct statement_block_class :      error_handler_base, annotation_base {};

using variable_declaration_parser = x3::rule< variable_declaration_class, ast::variable_declaration >;
using assignment_parser =           x3::rule< assignment_class, ast::assignment >;
using statements_parser =           x3::rule< statements_class, ast::statements >;
using statement_block_parser =      x3::rule< statement_block_class, ast::statement_block >;
using statement_parser =            x3::rule< statement_class, ast::statement >;

variable_declaration_parser const variable_declaration = "variable declaration";
assignment_parser const           assignment =           "assignment";
statements_parser const           statements =           "statement list";
statement_block_parser const      statement_block =      "statement block";
statement_parser const            statement =            "statement";

auto const variable_declaration_def =
        distinct_literal(ast::keyword::local_)
        > lvalue_list
        > lit(ast::assign::assign)
        > rvalue_list
        ;

auto const assignment_def =
        lvalue_list
        > assign_
        > rvalue_list
        ;

auto const statements_def =
        *statement
        ;

auto const statement_block_def =
        distinct_literal(ast::keyword::begin_)
        > statements
        > distinct_literal(ast::keyword::end_)
        ;

auto const statement_def =
        statement_block
        | variable_declaration
        | assignment
        ;

}

BOOST_SPIRIT_DEFINE(variable_declaration,
                    assignment,
                    statements,
                    statement_block,
                    statement)

namespace
{

struct entry_definition_class : error_handler_base, annotation_base {};
struct program_class :          error_handler_base, annotation_base {};

using entry_definition_parser = x3::rule< entry_definition_class, ast::entry_definition >;
using program_parser =          x3::rule< program_class, ast::program >;

entry_definition_parser const entry_definition = "entry definition";
program_parser          const program          = "program";

auto const entry_definition_def =
        distinct_literal(ast::keyword::function_)
        > identifier
        > ('(' > -lvalue_list > ')')
        > statements
        > distinct_literal(ast::keyword::return_)
        > rvalue_list
        > distinct_literal(ast::keyword::end_)
        ;

auto const program_def =
        +entry_definition
        ;

}

BOOST_SPIRIT_DEFINE(entry_definition,
                    program)

parse_result
parse(base_iterator_type const & first, base_iterator_type const & last)
{
    parse_result parse_result_;

    error_description_ptr & error_description_ = parse_result_.error_;
    auto const add_error_handler_ = x3::with< on_error_tag >(&error_description_)[program];

    auto const add_annotation_handler_ = x3::with< on_success_tag >(&parse_result_.ranges_)[add_error_handler_];

    auto const & grammar_ = add_annotation_handler_;

    auto const & skipper_ = get_skipper();

    input_iterator_type const beg(first);
    input_iterator_type pos = beg;
    input_iterator_type const end(last);
    try {
        if (x3::phrase_parse(pos, end, grammar_, skipper_, parse_result_.ast_)) {
            if (pos != end) {
                error_description_ = make_error_description("can't reach the end of input", beg, pos, end);
            }
        } else if (!error_description_) {
            error_description_ = make_error_description("can't parse input", beg, pos, end);
        }
    } catch (x3::expectation_failure< input_iterator_type > const & ef) {
        auto const where = ef.where();
        auto const context = std::min(std::distance(where, end), std::intptr_t{20});
        auto description_ = "expected: '" + ef.which() + "' got: '" + std::string{where, std::next(where, context)} + "'";
        if (!error_description_) {
            error_description_ = make_error_description(std::move(description_), beg, pos, end);
        }
    }
    return parse_result_;
}

}
}
#pragma clang diagnostic pop
