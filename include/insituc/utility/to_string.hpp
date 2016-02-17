#pragma once

#include <string>
#include <sstream>

namespace insituc
{

template< typename type >
std::string
to_string(type const & _value)
{
    std::ostringstream oss_;
    oss_ << _value;
    return oss_.str();
}

}
