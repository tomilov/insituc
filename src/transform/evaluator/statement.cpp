#include <insituc/transform/evaluator/statement.hpp>

#include <insituc/transform/evaluator/expression.hpp>

#include <versatile/visit.hpp>

namespace insituc
{
namespace transform
{

namespace statement
{

using result_type = ast::statement;

namespace copy
{
namespace
{

result_type
evaluate(ast::empty const & _empty)
{
    return _empty;
}

result_type
evaluate(ast::variable_declaration const & _variable_declaration)
{
    return ast::variable_declaration{_variable_declaration.lhs_, {transform::evaluate(_variable_declaration.rhs_.rvalues_)}};
}

result_type
evaluate(ast::assignment const & _assignment)
{
    return ast::assignment{_assignment.lhs_, _assignment.operator_, {transform::evaluate(_assignment.rhs_.rvalues_)}};
}

result_type
evaluate(ast::statement_block const & _statement_block)
{
    return ast::statement_block{transform::evaluate(_statement_block.statements_)};
}

} // static namespace
}

namespace move
{
namespace
{

result_type
evaluate(ast::empty && _empty)
{
    return std::move(_empty);
}

result_type
evaluate(ast::variable_declaration && _variable_declaration)
{
    _variable_declaration.rhs_.rvalues_ = transform::evaluate(std::move(_variable_declaration.rhs_.rvalues_));
    return std::move(_variable_declaration);
}

result_type
evaluate(ast::assignment && _assignment)
{
    _assignment.rhs_.rvalues_ = transform::evaluate(std::move(_assignment.rhs_.rvalues_));
    return std::move(_assignment);
}

result_type
evaluate(ast::statement_block && _statement_block)
{
    _statement_block.statements_ = transform::evaluate(std::move(_statement_block.statements_));
    return std::move(_statement_block);
}

} // static namespace
}

}

ast::statement
evaluate(ast::statement const & _statement)
{
    return visit([&] (auto const & s) -> typename statement::result_type
    {
        return statement::copy::evaluate(s);
    }, *_statement);
}

ast::statement
evaluate(ast::statement && _statement)
{
    return visit([&] (auto & s) -> typename statement::result_type
    {
        return statement::move::evaluate(std::move(s));
    }, *_statement);
}

ast::statements
evaluate(ast::statements const & _statements)
{
    ast::statements statements_;
    for (ast::statement const & statement_ : _statements) {
        if (!statement_.active< ast::empty >()) {
            statements_.push_back(evaluate(statement_));
        }
    }
    return statements_;
}

ast::statements
evaluate(ast::statements && _statements)
{
    ast::statements statements_;
    for (ast::statement & statement_ : _statements) {
        if (!statement_.active< ast::empty >()) {
            statements_.push_back(evaluate(std::move(statement_)));
        }
    }
    return statements_;
}

}
}
