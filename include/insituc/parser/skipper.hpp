#pragma once

#include <insituc/parser/base_types.hpp>

#include <boost/spirit/home/x3.hpp>

namespace insituc
{
namespace parser
{

namespace x3 = boost::spirit::x3;

using skipper_parser = x3::rule< struct skipper_class, x3::unused_type const >;

BOOST_SPIRIT_DECLARE(skipper_parser)

skipper_parser const &
get_skipper();

}
}
