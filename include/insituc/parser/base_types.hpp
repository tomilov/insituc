#pragma once

#include <insituc/base_types.hpp>

#include <insituc/variant.hpp>

#include <boost/spirit/home/support/iterators/line_pos_iterator.hpp>

#include <type_traits>
#include <iterator>
#include <deque>
#include <string>
#include <memory>

namespace insituc
{
namespace parser
{

static_assert(std::is_base_of< std::forward_iterator_tag, typename std::iterator_traits< base_iterator_type >::iterator_category >::value, "Need multipass guarantee!");

using input_iterator_type = boost::spirit::line_pos_iterator< base_iterator_type >;
using range = std::pair< input_iterator_type, input_iterator_type >;
using ranges = std::deque< range >;

struct error_description
{

    std::string which_;
    input_iterator_type first_;
    input_iterator_type where_;
    input_iterator_type last_;

};

using error_description_ptr = std::shared_ptr< aggregate_wrapper< error_description > >;

inline
error_description_ptr
make_error_description(std::string && _which,
                       input_iterator_type const & _first,
                       input_iterator_type const & _where,
                       input_iterator_type const & _last)
{
    return error_description_ptr::make_shared(std::move(_which), _first, _where, _last);
}

}
}
