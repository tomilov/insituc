#include <insituc/transform/optimizer/leaf.hpp>

#include <versatile/visit.hpp>

namespace insituc
{
namespace transform
{

struct trim
{

    std::set< ast::identifier > nulls_;
    typename std::set< ast::identifier >::iterator end = std::end(nulls_);

    [[noreturn]]
    ast::operand
    trim_operand(ast::empty const & /*_empty*/) const
    {
        throw std::runtime_error("empty operand in expression is not allowed");
    }

    ast::operand
    trim_operand(G const & _value) const
    {
        return _value;
    }

    ast::operand
    trim_operand(ast::constant const _constant) const
    {
        return _constant;
    }

    ast::operand
    trim_operand(ast::intrinsic_invocation const & _ast) const
    {
        return ast::intrinsic_invocation{_ast.intrinsic_, {operator () (_ast.argument_list_.rvalues_)}};
    }

    ast::operand
    trim_operand(ast::entry_substitution const & _ast) const
    {
        return ast::entry_substitution{_ast.entry_name_, {operator () (_ast.argument_list_.rvalues_)}};
    }

    ast::operand
    trim_operand(ast::identifier const & _identifier) const
    {
        if (nulls_.find(_identifier) != end) {
            return zero;
        }
        return _identifier;
    }

    ast::operand
    trim_operand(ast::unary_expression const & _ast) const
    {
        return ast::unary_expression{_ast.operator_, operator () (_ast.operand_)};
    }

    ast::operand
    trim_operand(ast::binary_expression const & _ast) const
    {
        return ast::binary_expression{operator () (_ast.lhs_), _ast.operator_, operator () (_ast.rhs_)};
    }

    ast::operand
    trim_operand(ast::expression const & _expression) const
    {
        ast::operation_list rest_;
        for (ast::operation const & operation_ : _expression.rest_) {
            rest_.push_back({operation_.operator_, operator () (operation_.operand_)});
        }
        return ast::expression{operator () (_expression.first_), std::move(rest_)};
    }

    ast::operand
    trim_operand(ast::rvalue_list const & _rvalue_list) const
    {
        return ast::rvalue_list{operator () (_rvalue_list.rvalues_)};
    }

    ast::operand
    trim_operand(ast::operand_cptr const _operand_cptr) const
    {
        return operator () (*_operand_cptr);
    }

    ast::operand
    operator () (ast::operand const & _operand) const
    {
        return visit([&] (auto const & o) -> ast::operand
        {
            return trim_operand(std::move(o));
        }, *_operand);
    }

    ast::rvalues
    operator () (ast::rvalues const & _rvalues) const
    {
        ast::rvalues rvalues_;
        for (ast::rvalue const & rvalue_ : _rvalues) {
            rvalues_.push_back(operator () (rvalue_));
        }
        return rvalues_;
    }

    ast::statement
    trim_statement(ast::empty const & _empty) const
    {
        return _empty;
    }

    ast::statement
    trim_statement(ast::variable_declaration const & _ast) const
    {
        return ast::variable_declaration{_ast.lhs_, {operator () (_ast.rhs_.rvalues_)}};
    }

    ast::statement
    trim_statement(ast::assignment const & _assignment) const
    {
        return ast::assignment{_assignment.lhs_, _assignment.operator_, {operator () (_assignment.rhs_.rvalues_)}};
    }

    ast::statement
    trim_statement(ast::statement_block const & _statement_block) const
    {
        return ast::statement_block{operator () (_statement_block.statements_)};
    }

    ast::statement
    operator () (ast::statement const & _statement) const
    {
        return visit([&] (auto const & s) -> ast::statement
        {
            return trim_statement(std::move(s));
        }, *_statement);
    }

    ast::statements
    operator () (ast::statements const & _statements) const
    {
        ast::statements statements_;
        for (ast::statement const & statement_ : _statements) {
            statements_.push_back(operator () (statement_));
        }
        return statements_;
    }

    ast::entry_definition
    operator () (ast::entry_definition const & _entry,
                 ast::symbol const & _name)
    {
        ast::lvalues arguments_;
        for (ast::lvalue const & argument_ : _entry.argument_list_.lvalues_) {
            if (argument_.is_original()) {
                arguments_.push_back(argument_);
            } else {
                nulls_.insert(argument_);
            }
        }
        return {{_name, {}}, {std::move(arguments_)}, {{operator () (_entry.body_.statements_)}}, {{operator () (_entry.return_statement_.rvalues_)}}};
    }

};

ast::entry_definition
leaf(ast::entry_definition const & _entry,
     ast::symbol const & _name)
{
    return trim{}(_entry, _name);
}

}
}
