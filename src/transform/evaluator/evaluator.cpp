#include <insituc/transform/evaluator/evaluator.hpp>

#include <insituc/transform/evaluator/expression.hpp>
#include <insituc/transform/evaluator/statement.hpp>

namespace insituc
{
namespace transform
{

ast::entry_definition
evaluate(ast::entry_definition const & _entry)
{
    return {_entry.entry_name_, _entry.argument_list_, {transform::evaluate(_entry.body_.statements_)}, {{transform::evaluate(_entry.return_statement_.rvalues_)}}};
}

ast::entry_definition
evaluate(ast::entry_definition && _entry)
{
    _entry.body_.statements_ = transform::evaluate(std::move(_entry.body_.statements_));
    _entry.return_statement_.rvalues_ = transform::evaluate(std::move(_entry.return_statement_.rvalues_));
    return std::move(_entry);
}

ast::program
evaluate(ast::program const & _program)
{
    ast::program program_;
    for (ast::entry_definition const & entry_ : _program.entries_) {
        program_.entries_.push_back(evaluate(entry_));
    }
    return program_;
}

ast::program
evaluate(ast::program && _program)
{
    for (ast::entry_definition & entry_ : _program.entries_) {
        entry_ = evaluate(std::move(entry_));
    }
    return std::move(_program);
}

}
}
