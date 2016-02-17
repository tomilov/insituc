#pragma once

#include <insituc/parser/skipper.hpp>

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

skipper_parser const skipper = "skipper";

auto const skipper_def =
        x3::ascii::space
        | (x3::lit("/*") > x3::seek[x3::lit("*/")])
        | (x3::lit("//") > x3::seek[x3::eol | x3::eoi])
        ; // R"(\s+|(/\*[^*]*\*+([^/*][^*]*\*+)*/)|(//[^\r\n]*))"

}

BOOST_SPIRIT_DEFINE(skipper);

skipper_parser const &
get_skipper()
{
    return skipper;
}

}
}
#pragma clang diagnostic pop
