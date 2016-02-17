#pragma once

#include <insituc/ast/tokens.hpp>
#include <insituc/ast/ast.hpp>
#include <insituc/variant.hpp>
#include <insituc/utility/numeric/safe_convert.hpp>
#include <insituc/utility/head.hpp>

#include <versatile/visit.hpp>

#include <ostream>
#include <limits>
#include <iomanip>
#include <iterator>
#include <stdexcept>
#include <algorithm>
#include <functional>

#include <cassert>

namespace insituc
{
namespace ast
{

struct printer
{

    printer(std::ostream & _out)
        : out_(_out)
    { ; }

    template< typename type >
    void
    operator () (type const & _value)
    {
        print(_value);
    }

private :

    std::ostream & out_;
    size_type indent_ = 0;

    void
    indent_insert()
    {
        for (size_type i = 0; i < indent_; ++i) {
            out_.put('\t');
        }
    }

    void
    indent_increase()
    {
        if (indent_ < std::numeric_limits< size_type >::max()) {
            ++indent_;
        } else {
            throw std::overflow_error("can't increase indent");
        }
    }

    void
    indent_decrease()
    {
        if (0 < indent_) {
            --indent_;
        } else {
            throw std::underflow_error("can't decrease indent");
        }
    }

    void
    print(empty const & /*_empty*/)
    {

    }

    void
    print(char_type const _character)
    {
        out_ << _character;
    }

    void
    print(char_type const * const _c_str)
    {
        out_ << _c_str;
    }

    void
    print(string_type const & _string)
    {
        out_ << _string;
    }

    template< typename type >
    std::enable_if_t< std::is_enum_v< type > >
    print(type const _value)
    {
        print(c_str(_value));
    }

    void
    print(G const & _value)
    {
        std::streamsize const default_precision_ = out_.precision();
        out_.precision(std::numeric_limits< G >::digits10);
        out_ << _value;
        assert(is_includes< int >(default_precision_));
        out_.precision(static_cast< int >(default_precision_));
    }

    void
    print(symbol const & _symbol)
    {
        print(_symbol.name_);
    }

    void
    print(identifier const & _identifier)
    {
        print(_identifier.symbol_);
        if (!_identifier.wrts_.empty()) {
            print('{');
            {
                for (symbol const & wrt_ : head(_identifier.wrts_)) {
                    print(wrt_);
                    print(", ");
                }
                print(_identifier.wrts_.back());
            }
            print('}');
        }
    }

    void
    print(lvalues const & _lvalues)
    {
        if (!_lvalues.empty()) {
            for (identifier const & lvalue_ : head(_lvalues)) {
                print(lvalue_);
                print(", ");
            }
            print(_lvalues.back());
        }
    }

    void
    print(lvalue_list const & _lvalue_list)
    {
        print(_lvalue_list.lvalues_);
    }

    void
    print(intrinsic_invocation const & _intrinsic_invocation)
    {
        print(_intrinsic_invocation.intrinsic_);
        print('(');
        print(_intrinsic_invocation.argument_list_);
        print(')');
    }

    void
    print(entry_substitution const & _entry_substitution)
    {
        print(_entry_substitution.entry_name_);
        print('(');
        print(_entry_substitution.argument_list_);
        print(')');
    }

    void
    print(unary_expression const & _unary_expression)
    {
        print(_unary_expression.operator_);
        print(_unary_expression.operand_);
    }

    void
    print(operation const & _operation)
    {
        print(' ');
        print(_operation.operator_);
        print(' ');
        print(_operation.operand_);
    }

    void
    print(operation_list const & _operation_list)
    {
        std::for_each(std::cbegin(_operation_list), std::cend(_operation_list), std::ref(*this));
    }

    void
    print(expression const & _expression)
    {
        print(_expression.first_);
        print(_expression.rest_);
    }

    void
    print(binary_expression const & _binary_expression)
    {
        print(_binary_expression.lhs_);
        print(' ');
        print(_binary_expression.operator_);
        print(' ');
        print(_binary_expression.rhs_);
    }

    void
    print(rvalues const & _rvalues)
    {
        if (!_rvalues.empty()) {
            for (rvalue const & rvalue_ : head(_rvalues)) {
                visit(*this, *rvalue_); // w/o brackets
                print(", ");
            }
            visit(*this, *_rvalues.back());
        }
    }

    void
    print(rvalue_list const & _rvalue_list)
    {
        print(_rvalue_list.rvalues_);
    }

    void
    print(operand const & _operand)
    {
        switch (_operand.which()) {
        case index_at< operand, expression > :
        case index_at< operand, binary_expression > : {
            print('(');
            visit(*this, *_operand);
            print(')');
            break;
        }
        default : {
            visit(*this, *_operand);
            break;
        }
        }
    }

    void
    print(operand_cptr const & _operand_cptr)
    {
        print(*_operand_cptr);
    }

    void
    print(variable_declaration const & _variable_declaration)
    {
        print(keyword::local_);
        print(' ');
        print(_variable_declaration.lhs_);
        print(' ');
        print(assign::assign);
        print(' ');
        print(_variable_declaration.rhs_);
    }

    void
    print(assignment const & _assignment)
    {
        print(_assignment.lhs_);
        print(' ');
        print(_assignment.operator_);
        print(' ');
        print(_assignment.rhs_);
    }

    void
    print(statement const & _statement)
    {
        indent_insert();
        visit(*this, *_statement);
    }

    void
    print(statements const & _statements)
    {
        for (statement const & statement_ : _statements) {
            print(statement_);
            print('\n');
        }
    }

    void
    print(statement_block const & _statement_block)
    {
        print(keyword::begin_);
        print('\n');
        indent_increase();
        print(_statement_block.statements_);
        indent_decrease();
        indent_insert();
        print(keyword::end_);
    }

    void
    print(entry_definition const & _entry)
    {
        print(keyword::function_);
        print(' ');
        print(_entry.entry_name_);
        print('(');
        print(_entry.argument_list_);
        print(")\n");
        indent_increase();
        print(_entry.body_.statements_);
        indent_insert();
        print(keyword::return_);
        print(' ');
        print(_entry.return_statement_);
        print('\n');
        indent_decrease();
        print(keyword::end_);
    }

    void
    print(entries const & _entries)
    {
        for (entry_definition const & entry_ : _entries) {
            print(entry_);
            print("\n\n");
        }
    }

    void
    print(program const & _program)
    {
        print(_program.entries_);
    }

    void
    print(programs const & _programs)
    {
        for (program const & program_ : _programs) {
            print(program_);
            print('\n');
        }
    }

};

using printable_nodes = pack
<
program, programs,
entry_definition, entries,
statement_block, statements,
statement, assignment, variable_declaration,
entry_substitution, intrinsic_invocation,
expression, unary_expression, binary_expression,
lvalues, lvalue_list, rvalues, rvalue_list,
operand, operand_cptr,
identifier, symbol,
assign, unary, binary, constant, intrinsic, keyword,
empty
>;

template< typename type >
std::enable_if_t< is_contained_v< type, printable_nodes >, std::ostream & >
operator << (std::ostream & _out, type const & _value)
{
    printer printer_(_out);
    printer_(_value);
    return _out;
}

#undef NODES

}
}
