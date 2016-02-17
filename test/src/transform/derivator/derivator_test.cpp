#include <insituc/transform/derivator/derivator.hpp>
#include <insituc/transform/evaluator/evaluator.hpp>
#include <insituc/transform/transform.hpp>

#include <insituc/utility/tail.hpp>
#include <insituc/ast/io.hpp>
#include <insituc/ast/compare.hpp>
#include <insituc/parser/parser.hpp>
#include <insituc/meta/compiler.hpp>

#include <experimental/optional>

#include <utility>
#include <iterator>
#include <exception>
#include <iostream>
#include <string>
#include <initializer_list>
#include <deque>

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>

#include <insituc/debug/demangle.hpp>
#include <typeinfo>
#include <cxxabi.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
namespace
{

using namespace std::string_literals;

using namespace insituc;

ast::program
join_programs(ast::programs && _programs)
{
    auto & first_ = _programs.front();
    for (auto & rest_ : tail(_programs)) {
        first_.append(std::move(rest_));
    }
    return std::move(first_);
}

class test
{

    std::experimental::optional< ast::program >
    parse(std::string const & _source) const
    {
        auto parse_result_ = parser::parse(std::cbegin(_source), std::cend(_source));
        if (!!parse_result_.error_) {
            auto const & error_description_ = *parse_result_.error_;
            std::cerr << "Error: \"" << error_description_.which_
                      << "\" at input position: ";
            std::copy(error_description_.where_, error_description_.last_, std::ostreambuf_iterator< char_type >(std::cerr));
            std::cerr << std::endl;
            return {};
        }
        return std::move(parse_result_.ast_);
    }

    bool
    is_primitive_of(std::string const & _primitive,
                    std::string const & _derivative,
                    ast::symbols const & _wrts = {{"x"}},
                    bool j = false)
    {
        auto const ast_ = parse(_derivative);
        if (!ast_) {
            std::cerr << "Can't parse _derivative" << std::endl;
            return false;
        }
        ast::program const rhs_ = transform::evaluate(*ast_);
        auto const lhs_ = parse(_primitive);
        if (!lhs_) {
            std::cerr << "Can't parse _primitive" << std::endl;
            return false;
        }
        ast::program const source_ = transform::evaluate(*lhs_);
        ast::program const derivative_ = j ? join_programs(transform::Jacobian(source_, _wrts))
                                           : transform::derive(source_, {{source_.entries_.back().entry_name_.symbol_}}, _wrts);
        using namespace std::rel_ops;
        if (derivative_ != rhs_) {
            std::cerr << "Left hand side does not match right hand side." << std::endl
                      << "Primitive (LHS):" << std::endl << *lhs_ << std::endl
                      << "Simplified primitive (source):" << std::endl << source_ << std::endl
                      << "Derivative:" << std::endl << derivative_ << std::endl
                      << "Model (RHS):" << std::endl << rhs_ << std::endl;
            return false;
        }
        return true;
    }

    void
    test_scalars()
    {
        assert(is_primitive_of("function a(z, t) return 2.718281828 end ", "function a{x}(z, t, z{x}, t{x}) return zero end "));
        assert(is_primitive_of("function a() return l2t end ", "function a{x}() return zero end "));
        assert(is_primitive_of("function a() local b = 1 return b end ", "function a{x}() local b, b{x} = 1, 0 return b{x} end "));
        assert(is_primitive_of("function a() local b = 1 return b{c} end ", "function a{x}() local b, b{x} = 1, 0 return b{c, x} end "));
        assert(is_primitive_of("function a() return x end ", "function a{x}() return one end "));
        assert(is_primitive_of("function a() return y end ", "function a{x}() return zero end "));
        assert(is_primitive_of("function a() local b = 1 return -b end ", "function a{x}() local b, b{x} = 1, 0 return -b{x} end "));
    }

    void
    test_function_call()
    {
        assert(is_primitive_of("function b() return zero end "
                               "function a() return b() end ",
                               "function b{x}() return zero end "
                               "function a{x}() return b{x}() end "));
        assert(is_primitive_of("function b(c, d) return zero end "
                               "function a() local c, d = 1, 1 return b(c, d) end ",
                               "function b{x}(c, d, c{x}, d{x}) return zero end "
                               "function a{x}() local c, d, c{x}, d{x} = 1, 1, 0, 0 return b{x}(c, d, c{x}, d{x}) end "));
    }

    void
    test_arithmetic()
    {
        assert(is_primitive_of("function a() local z, t = 1, 1 return z + t end ",
                               "function a{x}() local z, t, z{x}, t{x} = 1, 1, 0, 0 return z{x} + t{x} end "));
        assert(is_primitive_of("function a() local z, t = 1, 1 return z - t end ",
                               "function a{x}() local z, t, z{x}, t{x} = 1, 1, 0, 0 return z{x} - t{x} end "));
        assert(is_primitive_of("function a() local z, t = 1, 1 return z * t end ",
                               "function a{x}() local z, t, z{x}, t{x} = 1, 1, 0, 0 return z{x} * t + z * t{x} end "));
        assert(is_primitive_of("function a() local z, t = 1, 1 return z / t end ",
                               "function a{x}() local z, t, z{x}, t{x} = 1, 1, 0, 0 return z{x} / t - z * t{x} / sqr(t) end "));
        assert(is_primitive_of("function a() local z, t = 1, 1 return z % t end ",
                               "function a{x}() local z, t, z{x}, t{x} = 1, 1, 0, 0 return z{x} - t{x} * trunc(z / t) end "));
        assert(is_primitive_of("function a() local z, t = 1, 1 return z ^ t end ",
                               "function a{x}() local z, t, z{x}, t{x} = 1, 1, 0, 0 return (z ^ t) * (z{x} * (t / z) + t{x} * ln(z)) end "));
        assert(is_primitive_of("function b(zt) return zero end "
                               "function a() local z, t = 1, 1 return b(z * t) end ",
                               "function b{x}(zt, zt{x}) return zero end "
                               "function a{x}() local z, t, z{x}, t{x} = 1, 1, 0, 0 return b{x}(z * t, z{x} * t + z * t{x}) end "));

    }

    void
    test_intrinsics()
    {
        assert(is_primitive_of("function a() local z = 1 return chs(z) end ",
                               "function a{x}() local z, z{x} = 1, 0 return chs(z{x}) end "));
        assert(is_primitive_of("function a() local z = 1 return twice(z) end ",
                               "function a{x}() local z, z{x} = 1, 0 return twice(z{x}) end "));
        assert(is_primitive_of("function a() local z = 1 return twice(-z) end ",
                               "function a{x}() local z, z{x} = 1, 0 return -twice(z{x}) end "));
        assert(is_primitive_of("function a() local z = 1 return sqr(z) end ",
                               "function a{x}() local z, z{x} = 1, 0 return twice(z) * z{x} end "));
        assert(is_primitive_of("function a() local z = 1 return sqrt(z) end ",
                               "function a{x}() local z, z{x} = 1, 0 return z{x} / twice(sqrt(z)) end "));
        assert(is_primitive_of("function a() local z = 1 return round(z) end ",
                               "function a{x}() local z, z{x} = 1, 0 return zero end "));
        assert(is_primitive_of("function a() local z = 1 return trunc(z) end ",
                               "function a{x}() local z, z{x} = 1, 0 return zero end "));
        assert(is_primitive_of("function a() local z = 1 return cos(z) end ",
                               "function a{x}() local z, z{x} = 1, 0 return -(sin(z) * z{x}) end "));
        assert(is_primitive_of("function a() local z = 1 return sin(z) end ",
                               "function a{x}() local z, z{x} = 1, 0 return cos(z) * z{x} end "));
        assert(is_primitive_of("function a() local z = 1 return sincos(z) end ",
                               "function a{x}() local z, z{x} = 1, 0 return cos(z) * z{x}, -(sin(z) * z{x}) end "));
        assert(is_primitive_of("function a() local z = 1 return tg(z) end ",
                               "function a{x}() local z, z{x} = 1, 0 return z{x} / sqr(cos(z)) end "));
        assert(is_primitive_of("function a() local z = 1 return ctg(z) end ",
                               "function a{x}() local z, z{x} = 1, 0 return -(z{x} / sqr(sin(z))) end "));
        assert(is_primitive_of("function a() local z = 1 return arctg(z) end ",
                               "function a{x}() local z, z{x} = 1, 0 return z{x} / (one + sqr(z)) end "));
        assert(is_primitive_of("function a() local z = 1 return frac(z) end ",
                               "function a{x}() local z, z{x} = 1, 0 return z{x} end "));
        assert(is_primitive_of("function a() local z = 1 return intrem(z) end ",
                               "function a{x}() local z, z{x} = 1, 0 return zero, z{x} end "));
        assert(is_primitive_of("function a() local z = 1 return exp(z) end ",
                               "function a{x}() local z, z{x} = 1, 0 return exp(z) * z{x} end "));
        assert(is_primitive_of("function a() local z = 1 return pow2(z) end ",
                               "function a{x}() local z, z{x} = 1, 0 return (pow2(z) * ln2) * z{x} end "));
        assert(is_primitive_of("function a() local z = 1 return ln(z) end ",
                               "function a{x}() local z, z{x} = 1, 0 return z{x} / z end "));
        assert(is_primitive_of("function a() local z = 1 return log2(z) end ",
                               "function a{x}() local z, z{x} = 1, 0 return z{x} / (z * ln2) end "));
        assert(is_primitive_of("function a() local z = 1 return lg(z) end ",
                               "function a{x}() local z, z{x} = 1, 0 return (z{x} * l2e) / (z * l2t) end "));
        assert(is_primitive_of("function a() local z = 1 return pow2m1(z) end ",
                               "function a{x}() local z, z{x} = 1, 0 return (pow2(z) * ln2) * z{x} end "));
        assert(is_primitive_of("function a() local z = 1 return arcsin(z) end ",
                               "function a{x}() local z, z{x} = 1, 0 return z{x} / sqrt(one - sqr(z)) end "));
        assert(is_primitive_of("function a() local z = 1 return arccos(z) end ",
                               "function a{x}() local z, z{x} = 1, 0 return -(z{x} / sqrt(one - sqr(z))) end "));
    }

    void
    test_statements()
    {
        assert(is_primitive_of("function a() local b = 2 return b end ",
                               "function a{x}() local b, b{x} = 2, 0 return b{x} end "));
        assert(is_primitive_of("function a() local b = one return b end ",
                               "function a{x}() local b, b{x} = one, zero return b{x} end "));
        assert(is_primitive_of("function a() local z, t = 1, 1 local b, c = z, t return b * c end ",
                               "function a{x}() local z, t, z{x}, t{x} = 1, 1, 0, 0 local b, c, b{x}, c{x} = z, t, z{x}, t{x} return b{x} * c + b * c{x} end "));
    }

    void
    test_statement_block()
    {
        assert(is_primitive_of("function a() begin local b = 2 end return x end ",
                               "function a{x}() begin local b, b{x} = 2, 0 end return 1 end "));
        assert(is_primitive_of("function a() begin local b = 2 end begin local y = 1 end return x end ",
                               "function a{x}() begin local b, b{x} = 2, 0 end begin local y, y{x} = 1, 0 end return 1 end "));
    }

    void
    test_complete_program()
    {
        assert(is_primitive_of("function a() return one end "
                               "function b() return zero end ",
                               "function b{x}() return zero end "));
    }

    void
    test_mixed_partial_derivative()
    {
        assert(is_primitive_of("function b(w)"
                               "    return zero "
                               "end "
                               "function a(z, t)"
                               "    return b(z + t)"
                               "end ",
                               "function b{x}(w, w{x})"
                               "    return zero "
                               "end "
                               "function a{x}(z, t, z{x}, t{x})"
                               "    return b{x}(z + t, z{x} + t{x})"
                               "end "
                               "function b{x, y}(w, w{x}, w{y}, w{x, y})"
                               "    return zero "
                               "end "
                               "function a{x, y}(z, t, z{x}, t{x}, z{y}, t{y}, z{x, y}, t{x, y})"
                               "    return b{x, y}(z + t, z{x} + t{x}, z{y} + t{y}, z{x, y} + t{x, y})"
                               "end ",
        {{"x"}, {"y"}}));
        assert(is_primitive_of("function b() return zero end "
                               "function a() return b() end ",
                               "function b{x}() return zero end "
                               "function a{x}() return b{x}() end "
                               "function b{x, y}() return zero end "
                               "function a{x, y}() return b{x, y}() end "
                               "function b{x, y, z}() return zero end "
                               "function a{x, y, z}() return b{x, y, z}() end "
                               "function b{x, y, z, t}() return zero end "
                               "function a{x, y, z, t}() return b{x, y, z, t}() end ",
        {{"x"}, {"y"}, {"z"}, {"t"}}));
        assert(is_primitive_of("function a(t) return zero end ",
                               "function a{x}(t, t{x}) return zero end "
                               "function a{x, x}(t, t{x}, t{x, x}) return zero end "
                               "function a{x, x, x}(t, t{x}, t{x, x}, t{x, x, x}) return zero end ",
        {{"x"}, {"x"}, {"x"}}));
        assert(is_primitive_of("function b(t) return zero end "
                               "function a(t) return b(t) end ",
                               "function b{x}(t, t{x}) return zero end "
                               "function a{x}(t, t{x}) return b{x}(t, t{x}) end /* check if there are duplicated combinations*/"
                               "function b{x, y}(t, t{x}, t{y}, t{x, y}) return zero end "
                               "function a{x, y}(t, t{x}, t{y}, t{x, y}) return b{x, y}(t, t{x}, t{y}, t{x, y}) end "
                               "function b{x, y, y}(t, t{x}, t{y}, t{x, y}, t{y, y}, t{x, y, y}) return zero end "
                               "function a{x, y, y}(t, t{x}, t{y}, t{x, y}, t{y, y}, t{x, y, y}) return b{x, y, y}(t, t{x}, t{y}, t{x, y}, t{y, y}, t{x, y, y}) end "
                               "function b{x, y, y, z}(t, t{x}, t{y}, t{x, y}, t{y, y}, t{x, y, y}, t{z}, t{x, z}, t{y, z}, t{x, y, z}, t{y, y, z}, t{x, y, y, z}) return zero end "
                               "function a{x, y, y, z}(t, t{x}, t{y}, t{x, y}, t{y, y}, t{x, y, y}, t{z}, t{x, z}, t{y, z}, t{x, y, z}, t{y, y, z}, t{x, y, y, z}) return b{x, y, y, z}(t, t{x}, t{y}, t{x, y}, t{y, y}, t{x, y, y}, t{z}, t{x, z}, t{y, z}, t{x, y, z}, t{y, y, z}, t{x, y, y, z}) end "
                               "function b{x, y, y, z, y}(t, t{x}, t{y}, t{x, y}, t{y, y}, t{x, y, y}, t{z}, t{x, z}, t{y, z}, t{x, y, z}, t{y, y, z}, t{x, y, y, z}, t{y, y, y}, t{x, y, y, y}, t{z, y}, t{x, z, y}, t{y, z, y}, t{x, y, z, y}, t{y, y, z, y}, t{x, y, y, z, y}) return zero end "
                               "function a{x, y, y, z, y}(t, t{x}, t{y}, t{x, y}, t{y, y}, t{x, y, y}, t{z}, t{x, z}, t{y, z}, t{x, y, z}, t{y, y, z}, t{x, y, y, z}, t{y, y, y}, t{x, y, y, y}, t{z, y}, t{x, z, y}, t{y, z, y}, t{x, y, z, y}, t{y, y, z, y}, t{x, y, y, z, y}) return b{x, y, y, z, y}(t, t{x}, t{y}, t{x, y}, t{y, y}, t{x, y, y}, t{z}, t{x, z}, t{y, z}, t{x, y, z}, t{y, y, z}, t{x, y, y, z}, t{y, y, y}, t{x, y, y, y}, t{z, y}, t{x, z, y}, t{y, z, y}, t{x, y, z, y}, t{y, y, z, y}, t{x, y, y, z, y}) end ",
        {{"x"}, {"y"}, {"y"}, {"z"}, {"y"}}));
        assert(is_primitive_of("function x() return zero end "
                               "function y() return zero end "
                               "function a() return x() * y() end ",
                               "function x{x}() return zero end "
                               "function y{x}() return zero end "
                               "function a{x}() return x{x}() * y() + x() * y{x}() end "
                               "function x{x, y}() return zero end "
                               "function y{y}() return zero end "
                               "function x{y}() return zero end "
                               "function y{x, y}() return zero end "
                               "function a{x, y}() return (x{x, y}() * y() + x{x}() * y{y}()) + (x{y}() * y{x}() + x() * y{x, y}()) end ",
        {{"x"}, {"y"}}));
    }

    void
    test_dependent_recursive()
    {
        assert(is_primitive_of("function leaf() return zero end "
                               "function callee() return zero end "
                               "function caller() return callee() end ",
                               "function callee{x}() return 0 end "
                               "function caller{x}() return callee{x}() end ",
        {{"x"}}));
        assert(is_primitive_of("function a() return zero end "
                               "function b() return zero end "
                               "function caller() return a() + b() end ",
                               "function a{x}() return zero end "
                               "function b{x}() return zero end "
                               "function caller{x}() return a{x}() + b{x}() end "
                               "function a{x, y}() return zero end "
                               "function b{x, y}() return zero end "
                               "function caller{x, y}() return a{x, y}() + b{x, y}() end ",
        {{"x"}, {"y"}}));
        assert(is_primitive_of("function a() return zero end "
                               "function b() return zero end "
                               "function caller() return a() * b() end ",
                               "function a{x}() return zero end "
                               "function b{x}() return zero end "
                               "function caller{x}() return a{x}() * b() + a() * b{x}() end "
                               "function a{x, y}() return zero end "
                               "function b{y}() return zero end "
                               "function a{y}() return zero end "
                               "function b{x, y}() return zero end "
                               "function caller{x, y}() return (a{x, y}() * b() + a{x}() * b{y}()) + (a{y}() * b{x}() + a() * b{x, y}()) end ",
        {{"x"}, {"y"}}));
        assert(is_primitive_of("function callee(y) return y end "
                               "function caller(y) return callee(y) end ",
                               "function callee{x}(y, y{x}) return y{x} end "
                               "function caller{x}(y, y{x}) return callee{x}(y, y{x}) end ",
        {{"x"}}));
    }

    void
    test_jacobian()
    {
        assert(is_primitive_of("function Gauss() return a * pow2(-sqr(r * (x / m - one))) end ",
                               "function Gauss{a}() return pow2(-sqr(r * (x / m - 1))) end "
                               "function Gauss{r}() return -(a * ((pow2(-sqr(r * (x / m - 1))) * ln2) * (twice(r * (x / m - 1)) * (x / m - 1)))) end "
                               "function Gauss{m}() return a * ((pow2(-sqr(r * (x / m - 1))) * ln2) * (twice(r * (x / m - 1)) * (r * (x / sqr(m))))) end ",
        {{"a"}, {"r"}, {"m"}}, true));
    }

public:

    bool
    operator () ()
    try {
        test_scalars();
        test_function_call();
        test_arithmetic();
        test_intrinsics();
        test_statements();
        test_statement_block();
        test_complete_program();
        test_mixed_partial_derivative();
        test_dependent_recursive();
        test_jacobian();
        std::cout << "Success!" << std::endl;
        return true;
    } catch (std::exception const & _exception) {
        std::cerr << "Exception raised: " << _exception.what() << std::endl;
        return false;
    } catch (...) {
        if (std::type_info * et = abi::__cxa_current_exception_type()) {
            std::cerr << "unhandled exception type: " << get_demangled_name(et->name()) << std::endl;
        } else {
            std::cerr << "unhandled unknown exception" << std::endl;
        }
        return false;
    }

};

}
#pragma clang diagnostic pop

#include <cstdlib>

int
main()
{
    if (!test{}()) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
