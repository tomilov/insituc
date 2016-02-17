#pragma once

#include <insituc/parser/base_types.hpp>
#include <insituc/ast/ast.hpp>
#include <insituc/debug/demangle.hpp>

#include <versatile/visit.hpp>

#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/support/context.hpp>

#include <type_traits>
#include <stdexcept>
#include <typeinfo>

namespace insituc
{
namespace parser
{

namespace x3 = boost::spirit::x3;

struct on_error_tag;

struct error_handler_base
{

    template< typename iterator, typename exception, typename context >
    static
    x3::error_handler_result
    on_error(iterator & first, iterator const & last,
             exception const & x, context const & _context)
    {
        auto & error_description_ = *x3::get< on_error_tag >(_context);
        error_description_ = make_error_description("expected: " + x.which(), first, x.where(), last);
        return x3::error_handler_result::fail;
    }

};

struct on_success_tag;

struct annotation_base
{

    template< typename iterator, typename type, typename context >
    static
    std::enable_if_t< !ast::is_tagged_v< type > >
    on_success(iterator const & /*first*/, iterator const & /*last*/,
               type & /*_ast*/, context const & /*_context*/)
    { ; }

    template< typename iterator, typename type, typename context >
    static
    std::enable_if_t< ast::is_tagged_v< type > >
    on_success(iterator const & first, iterator const & last,
               type & _ast, context const & _context)
    {
        auto & ranges_ = *x3::get< on_success_tag >(_context);
        _ast.tag_ = ranges_.size();
        ranges_.push_back({first, last});
    }

};

}
}
