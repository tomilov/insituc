#pragma once

#include <insituc/parser/base_types.hpp>
#include <insituc/ast/ast.hpp>

#include <regex>
#include <iterator>
#include <sstream>

namespace insituc
{
namespace parser
{

template< typename value_type = size_type >
value_type
parse_pragma(ast::pragma const & _pragma, string_type const & _key)
{
    value_type value_{};
    std::regex key_value_{R"(\s*(\w+)\s*=\s*([^;]+);?)"};
    std::sregex_iterator it{std::cbegin(_pragma.record_), std::cend(_pragma.record_), key_value_};
    for (std::sregex_iterator end; it != end; ++it) {
        std::smatch const & match_ = *it;
        if (match_.size() == 3) {
            if (match_[1] == _key) {
                std::istringstream{match_[2]} >> value_;
                break;
            }
        }
    }
    return value_;
}

}
}
