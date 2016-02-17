#include <insituc/ast/tokens.hpp>
#include <insituc/ast/io.hpp>
#include <insituc/parser/parser.hpp>

#include <insituc/meta/compiler.hpp>
#include <insituc/meta/io.hpp>

#include <insituc/transform/evaluator/evaluator.hpp>

#include <insituc/runtime/jit_compiler/instance.hpp>
#include <insituc/runtime/jit_compiler/translator.hpp>
#include <insituc/runtime/interpreter/virtual_machine.hpp>

#include <boost/math/constants/constants.hpp>

#include <boost/spirit/home/x3/directive/expect.hpp>

#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <limits>

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>

#include <insituc/debug/demangle.hpp>
#include <typeinfo>
#include <cxxabi.h>

#ifdef __linux__
#define RED(str) __extension__ "\e[1;31m" str "\e[0m"
#else
#define RED(str) str
#endif

namespace
{

using namespace std::string_literals;
using namespace boost::math::constants;
using namespace insituc;

class test
{

    bool const simplify_;
    bool const interpret_;

    G const eps = sqrt(std::max(std::numeric_limits< G >::epsilon(), static_cast< G >(std::numeric_limits< F >::epsilon())));

    meta::assembler assembler_;
    meta::compiler const compiler_;
    meta::symbol_mapping_type const & global_variables_;

    runtime::instance instance_;
    runtime::translator translator_;
    runtime::virtual_machine virtual_machine_;

    bool
    build(std::string const & _filename)
    {
        std::ifstream ifs_("test/cases/" + _filename);
        if (!ifs_) {
            std::cerr << "Cannot open the file \"" << _filename << '"' << std::endl;
            return false;
        }
        std::stringstream ss_;
        if (!(ss_ << ifs_.rdbuf())) {
            std::cerr << "Can't read file \"" << _filename << "\" to string stream." << std::endl;
            return false;
        }
        string_type const source_ = ss_.str();
        auto parse_result_ = parser::parse(std::cbegin(source_), std::cend(source_));
        if (!!parse_result_.error_) {
            std::cerr << "Parse failure! File: \"" << _filename << "\". Source: " << std::endl << source_ << std::endl
                      << "//< begin" << std::endl
                      << source_ << std::endl
                      << "//< end" << std::endl;
            {
                auto const & error_description_ = *parse_result_.error_;
                std::cerr << "Error: \"" << error_description_.which_
                          << "\" at input position: ";
                std::copy(error_description_.where_, error_description_.last_, std::ostreambuf_iterator< char_type >(std::cerr));
                std::cerr << std::endl;
            }
            return false;
        }
        ast::program const program_ = simplify_ ? transform::evaluate(std::move(parse_result_.ast_)) : std::move(parse_result_.ast_);
        assert(!program_.entries_.empty());
        if (!compiler_(program_)) {
            std::cerr << "Compilation error. File \"" << _filename << "\". AST: " << std::endl
                      << "//< begin" << std::endl
                      << program_ << std::endl
                      << "//< end" << std::endl
                      << "CODE:" << std::endl
                      << assembler_ << std::endl;
            return false;
        }
        if (!interpret_) {
            if (!translator_(assembler_)) {
                std::cerr << "Translation error. File \"" << _filename << "\". AST: " << std::endl
                          << "//< begin" << std::endl
                          << program_ << std::endl
                          << "//< end" << std::endl;
                return false;
            }
            instance_ = std::move(translator_);
        }
        return true;
    }

    bool
    add_global(string_type && _symbol, G && _value = G(zero))
    {
        ast::identifier global_variable_;
        global_variable_.symbol_.name_ = std::move(_symbol);
        if (assembler_.is_global_variable(global_variable_)) {
            std::cerr << "global variable with such name already exists" << std::endl;
            return false;
        }
        if (assembler_.is_reserved_symbol(global_variable_)) {
            std::cerr << "cannot add global variable: name is reserved" << std::endl;
            return false;
        }
        if (assembler_.is_dummy_placeholder(global_variable_)) {
            std::cerr << "cannot add global variable: specified name is dummy placeholder" << std::endl;
            return false;
        }
        if (assembler_.is_function(global_variable_)) {
            std::cerr << "cannot add global variable: name used as function name" << std::endl;
            return false;
        }
        assembler_.add_global_variable(std::move(global_variable_), std::move(_value));
        return true;
    }

    G
    get_global(string_type const & _symbol) const
    {
        ast::identifier global_variable_;
        global_variable_.symbol_.name_ = std::move(_symbol);
        if (!assembler_.is_global_variable(global_variable_)) {
            throw std::runtime_error("cannot find global variable with specified name");
        }
        if (interpret_) {
            return assembler_.get_global_variable(global_variable_);
        } else {
            return static_cast< G >(instance_.heap_.at(global_variables_.at(global_variable_)));
        }
    }

    template< typename ...arguments >
    G
    call(arguments &&... _arguments)
    {
        size_type const size_ = assembler_.get_export_table().size();
        assert(0 < size_);
        size_type const function_ = size_ - 1;
        assert(assembler_.get_function(function_).output_ == 1);
        if (interpret_) {
            if (!virtual_machine_(function_, std::forward< arguments >(_arguments)...)) {
                throw std::runtime_error("interpretation error");
            }
            return virtual_machine_.get_result();
        } else {
            return static_cast< G >(instance_(function_, std::forward< arguments >(_arguments)...));
        }
    }

    template< typename ...arguments >
    bool
    check(G const & _result, arguments &&... _arguments)
    {
        G const result_ = call(std::forward< arguments >(_arguments)...);
        G const delta_ = abs(result_ - _result);
        if (!(delta_ < eps)) {
            std::cerr << "Result does not match the model" << std::endl;
            std::cerr << "result = " << result_ << std::endl;
            std::cerr << "model = " << _result << std::endl;
            std::cerr << RED("delta")" = " << delta_ << std::endl;
            return false;
        }
        return true;
    }

    bool
    cleanup()
    {
        return assembler_.clear();
    }

    void
    test_simple_return()
    {
        assert(build("retlocal.txt"));
        assert(check(G(-11.2)));
        assert(cleanup());

        assert(build("retliteral.txt"));
        assert(check(G(3.02)));
        assert(cleanup());

        assert(add_global("g", G(111.1)));
        assert(build("retglobal.txt"));
        assert(check(G(111.1)));
        assert(cleanup());

        assert(build("retsumoflit.txt"));
        assert(check(G(6.778)));
        assert(cleanup());

        assert(build("passarg.txt"));
        assert(check(G(222.2), G(222.2)));
        assert(cleanup());

        assert(add_global("g"));
        assert(build("set_global.txt"));
        assert(check(zero));
        assert(abs(get_global("g") - G(1000)) < eps);
        assert(cleanup());
    }

    void
    test_assign_operators()
    {
        assert(build("assign_op/assign.txt"));
        assert(check(G(1.23), G(1000), G(1.23)));
        assert(cleanup());

        assert(build("assign_op/plus_assign.txt"));
        assert(check(G(12), G(10), G(2)));
        assert(cleanup());

        assert(build("assign_op/minus_assign.txt"));
        assert(check(G(5), G(7), G(2)));
        assert(cleanup());

        assert(build("assign_op/times_assign.txt"));
        assert(check(G(24), G(6), G(4)));
        assert(cleanup());

        assert(build("assign_op/divide_assign.txt"));
        assert(check(G(3), G(6), G(2)));
        assert(cleanup());

        assert(build("assign_op/mod_assign.txt"));
        assert(check(G(3), G(7), G(4)));
        assert(cleanup());

        assert(build("assign_op/raise_assign.txt"));
        assert(check(G(32), G(2), G(5)));
        assert(cleanup());
    }

    void
    test_binary_operators()
    {
        assert(build("binary_op/plus.txt"));
        assert(check(G(12), G(10), G(2)));
        assert(cleanup());

        assert(build("binary_op/minus.txt"));
        assert(check(G(5), G(7), G(2)));
        assert(cleanup());

        assert(build("binary_op/times.txt"));
        assert(check(G(24), G(6), G(4)));
        assert(cleanup());

        assert(build("binary_op/divide.txt"));
        assert(check(G(3), G(6), G(2)));
        assert(cleanup());

        assert(build("binary_op/mod.txt"));
        assert(check(G(3), G(7), G(4)));
        assert(cleanup());

        assert(build("binary_op/raise.txt"));
        assert(check(G(32), G(2), G(5)));
        assert(cleanup());
    }

    void
    test_unary_operators()
    {
        assert(build("unary_plus_minus.txt"));
        assert(check(one));
        assert(cleanup());
    }

    void
    test_operators_comprehensive()
    {
        assert(build("subsub.txt"));
        assert(check(zero));
        assert(cleanup());

        assert(build("operators.txt"));
        assert(check(G(7)));
        assert(cleanup());
    }

    void
    test_operators_precedence()
    {
        assert(build("precedence/0.txt"));
        assert(check(G(131)));
        assert(cleanup());

        assert(build("precedence/1.txt"));
        assert(check(G(24)));
        assert(cleanup());

        assert(build("precedence/2.txt"));
        assert(check(zero));
        assert(cleanup());
    }

    void
    test_parentheses()
    {
        assert(build("parentheses.txt"));
        assert(check(G(36)));
        assert(cleanup());
    }

    void
    test_builtin_functions()
    {
        assert(build("builtin_function_wrappers/oeoeoeoe.txt"));
        assert(check(zero));
        assert(cleanup());

        assert(build("builtin_function_wrappers/sincos.txt"));
        assert(check(zero, third_pi< G >(), half< G >() * root_three< G >(), half< G >()));
        assert(cleanup());

        assert(build("builtin_function_wrappers/chs.txt"));
        assert(check(G(-2.3), G(+2.3)));
        assert(cleanup());

        assert(build("builtin_function_wrappers/chs.txt"));
        assert(check(G(+12.3), G(-12.3)));
        assert(cleanup());

        assert(build("builtin_function_wrappers/chs.txt"));
        assert(check(zero, zero));
        assert(cleanup());

        assert(build("builtin_function_wrappers/abs.txt"));
        assert(check(G(+10), G(+10)));
        assert(cleanup());

        assert(build("builtin_function_wrappers/abs.txt"));
        assert(check(G(+11), G(-11)));
        assert(cleanup());

        assert(build("builtin_function_wrappers/abs.txt"));
        assert(check(G(+0), G(-0)));
        assert(cleanup());

        assert(build("builtin_function_wrappers/twice.txt"));
        assert(check(G(-12), G(-6)));
        assert(cleanup());

        assert(build("builtin_function_wrappers/twice.txt"));
        assert(check(zero, zero));
        assert(cleanup());

        assert(build("builtin_function_wrappers/sqrt.txt"));
        assert(check(zero, zero));
        assert(cleanup());

        assert(build("builtin_function_wrappers/sqrt.txt"));
        assert(check(G(3), G(9)));
        assert(cleanup());

        assert(build("builtin_function_wrappers/sqrt.txt"));
        assert(check(G(2), G(4)));
        assert(cleanup());

        assert(build("builtin_function_wrappers/sumsqr.txt"));
        assert(check(G(14), G(-3), one, G(2)));
        assert(cleanup());

        assert(build("builtin_function_wrappers/sumsqr_pack.txt"));
        assert(check(zero));
        assert(cleanup());

        assert(build("builtin_function_wrappers/sumsqr.txt"));
        assert(check(G(101), zero, one, G(10)));
        assert(cleanup());

        assert(build("builtin_function_wrappers/round.txt"));
        assert(check(one, G(1.49)));
        assert(cleanup());

        assert(build("builtin_function_wrappers/round.txt"));
        assert(check(G(2), G(1.5)));
        assert(cleanup());

        assert(build("builtin_function_wrappers/round.txt"));
        assert(check(G(-2), G(-1.5)));
        assert(cleanup());

        assert(build("builtin_function_wrappers/round.txt"));
        assert(check(G(2), G(2.5)));
        assert(cleanup());

        assert(build("builtin_function_wrappers/round.txt"));
        assert(check(G(-2), G(-2.5)));
        assert(cleanup());

        assert(build("builtin_function_wrappers/round.txt"));
        assert(check(zero, zero));
        assert(cleanup());

        assert(build("builtin_function_wrappers/trunc.txt"));
        assert(check(one, G(1.49)));
        assert(cleanup());

        assert(build("builtin_function_wrappers/trunc.txt"));
        assert(check(one, G(1.5)));
        assert(cleanup());

        assert(build("builtin_function_wrappers/trunc.txt"));
        assert(check(one, G(1.9)));
        assert(cleanup());

        assert(build("builtin_function_wrappers/trunc.txt"));
        assert(check(one, G(1.1)));
        assert(cleanup());

        assert(build("builtin_function_wrappers/trunc.txt"));
        assert(check(G(-1), G(-1.5)));
        assert(cleanup());

        assert(build("builtin_function_wrappers/remainder.txt"));
        assert(check(G(3), G(10), G(7)));
        assert(cleanup());

        assert(build("builtin_function_wrappers/remainder.txt"));
        assert(check(G(-3), G(11), G(7)));
        assert(cleanup());

        assert(build("builtin_function_wrappers/exp.txt"));
        assert(check(e< G >(), one));
        assert(cleanup());

        assert(build("builtin_function_wrappers/exp.txt"));
        assert(check(e_pow_pi< G >(), pi< G >()));
        assert(cleanup());

        assert(build("builtin_function_wrappers/exp.txt"));
        assert(check(exp_minus_half< G >(), -half< G >()));
        assert(cleanup());

        assert(build("builtin_function_wrappers/exp.txt"));
        assert(check(root_e< G >(), half< G >()));
        assert(cleanup());

        assert(build("builtin_function_wrappers/cos.txt"));
        assert(check(G(-1), pi< G >()));
        assert(cleanup());

        assert(build("builtin_function_wrappers/cos.txt"));
        assert(check(zero, half_pi< G >()));
        assert(cleanup());

        assert(build("builtin_function_wrappers/cos.txt"));
        assert(check(half< G >(), third_pi< G >()));
        assert(cleanup());

        assert(build("builtin_function_wrappers/sin.txt"));
        assert(check(root_three< G >() * half< G >(), third_pi< G >()));
        assert(cleanup());

        assert(build("builtin_function_wrappers/tg.txt"));
        assert(check(one, half_pi< G >() * half< G >()));
        assert(cleanup());

        assert(build("builtin_function_wrappers/tg.txt"));
        assert(check(root_three< G >(), third_pi< G >()));
        assert(cleanup());

        assert(build("builtin_function_wrappers/ctg.txt"));
        assert(check(one, half_pi< G >() * half< G >()));
        assert(cleanup());

        assert(build("builtin_function_wrappers/ctg.txt"));
        assert(check(one / root_three< G >(), third_pi< G >()));
        assert(cleanup());

        assert(build("builtin_function_wrappers/arctg.txt"));
        assert(check(half_pi< G >() * half< G >(), one));
        assert(cleanup());

        assert(build("builtin_function_wrappers/arctg.txt"));
        assert(check(third_pi< G >(), root_three< G >()));
        assert(cleanup());

        assert(build("builtin_function_wrappers/atan2.txt"));
        assert(check(sixth_pi< G >(), half< G >(), root_three< G >() * half< G >()));
        assert(cleanup());

        assert(build("builtin_function_wrappers/poly.txt"));
        assert(check(G(1.248), G(+2), one, G(0.1), G(0.01), G(0.001)));
        assert(cleanup());

        assert(build("builtin_function_wrappers/poly.txt"));
        assert(check(G(0.832), G(-2), one, G(0.1), G(0.01), G(0.001)));
        assert(cleanup());

        assert(build("builtin_function_wrappers/frac.txt"));
        assert(check(zero, pi< G >(), pi_minus_three< G >()));
        assert(cleanup());

        assert(build("builtin_function_wrappers/frac.txt"));
        assert(check(zero, e< G >(), (e< G >() - G(3))));
        assert(cleanup());

        assert(build("builtin_function_wrappers/intrem.txt"));
        assert(check(zero, pi< G >(), e< G >(), e< G >(), fmod(pi< G >(), e< G >())));
        assert(cleanup());

        assert(build("builtin_function_wrappers/intrem.txt"));
        assert(check(zero, pi< G >() + e< G >(), e< G >(), e< G >() + e< G >(), fmod(pi< G >(), e< G >())));
        assert(cleanup());

        assert(build("builtin_function_wrappers/fracint.txt"));
        assert(check(zero, pi< G >(), G(3), fmod(pi< G >(), one)));
        assert(cleanup());

        assert(build("builtin_function_wrappers/pow.txt"));
        assert(check(one_div_root_two< G >(), G(2), -half< G >()));
        assert(cleanup());

        assert(build("builtin_function_wrappers/pow2.txt"));
        assert(check(root_two< G >(), half< G >()));
        assert(cleanup());

        assert(build("builtin_function_wrappers/sqr.txt"));
        assert(check(G(9), G(3)));
        assert(cleanup());

        assert(build("builtin_function_wrappers/sqr.txt"));
        assert(check(zero, zero));
        assert(cleanup());

        assert(build("builtin_function_wrappers/sqr.txt"));
        assert(check(G(4), G(2)));
        assert(cleanup());

        assert(build("builtin_function_wrappers/sqr.txt"));
        assert(check(G(4), G(-2)));
        assert(cleanup());

        assert(build("builtin_function_wrappers/log.txt"));
        assert(check(-half< G >(), pi< G >(), one_div_root_pi< G >()));
        assert(cleanup());

        assert(build("builtin_function_wrappers/ln.txt"));
        assert(check(ln_ln_two< G >(), ln_two< G >()));
        assert(cleanup());

        assert(build("builtin_function_wrappers/log2.txt"));
        assert(check(G(10), G(1024)));
        assert(cleanup());

        assert(build("builtin_function_wrappers/lg.txt"));
        assert(check(G(3), G(1000)));
        assert(cleanup());

        assert(build("builtin_function_wrappers/yl2xp1.txt"));
        assert(check((G(3) * log2(G(1.25))), G(0.25), G(3)));
        assert(cleanup());

        assert(build("builtin_function_wrappers/scale2.txt"));
        assert(check(G(16), G(2), G(3)));
        assert(cleanup());

        assert(build("builtin_function_wrappers/scale2.txt"));
        assert(check(G(8), one, G(3.7))); // truncate
        assert(cleanup());

        assert(build("builtin_function_wrappers/scale2.txt"));
        assert(check(G(8), one, G(3.3))); // truncate
        assert(cleanup());

        assert(build("builtin_function_wrappers/scale2.txt"));
        assert(check(G(12), G(3), G(2)));
        assert(cleanup());

        assert(build("builtin_function_wrappers/extract.txt"));
        assert(check(zero, pi< G >(), one, half_pi< G >()));
        assert(cleanup());

        assert(build("builtin_function_wrappers/pow2m1.txt"));
        assert(check(root_two< G >() - one, half< G >()));
        assert(cleanup());

        assert(build("builtin_function_wrappers/arcsin.txt"));
        assert(check(sixth_pi< G >(), half< G >()));
        assert(cleanup());

        assert(build("builtin_function_wrappers/arccos.txt"));
        assert(check(third_pi< G >(), half< G >()));
        assert(cleanup());

        assert(build("builtin_function_wrappers/max.txt"));
        assert(check(pi< G >(), root_three< G >(), half< G >(), pi< G >(), e< G >()));
        assert(cleanup());

        assert(build("builtin_function_wrappers/max_pack.txt"));
        assert(check(zero));
        assert(cleanup());

        assert(build("builtin_function_wrappers/min.txt"));
        assert(check(half< G >(), root_three< G >(), half< G >(), pi< G >(), e< G >()));
        assert(cleanup());

        assert(build("builtin_function_wrappers/min_pack.txt"));
        assert(check(zero));
        assert(cleanup());
    }

    void
    test_builtin_constants()
    {
        assert(build("builtin_constant_wrappers/zero.txt"));
        assert(check(zero));
        assert(cleanup());

        assert(build("builtin_constant_wrappers/one.txt"));
        assert(check(one));
        assert(cleanup());

        assert(build("builtin_constant_wrappers/pi.txt"));
        assert(check(pi< G >()));
        assert(cleanup());

        assert(build("builtin_constant_wrappers/l2e.txt"));
        assert(check(one / ln_two< G >()));
        assert(cleanup());

        assert(build("builtin_constant_wrappers/l2t.txt"));
        assert(check(ln_ten< G >() / ln_two< G >()));
        assert(cleanup());

        assert(build("builtin_constant_wrappers/lg2.txt"));
        assert(check(ln_two< G >() * log10_e< G >()));
        assert(cleanup());

        assert(build("builtin_constant_wrappers/ln2.txt"));
        assert(check(ln_two< G >()));
        assert(cleanup());
    }

    void
    cover_binary_variants()
    {
        assert(build("cover_binary_variants.txt"));
        assert(check(zero));
        assert(cleanup());
    }

    void
    test_vector_assignment()
    {
        assert(build("vector/assign.txt"));
        assert(check(zero));
        assert(cleanup());

        assert(build("vector/plus_assign.txt"));
        assert(check(zero));
        assert(cleanup());

        assert(build("vector/minus_assign.txt"));
        assert(check(zero));
        assert(cleanup());

        assert(build("vector/times_assign.txt"));
        assert(check(zero));
        assert(cleanup());

        assert(build("vector/divide_assign.txt"));
        assert(check(zero));
        assert(cleanup());

        assert(build("vector/mod_assign.txt"));
        assert(check(zero));
        assert(cleanup());

        assert(build("vector/raise_assign.txt"));
        assert(check(zero));
        assert(cleanup());

        assert(build("vector/squeeze.txt"));
        assert(check(zero));
        assert(cleanup());

        assert(build("vector/function_assign.txt"));
        assert(check(zero));
        assert(cleanup());

        assert(build("vector/variables_declaration.txt"));
        assert(check(zero));
        assert(cleanup());

        assert(build("vector/variables_declaration0.txt"));
        assert(check(zero));
        assert(cleanup());
    }

    void
    test_logical_brackets()
    {
        assert(build("brackets/1.txt"));
        assert(check(G(3)));
        assert(cleanup());

        assert(build("brackets/2.txt"));
        assert(check(zero));
        assert(cleanup());

        assert(build("brackets/3.txt"));
        assert(check(zero));
        assert(cleanup());
    }

    void
    stack_overflow()
    {
        assert(build("stackoverflow/rassoc8.txt"));
        assert(check(G(25)));
        assert(cleanup());

        assert(build("stackoverflow/1.txt"));
        assert(check(G(253)));
        assert(cleanup());

        assert(build("stackoverflow/2.txt"));
        assert(check(G(391)));
        assert(cleanup());

        assert(build("stackoverflow/very_hard.txt"));
        assert(check(G(22775)));
        assert(cleanup());
    }

public:

    test(bool const _simplify, bool const _interpret)
        : simplify_(_simplify)
        , interpret_(_interpret)
        , assembler_()
        , compiler_(assembler_)
        , global_variables_(assembler_.get_heap_symbols())
        , virtual_machine_(assembler_)
    { ; }

    bool
    operator () ()
    try {
        test_simple_return();
        test_assign_operators();
        test_binary_operators();
        test_unary_operators();
        test_builtin_functions();
        test_builtin_constants();
        test_operators_comprehensive();
        test_operators_precedence();
        test_parentheses();
        cover_binary_variants();
        test_vector_assignment();
        test_logical_brackets();
        stack_overflow();
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

#include <cstdlib>

int
main()
{
    if (!test{false, false}()) {
        return EXIT_FAILURE;
    }
    if (!test{true, false}()) {
        return EXIT_FAILURE;
    }
    if (!test{false, true}()) {
        return EXIT_FAILURE;
    }
    if (!test{true, true}()) {
        return EXIT_FAILURE;
    }
    std::cout << "Success!" << std::endl;
    return EXIT_SUCCESS;
}
