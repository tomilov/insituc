#include <insituc/parser/implementation/skipper.hpp>

#include <insituc/parser/base_types.hpp>

namespace insituc
{
namespace parser
{

BOOST_SPIRIT_INSTANTIATE(skipper_parser, input_iterator_type, x3::unused_type)

}
}
