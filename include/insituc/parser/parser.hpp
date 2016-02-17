#pragma once

#include <insituc/parser/base_types.hpp>
#include <insituc/ast/ast.hpp>

#include <deque>
#include <utility>
#include <experimental/optional>
#include <memory>

namespace insituc
{
namespace parser
{

struct parse_result
{

    ast::program ast_;
    ranges ranges_;
    error_description_ptr error_ = nullptr;

};

parse_result
parse(base_iterator_type const & first, base_iterator_type const & last);

std::experimental::optional< G >
parse_real_number(base_iterator_type const & first, base_iterator_type const & last);

std::deque< string_type >
get_constants();

std::deque< string_type >
get_intrinsics();

std::deque< string_type >
get_keywords();

}
}
