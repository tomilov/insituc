#pragma once

#include <insituc/ast/ast.hpp>

#include <boost/fusion/include/adapt_struct.hpp>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdisabled-macro-expansion"

BOOST_FUSION_ADAPT_STRUCT(insituc::ast::pragma,               record_)
BOOST_FUSION_ADAPT_STRUCT(insituc::ast::symbol,               name_)
BOOST_FUSION_ADAPT_STRUCT(insituc::ast::identifier,           symbol_, wrts_)
BOOST_FUSION_ADAPT_STRUCT(insituc::ast::unary_expression,     operator_, operand_)
BOOST_FUSION_ADAPT_STRUCT(insituc::ast::binary_expression,    lhs_, operator_, rhs_)
BOOST_FUSION_ADAPT_STRUCT(insituc::ast::operation,            operator_, operand_)
BOOST_FUSION_ADAPT_STRUCT(insituc::ast::expression,           first_, rest_)
BOOST_FUSION_ADAPT_STRUCT(insituc::ast::entry_substitution,   entry_name_, argument_list_)
BOOST_FUSION_ADAPT_STRUCT(insituc::ast::intrinsic_invocation, intrinsic_, argument_list_)
BOOST_FUSION_ADAPT_STRUCT(insituc::ast::lvalue_list,          lvalues_)
BOOST_FUSION_ADAPT_STRUCT(insituc::ast::rvalue_list,          pragma_, rvalues_)
BOOST_FUSION_ADAPT_STRUCT(insituc::ast::variable_declaration, lhs_, rhs_)
BOOST_FUSION_ADAPT_STRUCT(insituc::ast::assignment,           lhs_, operator_, rhs_)
BOOST_FUSION_ADAPT_STRUCT(insituc::ast::statement_block,      statements_)
BOOST_FUSION_ADAPT_STRUCT(insituc::ast::entry_definition,     entry_name_, argument_list_, body_, return_statement_)
BOOST_FUSION_ADAPT_STRUCT(insituc::ast::program,              entries_)

#pragma clang diagnostic pop
