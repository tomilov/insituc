#include <insituc/parser/implementation/parser.hpp>

#include <insituc/parser/base_types.hpp>
#include <insituc/parser/skipper.hpp>
#include <insituc/parser/annotation.hpp>

#include <functional>

namespace insituc
{
namespace parser
{

using phrase_context_type = typename x3::phrase_parse_context< skipper_parser >::type;


using context_type = x3::context< on_error_tag, error_description_ptr * const, x3::context< on_success_tag, ranges * const, phrase_context_type > >;

BOOST_SPIRIT_INSTANTIATE(program_parser, input_iterator_type, context_type)

}
}
