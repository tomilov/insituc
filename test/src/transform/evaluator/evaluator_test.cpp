#include <insituc/transform/evaluator/expression.hpp>
#include <insituc/floating_point_type.hpp>

#include <insituc/base_types.hpp>
#include <insituc/variant.hpp>
#include <insituc/ast/ast.hpp>
#include <insituc/ast/io.hpp>
#include <insituc/ast/compare.hpp>
#include <insituc/utility/append.hpp>

#include <boost/math/constants/constants.hpp>

#include <utility>
#include <iterator>
#include <exception>
#include <iostream>
#include <string>
#include <initializer_list>
#include <limits>
#include <random>
#include <chrono>

#include <cmath>

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>

#include <insituc/debug/demangle.hpp>
#include <typeinfo>
#include <cxxabi.h>

namespace floating_point_type // extend the definition for <random>
{

template< typename I, typename F >
std::enable_if_t< std::is_integral_v< I >, G< F > >
operator + (I i, G< F > g)
{
    return static_cast< G< F > >(F(i) + static_cast< F >(std::move(g)));
}

template< typename I, typename F >
std::enable_if_t< std::is_integral_v< I >, G< F > >
operator - (I i, G< F > g)
{
    return static_cast< G< F > >(F(i) - static_cast< F >(std::move(g)));
}

template< typename I, typename F >
std::enable_if_t< std::is_integral_v< I >, G< F > >
operator + (G< F > g, I i)
{
    return static_cast< G< F > >(static_cast< F >(std::move(g)) + F(i));
}

template< typename I, typename F >
std::enable_if_t< std::is_integral_v< I >, G< F > >
operator - (G< F > g, I i)
{
    return static_cast< G< F > >(static_cast< F >(std::move(g)) - F(i));
}

template< typename I, typename F >
std::enable_if_t< std::is_integral_v< I >, G< F > >
operator * (I i, G< F > g)
{
    return static_cast< G< F > >(F(i) * static_cast< F >(std::move(g)));
}

template< typename I, typename F >
std::enable_if_t< std::is_integral_v< I >, G< F > >
operator / (I i, G< F > g)
{
    return static_cast< G< F > >(F(i) / static_cast< F >(std::move(g)));
}

template< typename I, typename F >
std::enable_if_t< std::is_integral_v< I >, G< F > >
operator * (G< F > g, I i)
{
    return static_cast< G< F > >(static_cast< F >(std::move(g)) * F(i));
}

template< typename I, typename F >
std::enable_if_t< std::is_integral_v< I >, G< F > >
operator / (G< F > g, I i)
{
    return static_cast< G< F > >(static_cast< F >(std::move(g)) / F(i));
}

} // namespace floating_point_type

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
namespace
{

using namespace std::rel_ops;

using namespace boost::math::constants;

using namespace insituc;

class test
{

    using E = ast::expression;
    using O = ast::operand;
    using R = ast::rvalue_list;
    using B = ast::binary_expression;
    using U = ast::unary_expression;
    using S = ast::symbol;
    using N = ast::identifier;
    using I = ast::intrinsic_invocation;

    G const eps = sqrt(std::numeric_limits< G >::epsilon());

    using seed_type = typename std::default_random_engine::result_type;
    seed_type seed_;
    std::default_random_engine random_;

    void
    set_seed(seed_type const _seed)
    {
        seed_ = _seed;
        random_.seed(seed_);
    }

    void
    set_seed()
    {
#if 0
        std::random_device rd_;
        set_seed(rd_());
#else
        set_seed(static_cast< seed_type >(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
#endif
    }

    std::uniform_real_distribution< G > zero_to_one_; // uniform [0;1) ditribution
    std::uniform_int_distribution< int > uniform_int_;

    bool
    is_reduceable(O const & _from, O const & _to) const
    {
        O const copy_simplified_ = transform::evaluate(_from);
        if (copy_simplified_ != _to) {
            std::cerr << "copy-simplified not equal to objective: " << std::endl;
            std::cerr << copy_simplified_ << std::endl;
            std::cerr << "from:" << std::endl;
            std::cerr << _from << std::endl;
            std::cerr << "to:" << std::endl;
            std::cerr << _to << std::endl;
            return false;
        }
        O const move_simplified_ = transform::evaluate(O(_from));
        if (move_simplified_ != _to) {
            std::cerr << "move-simplified not equal to objective: " << std::endl;
            std::cerr << move_simplified_ << std::endl;
            std::cerr << "from:" << std::endl;
            std::cerr << _from << std::endl;
            std::cerr << "to:" << std::endl;
            std::cerr << _to << std::endl;
            return false;
        }
        return true;
    }/* TODO:

    bool
    is_reduceable(O const & _from, R const & _to) const
    {
        O const copy_simplified_ = transform::evaluate(_from);
        if (copy_simplified_ != _to) {
            std::cerr << "copy-simplified not equal to objective: " << std::endl;
            std::cerr << copy_simplified_ << std::endl;
            std::cerr << "from:" << std::endl;
            std::cerr << _from << std::endl;
            std::cerr << "to:" << std::endl;
            std::cerr << _to << std::endl;
            return false;
        }
        O const move_simplified_ = transform::evaluate(O(_from));
        if (move_simplified_ != _to) {
            std::cerr << "move-simplified not equal to objective: " << std::endl;
            std::cerr << move_simplified_ << std::endl;
            std::cerr << "from:" << std::endl;
            std::cerr << _from << std::endl;
            std::cerr << "to:" << std::endl;
            std::cerr << _to << std::endl;
            return false;
        }
        return true;
    }*/

    bool
    is_reduceable(O const & _from, G const & _to) const
    {
        O const simplified_ = transform::evaluate(_from);
        O const move_simplified_ = transform::evaluate(O(_from));
        if (simplified_ != move_simplified_) {
            return false;
        }
        try {
            auto const & lhs_ = get< G const & >(simplified_);
            G const & rhs_ = _to;
            if (lhs_ != lhs_) {
                return (rhs_ != rhs_);
            }
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wfloat-equal"
            if (lhs_ == rhs_) { // isinf?
#pragma clang diagnostic pop
                return true;
            }
            G const delta_ = abs(lhs_ - rhs_);
            if (delta_ < eps) {
                return true;
            } else {
                std::cout << "delta = " << delta_ << std::endl;
            }
        } catch (std::bad_cast const &) {
            std::cerr << "lhs not containing a value of floating-point type" << std::endl;
        }
        return false;
    }

    bool
    is_immutable(O const & _unchangeable) const
    {
        O const immutable_ = transform::evaluate(_unchangeable);
        O const move_immutable_ = transform::evaluate(O(_unchangeable));
        if (immutable_ != move_immutable_) {
            return false;
        }
        return (immutable_ == _unchangeable);
    }

    I
    invoke(ast::intrinsic const _intrinsic, std::initializer_list< O > _il) const
    {
        return {_intrinsic, R{_il}};
    }

    bool
    is_reduceable(ast::intrinsic const _intrinsic, std::initializer_list< O > _il, G const & _rhs) const
    {
        return is_reduceable(invoke(_intrinsic, _il), _rhs);
    }

    bool
    is_reduceable(ast::intrinsic const _intrinsic, std::initializer_list< O > _il, O const & _rhs) const
    {
        return is_reduceable(invoke(_intrinsic, _il), _rhs);
    }

    bool
    is_immutable(ast::intrinsic const _intrinsic, std::initializer_list< O > _il) const
    {
        return is_immutable(invoke(_intrinsic, _il));
    }

    G
    unsigned_value()
    {
        return scalbn(one + zero_to_one_(random_), uniform_int_(random_));
    }

    G
    add_sign(G && _value)
    {
        return (uniform_int_(random_) % 2 == 0) ? -std::move(_value) : std::move(_value);
    }

    G
    signed_value()
    {
        return add_sign(unsigned_value());
    }

    void
    test_floating_point()
    {
        assert(is_immutable(zero));
        assert(is_immutable(one));
        assert(is_immutable(std::numeric_limits< G >::max()));
        assert(is_immutable(std::numeric_limits< G >::min()));
        assert(is_immutable(std::numeric_limits< G >::lowest()));

        for (size_type i = 0; i < 100; ++i) {
            G value_ = signed_value();
            if (!is_immutable(value_)) {
                std::cerr << value_ << std::endl;
                assert(false);
            }
        }
    }

    void
    test_constant()
    {
        assert(is_reduceable(ast::constant::zero, zero));
        assert(is_reduceable(ast::constant::one, one));
        assert(is_immutable(ast::constant::pi));
        assert(is_immutable(ast::constant::l2e));
        assert(is_immutable(ast::constant::l2t));
        assert(is_immutable(ast::constant::lg2));
        assert(is_immutable(ast::constant::ln2));
    }

    void
    test_identifier()
    {
        N a;
        a.symbol_ = S{"a"};
        a.derive(S{"b"});
        a.derive(S{"c"});
        N b;
        b.symbol_ = S{"b"};
        assert(is_immutable(a));
        assert(is_immutable(b));
    }

    void
    test_unary_expression()
    {
        N a;
        a.symbol_ = S{"a"};
        U const u0{ast::unary::plus, zero};
        U const u1{ast::unary::plus, one};
        U const u2{ast::unary::minus, zero};
        U const u3{ast::unary::minus, one};
        U const u4{ast::unary::plus, ast::constant::zero};
        U const u5{ast::unary::plus, ast::constant::one};
        U const u6{ast::unary::minus, ast::constant::zero};
        U const u7{ast::unary::minus, ast::constant::one};
        U const u8{ast::unary::minus, ast::constant::pi};
        U const u9{ast::unary::plus, ast::constant::pi};
        U const u10{ast::unary::minus, a};
        U const u11{ast::unary::plus, a};
        U const u12{ast::unary::minus, u3};
        U const u14{ast::unary::minus, u7};
        assert(is_reduceable(u0, zero));
        assert(is_reduceable(u1, one));
        assert(is_reduceable(u2, zero));
        assert(is_reduceable(u3, -one));
        assert(is_reduceable(u4, zero));
        assert(is_reduceable(u5, one));
        assert(is_reduceable(u6, zero));
        assert(is_reduceable(u7, -one));
        assert(is_immutable(u8));
        assert(is_reduceable(u9, ast::constant::pi));
        assert(is_immutable(u10));
        assert(is_reduceable(u11, a));
        assert(is_reduceable(u12, one));
        assert(is_reduceable(u14, one));
    }

    void
    test_intrinsic()
    {
        N a_;
        a_.symbol_ = S{"a"};
        N b_;
        b_.symbol_ = S{"b"};
        N c_;
        c_.symbol_ = S{"c"};
        N d_;
        d_.symbol_ = S{"d"};
        assert(is_reduceable(ast::intrinsic::chs, {one}, -one));
        assert(is_immutable(ast::intrinsic::chs, {ast::constant::l2t}));
        assert(is_reduceable(ast::intrinsic::chs, {U{ast::unary::minus, a_}}, a_));
        assert(is_reduceable(ast::intrinsic::chs, {I{ast::intrinsic::chs, {{a_}}}}, a_));

        assert(is_reduceable(ast::intrinsic::abs, {-one}, one));
        assert(is_reduceable(ast::intrinsic::abs, {ast::constant::one}, one));
        assert(is_reduceable(ast::intrinsic::abs, {U{ast::unary::minus, a_}}, I{ast::intrinsic::abs, {{a_}}}));
        assert(is_reduceable(ast::intrinsic::abs, {I{ast::intrinsic::chs, {{a_}}}}, I{ast::intrinsic::abs, {{a_}}}));
        assert(is_reduceable(ast::intrinsic::abs, {I{ast::intrinsic::abs, {{a_}}}}, I{ast::intrinsic::abs, {{a_}}}));
        assert(is_reduceable(ast::intrinsic::abs, {I{ast::intrinsic::sqrt, {{a_}}}}, I{ast::intrinsic::sqrt, {{a_}}}));
        assert(is_reduceable(ast::intrinsic::abs, {I{ast::intrinsic::exp, {{a_}}}}, I{ast::intrinsic::exp, {{a_}}}));
        assert(is_reduceable(ast::intrinsic::abs, {I{ast::intrinsic::arccos, {{a_}}}}, I{ast::intrinsic::arccos, {{a_}}}));
        assert(is_reduceable(ast::intrinsic::abs, {I{ast::intrinsic::pow2, {{a_}}}}, I{ast::intrinsic::pow2, {{a_}}}));
        assert(is_reduceable(ast::intrinsic::abs, {I{ast::intrinsic::sqr, {{a_}}}}, I{ast::intrinsic::sqr, {{a_}}}));
        assert(is_reduceable(ast::intrinsic::abs, {I{ast::intrinsic::sumsqr, {{a_, ast::constant::l2e}}}}, I{ast::intrinsic::sumsqr, {{a_, ast::constant::l2e}}}));

        assert(is_reduceable(ast::intrinsic::twice, {ast::constant::one}, G(2)));
        assert(is_immutable(ast::intrinsic::twice, {ast::constant::l2t}));
        assert(is_reduceable(ast::intrinsic::twice, {U{ast::unary::minus, a_}}, U{ast::unary::minus, I{ast::intrinsic::twice, {{a_}}}}));
        assert(is_reduceable(ast::intrinsic::twice, {I{ast::intrinsic::chs, {{a_}}}}, I{ast::intrinsic::chs, {{I{ast::intrinsic::twice, {{a_}}}}}}));
        assert(is_reduceable(ast::intrinsic::twice, {I{ast::intrinsic::abs, {{a_}}}}, I{ast::intrinsic::abs, {{I{ast::intrinsic::twice, {{a_}}}}}}));

        assert(is_reduceable(ast::intrinsic::sumsqr, {G(-3), G(4)}, G(25)));
        assert(is_reduceable(ast::intrinsic::sumsqr, {zero, zero}, zero));
        assert(is_reduceable(ast::intrinsic::sumsqr, {ast::constant::one}, one));
        assert(is_reduceable(ast::intrinsic::sumsqr, {ast::constant::zero}, zero));
        assert(is_immutable(ast::intrinsic::sumsqr, {ast::constant::l2t, ast::constant::lg2}));
        for (size_type i = 0; i < 100; ++i) {
            G x_ = add_sign(sqrt(unsigned_value() / (one + one)));
            G y_ = add_sign(sqrt(unsigned_value() / (one + one)));
            if (!is_reduceable(ast::intrinsic::sumsqr, {x_, y_}, x_ * x_ + y_ * y_)) {
                std::cerr << x_ << ' ' << y_ << std::endl;
                assert(false);
            }
        }

        assert(is_immutable(ast::intrinsic::sqrt, {a_}));
        assert(is_reduceable(ast::intrinsic::sqrt, {one}, one));
        assert(is_reduceable(ast::intrinsic::sqrt, {zero}, zero));
        assert(is_reduceable(ast::intrinsic::sqrt, {G(4)}, G(2)));
        assert(is_reduceable(ast::intrinsic::sqrt, {G(100)}, G(10)));
        assert(is_reduceable(ast::intrinsic::sqrt, {I{ast::intrinsic::sqr, {{a_}}}}, I{ast::intrinsic::abs, {{a_}}}));

        assert(is_reduceable(ast::intrinsic::round, {ast::constant::one}, one));
        assert(is_reduceable(ast::intrinsic::round, {one}, one));
        assert(is_reduceable(ast::intrinsic::round, {G(1.49)}, one));
        assert(is_reduceable(ast::intrinsic::round, {G(1.5)}, G(2)));
        assert(is_reduceable(ast::intrinsic::round, {G(-1.5)}, G(-2)));
        assert(is_reduceable(ast::intrinsic::round, {G(2.5)}, G(2)));
        assert(is_reduceable(ast::intrinsic::round, {G(-2.5)}, G(-2)));
        assert(is_reduceable(ast::intrinsic::round, {G(-3.5)}, G(-4)));
        assert(is_reduceable(ast::intrinsic::round, {ast::constant::pi}, G(3)));
        assert(is_reduceable(ast::intrinsic::round, {I{ast::intrinsic::chs, {{a_}}}}, I{ast::intrinsic::chs, {{I{ast::intrinsic::round, {{a_}}}}}}));
        assert(is_reduceable(ast::intrinsic::round, {I{ast::intrinsic::abs, {{a_}}}}, I{ast::intrinsic::abs, {{I{ast::intrinsic::round, {{a_}}}}}}));
        assert(is_reduceable(ast::intrinsic::round, {I{ast::intrinsic::round, {{a_}}}}, I{ast::intrinsic::round, {{a_}}}));
        assert(is_reduceable(ast::intrinsic::round, {I{ast::intrinsic::trunc, {{a_}}}}, I{ast::intrinsic::trunc, {{a_}}}));
        assert(is_reduceable(ast::intrinsic::round, {I{ast::intrinsic::frac, {{a_}}}}, zero));

        assert(is_reduceable(ast::intrinsic::trunc, {ast::constant::one}, one));
        assert(is_reduceable(ast::intrinsic::trunc, {one}, one));
        assert(is_reduceable(ast::intrinsic::trunc, {G(1.49)}, one));
        assert(is_reduceable(ast::intrinsic::trunc, {G(1.5)}, one));
        assert(is_reduceable(ast::intrinsic::trunc, {G(-1.5)}, -one));
        assert(is_reduceable(ast::intrinsic::trunc, {G(2.5)}, G(2)));
        assert(is_reduceable(ast::intrinsic::trunc, {G(-2.5)}, G(-2)));
        assert(is_reduceable(ast::intrinsic::trunc, {G(-3.5)}, G(-3)));
        assert(is_reduceable(ast::intrinsic::trunc, {ast::constant::pi}, G(3)));
        assert(is_reduceable(ast::intrinsic::trunc, {I{ast::intrinsic::chs, {{a_}}}}, I{ast::intrinsic::chs, {{I{ast::intrinsic::trunc, {{a_}}}}}}));
        assert(is_reduceable(ast::intrinsic::trunc, {I{ast::intrinsic::abs, {{a_}}}}, I{ast::intrinsic::abs, {{I{ast::intrinsic::trunc, {{a_}}}}}}));
        assert(is_reduceable(ast::intrinsic::trunc, {I{ast::intrinsic::round, {{a_}}}}, I{ast::intrinsic::round, {{a_}}}));
        assert(is_reduceable(ast::intrinsic::trunc, {I{ast::intrinsic::trunc, {{a_}}}}, I{ast::intrinsic::trunc, {{a_}}}));
        assert(is_reduceable(ast::intrinsic::trunc, {I{ast::intrinsic::frac, {{a_}}}}, zero));

        // The two instructions differ in the way the notional round-to-integer operation is performed.
        // FPREM does it by rounding towards zero, so that the remainder it returns always has the same sign as the original value in ST0;
        // FPREM1 does it by rounding to the nearest integer, so that the remainder always has at most half the magnitude of ST1.
        assert(is_reduceable(ast::intrinsic::remainder, {G(11), G(7)}, G(-3)));
        assert(is_reduceable(ast::intrinsic::remainder, {G(10), G(7)}, G(3)));

        assert(is_immutable(ast::intrinsic::cos, {a_}));
        assert(is_reduceable(ast::intrinsic::cos, {pi< G >()}, -one));
        assert(is_reduceable(ast::intrinsic::cos, {zero}, one));
        assert(is_immutable(ast::intrinsic::sin, {a_}));
        assert(is_reduceable(ast::intrinsic::sin, {pi< G >()}, zero));
        assert(is_reduceable(ast::intrinsic::sin, {zero}, zero));
        assert(is_reduceable(ast::intrinsic::sin, {half_pi< G >()}, one));
        assert(is_reduceable(ast::intrinsic::sin, {-half_pi< G >()}, -one));
        assert(is_immutable(ast::intrinsic::tg, {ast::constant::ln2}));
        assert(is_reduceable(ast::intrinsic::tg, {half< G >() * half_pi< G >()}, one));
        assert(is_reduceable(ast::intrinsic::tg, {zero}, zero));
        assert(is_immutable(ast::intrinsic::ctg, {ast::constant::ln2}));
        assert(is_reduceable(ast::intrinsic::ctg, {-half< G >() * half_pi< G >()}, -one));

        assert(is_reduceable(ast::intrinsic::atan2, {half< G >(), root_three< G >() * half< G >()}, sixth_pi< G >()));

        {
            G pi_ = pi< G >();
            assert(is_reduceable(ast::intrinsic::poly, {a_}, zero));
            assert(is_reduceable(ast::intrinsic::poly, {zero}, zero));
            assert(is_reduceable(ast::intrinsic::poly, {one}, zero));

            assert(is_reduceable(ast::intrinsic::poly, {a_, b_}, b_));
            assert(is_reduceable(ast::intrinsic::poly, {zero, b_}, b_));
            assert(is_reduceable(ast::intrinsic::poly, {one, b_}, b_));
            assert(is_reduceable(ast::intrinsic::poly, {-one, b_}, b_));

            assert(is_reduceable(ast::intrinsic::poly, {a_, b_, c_}, B{b_, ast::binary::add, B{c_, ast::binary::mul, a_}}));
            assert(is_reduceable(ast::intrinsic::poly, {zero, b_, c_}, b_));
            assert(is_reduceable(ast::intrinsic::poly, {one, b_, c_}, B{b_, ast::binary::add, c_}));
            assert(is_reduceable(ast::intrinsic::poly, {-one, b_, c_}, B{b_, ast::binary::sub, c_}));

            assert(is_reduceable(ast::intrinsic::poly, {zero, b_, c_, d_}, b_));
            assert(is_reduceable(ast::intrinsic::poly, {zero, pi_, c_, d_}, pi_));

            {
                assert(is_reduceable(ast::intrinsic::poly, {one, b_, c_, d_}, B{B{b_, ast::binary::add, c_}, ast::binary::add, d_}));

                assert(is_reduceable(ast::intrinsic::poly, {one, pi_, c_, d_}, B{B{c_, ast::binary::add, d_}, ast::binary::add, pi_}));
                assert(is_reduceable(ast::intrinsic::poly, {one, b_, pi_, d_}, B{B{b_, ast::binary::add, d_}, ast::binary::add, pi_}));
                assert(is_reduceable(ast::intrinsic::poly, {one, b_, c_, pi_}, B{B{b_, ast::binary::add, c_}, ast::binary::add, pi_}));

                assert(is_reduceable(ast::intrinsic::poly, {one, -pi_, c_, d_}, B{B{c_, ast::binary::add, d_}, ast::binary::sub, pi_}));
                assert(is_reduceable(ast::intrinsic::poly, {one, b_, -pi_, d_}, B{B{b_, ast::binary::add, d_}, ast::binary::sub, pi_}));
                assert(is_reduceable(ast::intrinsic::poly, {one, b_, c_, -pi_}, B{B{b_, ast::binary::add, c_}, ast::binary::sub, pi_}));

                assert(is_reduceable(ast::intrinsic::poly, {one, b_, pi_, pi_}, B{b_, ast::binary::add, pi_ + pi_}));
                assert(is_reduceable(ast::intrinsic::poly, {one, pi_, c_, pi_}, B{c_, ast::binary::add, pi_ + pi_}));
                assert(is_reduceable(ast::intrinsic::poly, {one, pi_, pi_, d_}, B{d_, ast::binary::add, pi_ + pi_}));

                assert(is_reduceable(ast::intrinsic::poly, {one, b_, -pi_, -pi_}, B{b_, ast::binary::sub, pi_ + pi_}));
                assert(is_reduceable(ast::intrinsic::poly, {one, -pi_, c_, -pi_}, B{c_, ast::binary::sub, pi_ + pi_}));
                assert(is_reduceable(ast::intrinsic::poly, {one, -pi_, -pi_, d_}, B{d_, ast::binary::sub, pi_ + pi_}));

                assert(is_reduceable(ast::intrinsic::poly, {one, b_, pi_, -pi_}, b_));
                assert(is_reduceable(ast::intrinsic::poly, {one, pi_, c_, -pi_}, c_));
                assert(is_reduceable(ast::intrinsic::poly, {one, pi_, -pi_, d_}, d_));

                assert(is_reduceable(ast::intrinsic::poly, {one, b_, pi_, -pi_}, b_));
                assert(is_reduceable(ast::intrinsic::poly, {one, pi_, c_, -pi_}, c_));
                assert(is_reduceable(ast::intrinsic::poly, {one, pi_, -pi_, d_}, d_));

                assert(is_reduceable(ast::intrinsic::poly, {one, pi_, pi_, pi_}, (pi_ + pi_ + pi_)));
            }
            {
                assert(is_reduceable(ast::intrinsic::poly, {-one, b_, c_, d_}, B{B{b_, ast::binary::add, d_}, ast::binary::sub, c_}));
                assert(is_reduceable(ast::intrinsic::poly, {-one, a_, b_, c_, d_}, B{B{a_, ast::binary::add, c_}, ast::binary::sub, B{b_, ast::binary::add, d_}}));

                assert(is_reduceable(ast::intrinsic::poly, {-one, pi_, c_, d_}, B{B{d_, ast::binary::add, pi_}, ast::binary::sub, c_}));
                assert(is_reduceable(ast::intrinsic::poly, {-one, -pi_, c_, d_}, B{d_, ast::binary::sub, B{c_, ast::binary::add, pi_}}));
                assert(is_reduceable(ast::intrinsic::poly, {-one, b_, pi_, d_}, B{B{b_, ast::binary::add, d_}, ast::binary::sub, pi_}));
                assert(is_reduceable(ast::intrinsic::poly, {-one, b_, -pi_, d_}, B{B{b_, ast::binary::add, d_}, ast::binary::add, pi_}));
                assert(is_reduceable(ast::intrinsic::poly, {-one, b_, c_, pi_}, B{B{b_, ast::binary::add, pi_}, ast::binary::sub, c_}));
                assert(is_reduceable(ast::intrinsic::poly, {-one, b_, c_, -pi_}, B{b_, ast::binary::sub, B{c_, ast::binary::add, pi_}}));

                assert(is_reduceable(ast::intrinsic::poly, {-one, b_, pi_, pi_}, b_));
                assert(is_reduceable(ast::intrinsic::poly, {-one, pi_, c_, pi_}, B{pi_ + pi_, ast::binary::sub, c_}));
                assert(is_reduceable(ast::intrinsic::poly, {-one, pi_, pi_, d_}, d_));

                assert(is_reduceable(ast::intrinsic::poly, {-one, b_, pi_, -pi_}, B{b_, ast::binary::sub, pi_ + pi_}));
                assert(is_reduceable(ast::intrinsic::poly, {-one, pi_, c_, -pi_}, U{ast::unary::minus, c_}));
                assert(is_reduceable(ast::intrinsic::poly, {-one, pi_, -pi_, d_}, B{d_, ast::binary::add, pi_ + pi_}));

                assert(is_reduceable(ast::intrinsic::poly, {-one, b_, -pi_, pi_}, B{b_, ast::binary::add, pi_ + pi_}));
                assert(is_reduceable(ast::intrinsic::poly, {-one, -pi_, c_, pi_}, U{ast::unary::minus, c_}));
                assert(is_reduceable(ast::intrinsic::poly, {-one, -pi_, pi_, d_}, B{d_, ast::binary::sub, pi_ + pi_}));

                assert(is_reduceable(ast::intrinsic::poly, {-one, one, G(2), G(4)}, G(3)));
            }
            {
                G two = one + one;
                assert(is_immutable(ast::intrinsic::poly, {two, b_, c_, d_}));
                assert(is_immutable(ast::intrinsic::poly, {two, a_, b_, c_, d_}));

                assert(is_immutable(ast::intrinsic::poly, {two, pi_, c_, d_}));
                assert(is_immutable(ast::intrinsic::poly, {two, -pi_, c_, d_}));
                assert(is_reduceable(ast::intrinsic::poly, {two, b_, pi_, d_}, I{ast::intrinsic::poly, {{two, B{b_, ast::binary::add, two * pi_}, zero, d_}}}));
                assert(is_reduceable(ast::intrinsic::poly, {two, b_, -pi_, d_}, I{ast::intrinsic::poly, {{two, B{b_, ast::binary::sub, two * pi_}, zero, d_}}}));
                assert(is_reduceable(ast::intrinsic::poly, {two, b_, c_, pi_}, I{ast::intrinsic::poly, {{two, B{b_, ast::binary::add, two * two * pi_}, c_, zero}}}));
                assert(is_reduceable(ast::intrinsic::poly, {two, b_, c_, -pi_}, I{ast::intrinsic::poly, {{two, B{b_, ast::binary::sub, two * two * pi_}, c_, zero}}}));

                assert(is_reduceable(ast::intrinsic::poly, {two, b_, pi_, pi_}, I{ast::intrinsic::poly, {{two, B{b_, ast::binary::add, two * two * pi_ + two * pi_}, zero, zero}}}));
                assert(is_reduceable(ast::intrinsic::poly, {two, pi_, c_, pi_}, I{ast::intrinsic::poly, {{two, two * two * pi_ + pi_, c_, zero}}}));
                assert(is_reduceable(ast::intrinsic::poly, {two, pi_, pi_, d_}, I{ast::intrinsic::poly, {{two, two * pi_ + pi_, zero, d_}}}));
            }

            assert(is_reduceable(ast::intrinsic::poly, {G(+2), one, G(0.05), G(0.0025), G(0.000125)}, G(1.111)));
            assert(is_reduceable(ast::intrinsic::poly, {G(+2), one, G(0.1), G(0.01), G(0.001)}, G(1.248)));

            assert(is_reduceable(ast::intrinsic::poly, {G(+2), one, G(0.25), G(0.0625), G(0.015625)}, G(1.875)));
            assert(is_reduceable(ast::intrinsic::poly, {G(-2), one, G(0.25), G(0.0625), G(0.015625)}, G(0.625)));
        }

        assert(is_immutable(ast::intrinsic::frac, {a_}));
        assert(is_reduceable(ast::intrinsic::frac, {G(1.5)}, G(-0.5)));
        assert(is_reduceable(ast::intrinsic::frac, {G(0.25)}, G(0.25)));
        assert(is_reduceable(ast::intrinsic::frac, {-one}, zero));
        assert(is_reduceable(ast::intrinsic::frac, {ast::constant::zero}, zero));
        assert(is_reduceable(ast::intrinsic::frac, {ast::constant::one}, zero));

        {
            G x_ = fmod(pi< G >(), e< G >());
            G y_ = e< G >();
            assert(is_reduceable(ast::intrinsic::sumsqr, {invoke(ast::intrinsic::intrem, {pi< G >(), e< G >()})}, x_ * x_ + y_ * y_));
        }

        assert(is_immutable(ast::intrinsic::pow, {a_, b_}));
        assert(is_reduceable(ast::intrinsic::pow, {G(3), G(2)}, G(9)));
        assert(is_reduceable(ast::intrinsic::pow, {G(3), G(1)}, G(3)));
        assert(is_reduceable(ast::intrinsic::pow, {G(3), G(0)}, one));
        assert(is_reduceable(ast::intrinsic::pow, {G(2), -one}, G(0.5)));
        assert(is_reduceable(ast::intrinsic::pow, {G(4), G(0.5)}, G(2)));
        assert(is_reduceable(ast::intrinsic::pow, {zero, zero}, one));
        for (size_type i = 0; i < 100; ++i) {
            G power_ = unsigned_value();
            if (!is_reduceable(ast::intrinsic::pow, {zero, power_}, zero)) {
                std::cerr << power_ << std::endl;
                assert(false);
            }
        }
        assert(is_reduceable(ast::intrinsic::pow, {one, G(2)}, one));
        assert(is_reduceable(ast::intrinsic::pow, {one, G(-2)}, one));
        assert(is_reduceable(ast::intrinsic::pow, {one, zero}, one));
        for (size_type i = 0; i < 100; ++i) {
            G power_ = signed_value();
            if (!is_reduceable(ast::intrinsic::pow, {one, power_}, one)) {
                std::cerr << power_ << std::endl;
                assert(false);
            }
        }
        for (size_type i = 0; i < 100; ++i) {
            G base_ = unsigned_value();
            G power_ = signed_value();
            G pow_ = pow(base_, power_);
            if (pow_ == pow_) {
                if (!is_reduceable(ast::intrinsic::pow, {base_, power_}, pow_)) {
                    std::cerr << base_ << ' ' << power_ << ' ' << pow_ << std::endl;
                    assert(false);
                }
            }
        }

        assert(is_immutable(ast::intrinsic::pow2, {a_}));
        assert(is_reduceable(ast::intrinsic::pow2, {G(-2)}, G(0.25)));
        assert(is_reduceable(ast::intrinsic::pow2, {G(2)}, G(4)));
        assert(is_reduceable(ast::intrinsic::pow2, {one}, G(2)));

        assert(is_reduceable(ast::intrinsic::sqr, {ast::constant::one}, one));
        assert(is_reduceable(ast::intrinsic::sqr, {ast::constant::zero}, zero));
        assert(is_reduceable(ast::intrinsic::sqr, {G(2)}, G(4)));
        assert(is_reduceable(ast::intrinsic::sqr, {G(10)}, G(100)));
        assert(is_reduceable(ast::intrinsic::sqr, {U{ast::unary::minus, a_}}, I{ast::intrinsic::sqr, {{a_}}}));
        assert(is_reduceable(ast::intrinsic::sqr, {I{ast::intrinsic::abs, {{a_}}}}, I{ast::intrinsic::sqr, {{a_}}}));
        assert(is_reduceable(ast::intrinsic::sqr, {I{ast::intrinsic::chs, {{a_}}}}, I{ast::intrinsic::sqr, {{a_}}}));
        assert(is_reduceable(ast::intrinsic::sqr, {I{ast::intrinsic::exp, {{a_}}}}, I{ast::intrinsic::exp, {{I{ast::intrinsic::twice, {{a_}}}}}}));
        assert(is_reduceable(ast::intrinsic::sqr, {I{ast::intrinsic::pow2, {{a_}}}}, I{ast::intrinsic::pow2, {{I{ast::intrinsic::twice, {{a_}}}}}}));
        assert(is_reduceable(ast::intrinsic::sqr, {invoke(ast::intrinsic::sqr, {B{ast::constant::one, ast::binary::add, ast::constant::one}})}, G(16)));

        assert(is_reduceable(ast::intrinsic::log, {G(2), G(4)}, G(2)));
        assert(is_reduceable(ast::intrinsic::log, {G(4), G(2)}, G(0.5)));
        assert(is_reduceable(ast::intrinsic::log, {G(3), G(27)}, G(3)));
        assert(is_reduceable(ast::intrinsic::log, {G(5), G(one)}, zero));
        assert(is_reduceable(ast::intrinsic::log, {G(0.5), G(2)}, G(-1)));

        assert(is_immutable(ast::intrinsic::ln, {a_}));
        assert(is_reduceable(ast::intrinsic::ln, {one}, zero));

        assert(is_immutable(ast::intrinsic::log2, {a_}));
        assert(is_reduceable(ast::intrinsic::log2, {one}, zero));
        assert(is_reduceable(ast::intrinsic::log2, {G(4)}, G(2)));
        assert(is_reduceable(ast::intrinsic::log2, {G(0.5)}, -one));
        assert(is_reduceable(ast::intrinsic::log2, {G(8)}, G(3)));

        assert(is_immutable(ast::intrinsic::lg, {a_}));
        assert(is_reduceable(ast::intrinsic::lg, {one}, zero));
        assert(is_reduceable(ast::intrinsic::lg, {G(10)}, one));
        assert(is_reduceable(ast::intrinsic::lg, {G(100)}, G(2)));
        assert(is_reduceable(ast::intrinsic::lg, {G(0.01)}, G(-2)));

        assert(is_reduceable(ast::intrinsic::yl2xp1, {G(0.0625), G(3)}, G(3) * log2(G(1.0625))));
        assert(is_reduceable(ast::intrinsic::yl2xp1, {G(0.25), G(5)}, G(5) * log2(G(1.25))));
        assert(is_reduceable(ast::intrinsic::yl2xp1, {G(0.125), G(4)}, G(4) * log2(G(1.125))));

        assert(is_reduceable(ast::intrinsic::scale2, {G(3), G(7)}, G(56)));
        assert(is_reduceable(ast::intrinsic::scale2, {zero, G(5)}, G(5)));

        assert(is_immutable(ast::intrinsic::pow2m1, {a_}));
        assert(is_reduceable(ast::intrinsic::pow2m1, {G(2)}, G(3)));
        assert(is_reduceable(ast::intrinsic::pow2m1, {ast::constant::l2t}, G(9)));

        assert(is_reduceable(ast::intrinsic::max, {G(2), G(-2), zero}, G(2)));
        assert(is_immutable(ast::intrinsic::max, {ast::constant::ln2, ast::constant::l2t, ast::constant::lg2}));
        assert(is_immutable(ast::intrinsic::max, {ast::constant::ln2, one}));
        assert(is_reduceable(ast::intrinsic::max, {one, ast::constant::ln2}, I{ast::intrinsic::max, {{ast::constant::ln2, one}}}));
        assert(is_reduceable(ast::intrinsic::min, {G(2), G(-2), zero}, G(-2)));
        assert(is_immutable(ast::intrinsic::min, {ast::constant::ln2, ast::constant::l2t, ast::constant::lg2}));
        assert(is_immutable(ast::intrinsic::min, {ast::constant::ln2, one}));
        assert(is_reduceable(ast::intrinsic::min, {one, ast::constant::ln2}, I{ast::intrinsic::min, {{ast::constant::ln2, one}}}));

        assert(is_immutable(ast::intrinsic::arccos, {ast::constant::ln2}));
        assert(is_reduceable(ast::intrinsic::arccos, {G(0.12345)}, acos(G(0.12345))));
        assert(is_reduceable(ast::intrinsic::arccos, {G(0.5)}, third_pi< G >()));
        assert(is_reduceable(ast::intrinsic::arccos, {one}, acos(one)));
        assert(is_reduceable(ast::intrinsic::arccos, {zero}, acos(zero)));
        assert(is_reduceable(ast::intrinsic::arccos, {-one}, acos(-one)));

        assert(is_immutable(ast::intrinsic::arcsin, {ast::constant::ln2}));
        assert(is_reduceable(ast::intrinsic::arcsin, {G(0.12345)}, asin(G(0.12345))));
        assert(is_reduceable(ast::intrinsic::arcsin, {G(0.5)}, sixth_pi< G >()));
        assert(is_reduceable(ast::intrinsic::arcsin, {one}, asin(one)));
        assert(is_reduceable(ast::intrinsic::arcsin, {zero}, asin(zero)));
        assert(is_reduceable(ast::intrinsic::arcsin, {-one}, asin(-one)));

        assert(is_immutable(ast::intrinsic::extract, {a_}));
        //assert(is_reduceable(ast::intrinsic::extract, {zero}, R{{-std::numeric_limits< G >::infinity(), -std::numeric_limits< G >::quiet_NaN()}})); // NaN != NaN :(
        for (size_type i = 0; i < 100; ++i) {
            G value_ = signed_value();
            G exponent_ = logb(abs(value_));
            G significand_ = value_ / exp2(exponent_);
            if (!is_reduceable(ast::intrinsic::extract, {value_}, R{append< ast::rvalues >(exponent_, significand_)})) {
                std::cerr << value_ << std::endl;
                assert(false);
            }
        }

        assert(is_immutable(ast::intrinsic::fracint, {a_}));
        assert(is_reduceable(ast::intrinsic::fracint, {zero}, R{{zero, zero}}));
        assert(is_reduceable(ast::intrinsic::fracint, {ast::constant::one}, R{{zero, one}}));
        assert(is_reduceable(ast::intrinsic::fracint, {ast::constant::zero}, R{{zero, zero}}));
        for (size_type i = 0; i < 100; ++i) {
            G value_ = signed_value();
            G integral_ = trunc(value_);
            G fractional_ = value_ - integral_;
            if (!is_reduceable(ast::intrinsic::fracint, {value_}, R{append< ast::rvalues >(fractional_, integral_)})) {
                std::cerr << value_ << std::endl;
                assert(false);
            }
        }

        assert(is_immutable(ast::intrinsic::sincos, {ast::constant::ln2}));
        for (size_type i = 0; i < 100; ++i) {
            G angle_ = G(4) * pi< G >() * (zero_to_one_(random_) * G(2) - one);
            if (!is_reduceable(ast::intrinsic::sincos, {angle_}, R{append< ast::rvalues >(sin(angle_), cos(angle_))})) {
                std::cerr << angle_ << std::endl;
                assert(false);
            }
        }

        assert(is_immutable(ast::intrinsic::arctg, {ast::constant::ln2}));
        assert(is_reduceable(ast::intrinsic::arctg, {one}, pi< G >() / G(4)));
        assert(is_reduceable(ast::intrinsic::arctg, {zero}, zero));
        for (size_type i = 0; i < 100; ++i) {
            G tg_ = signed_value();
            if (!is_reduceable(ast::intrinsic::arctg, {tg_}, atan(tg_))) {
                std::cerr << tg_ << std::endl;
                assert(false);
            }
        }

        assert(is_immutable(ast::intrinsic::exp, {a_}));
        assert(is_reduceable(ast::intrinsic::exp, {one}, e< G >()));
        assert(is_reduceable(ast::intrinsic::exp, {G(0.5)}, root_e< G >()));
        assert(is_reduceable(ast::intrinsic::exp, {G(-0.5)}, exp_minus_half< G >()));
        assert(is_reduceable(ast::intrinsic::exp, {pi< G >()}, e_pow_pi< G >()));
    }

    void
    test_expression()
    {
        G const o1(1);
        G const o2(2);
        G const o3(3);
        G const o4(4);
        G const o5(5);
        G const o6(6);

        auto const e00 = E{o1, {{ast::binary::add, o2}, {ast::binary::add, o3}, {ast::binary::add, o5}, {ast::binary::mul, o4}, {ast::binary::mul, o5}, {ast::binary::add, o6}}};
        assert(is_reduceable(e00, G(112.0)));

        auto const e10 = E{o1, {{ast::binary::add, o2}, {ast::binary::sub, o3}, {ast::binary::add, o5}, {ast::binary::mul, o4}, {ast::binary::div, o5}, {ast::binary::mul, o6}}};
        assert(is_reduceable(e10, G(24)));

        // Return value: Remainder of dividing arguments. The result has the same sign as x.
        assert(is_reduceable(E{G(5), {{ast::binary::mod, G(3)}}}, G(2)));
        assert(is_reduceable(E{G(-5), {{ast::binary::mod, G(3)}}}, G(-2)));
        assert(is_reduceable(E{G(5), {{ast::binary::mod, G(-3)}}}, G(2)));
        assert(is_reduceable(E{G(-5), {{ast::binary::mod, G(-3)}}}, G(-2)));

        // The two instructions differ in the way the notional round-to-integer operation is performed.
        // FPREM does it by rounding towards zero, so that the remainder it returns always has the same sign as the original value in ST0;
        // FPREM1 does it by rounding to the nearest integer, so that the remainder always has at most half the magnitude of ST1.
        assert(is_reduceable(B{G(11), ast::binary::mod, G(7)}, G(4)));
        assert(is_reduceable(B{G(10), ast::binary::mod, G(7)}, G(3)));

        assert(is_reduceable(E{G(3), {{ast::binary::sub, G(2)}, {ast::binary::sub, G(1)}}}, zero));
    }

    void
    test_unary_expression_branches()
    {
        assert(is_reduceable(U{ast::unary::plus, ast::constant::l2e}, ast::constant::l2e));
        assert(is_reduceable(U{ast::unary::minus, U{ast::unary::minus, ast::constant::l2t}}, ast::constant::l2t));
        assert(is_reduceable(U{ast::unary::minus, G(2)}, G(-2)));
        assert(is_reduceable(U{ast::unary::plus, G(2)}, G(2)));
        assert(is_reduceable(U{ast::unary::minus, ast::constant::one}, -one));
    }

    void
    test_expression_branches()
    {
        U const lhs_{ast::unary::minus, ast::constant::lg2};
        U const rhs_{ast::unary::minus, ast::constant::pi};

        assert(is_reduceable(B{ast::constant::ln2, ast::binary::add, rhs_}, B{ast::constant::ln2, ast::binary::sub, ast::constant::pi}));
        assert(is_reduceable(B{ast::constant::ln2, ast::binary::sub, rhs_}, B{ast::constant::ln2, ast::binary::add, ast::constant::pi}));
        assert(is_reduceable(B{ast::constant::ln2, ast::binary::mul, rhs_}, U{ast::unary::minus, B{ast::constant::ln2, ast::binary::mul, ast::constant::pi}}));
        assert(is_reduceable(B{ast::constant::ln2, ast::binary::div, rhs_}, U{ast::unary::minus, B{ast::constant::ln2, ast::binary::div, ast::constant::pi}}));
        assert(is_reduceable(B{ast::constant::l2t, ast::binary::mod, U{ast::unary::minus, G(-2)}}, B{ast::constant::l2t, ast::binary::mod, G(2)}));
        assert(is_immutable(B{ast::constant::ln2, ast::binary::pow, lhs_}));

        assert(is_reduceable(B{lhs_, ast::binary::add, ast::constant::ln2}, B{ast::constant::ln2, ast::binary::sub, ast::constant::lg2}));
        assert(is_reduceable(B{lhs_, ast::binary::sub, ast::constant::l2e}, U{ast::unary::minus, B{ast::constant::lg2, ast::binary::add, ast::constant::l2e}}));\
        assert(is_reduceable(B{lhs_, ast::binary::mul, ast::constant::l2e}, U{ast::unary::minus, B{ast::constant::lg2, ast::binary::mul, ast::constant::l2e}}));\
        assert(is_reduceable(B{lhs_, ast::binary::div, ast::constant::l2e}, U{ast::unary::minus, B{ast::constant::lg2, ast::binary::div, ast::constant::l2e}}));\
        assert(is_reduceable(B{lhs_, ast::binary::mod, ast::constant::l2e}, U{ast::unary::minus, B{ast::constant::lg2, ast::binary::mod, ast::constant::l2e}}));\
        assert(is_immutable(B{ast::constant::pi, ast::binary::pow, ast::constant::ln2}));

        assert(is_reduceable(B{lhs_, ast::binary::add, rhs_}, U{ast::unary::minus, B{ast::constant::lg2, ast::binary::add, ast::constant::pi}}));
        assert(is_reduceable(B{lhs_, ast::binary::sub, rhs_}, B{ast::constant::pi, ast::binary::sub, ast::constant::lg2}));
        assert(is_reduceable(B{lhs_, ast::binary::mul, rhs_}, B{ast::constant::lg2, ast::binary::mul, ast::constant::pi}));
        assert(is_reduceable(B{lhs_, ast::binary::div, rhs_}, B{ast::constant::lg2, ast::binary::div, ast::constant::pi}));
        assert(is_reduceable(B{lhs_, ast::binary::mod, rhs_}, U{ast::unary::minus, B{ast::constant::lg2, ast::binary::mod, ast::constant::pi}}));\
        assert(is_immutable(B{lhs_, ast::binary::pow, rhs_}));

        assert(is_reduceable(B{ast::constant::pi, ast::binary::add, zero}, ast::constant::pi));
        assert(is_reduceable(B{ast::constant::pi, ast::binary::sub, zero}, ast::constant::pi));
        assert(is_reduceable(B{ast::constant::pi, ast::binary::mul, zero}, zero));
        assert(is_reduceable(B{ast::constant::pi, ast::binary::mul, one}, ast::constant::pi));
        assert(is_reduceable(B{ast::constant::pi, ast::binary::mul, -one}, U{ast::unary::minus, ast::constant::pi}));
        assert(is_reduceable(B{ast::constant::pi, ast::binary::div, -one}, U{ast::unary::minus, ast::constant::pi}));
        assert(is_reduceable(B{ast::constant::pi, ast::binary::div, one}, ast::constant::pi));
        assert(is_reduceable(B{ast::constant::pi, ast::binary::pow, zero}, one));
        assert(is_reduceable(B{ast::constant::pi, ast::binary::pow, one}, ast::constant::pi));

        assert(is_reduceable(B{zero, ast::binary::add, ast::constant::lg2}, ast::constant::lg2));
        assert(is_reduceable(B{zero, ast::binary::sub, ast::constant::lg2}, U{ast::unary::minus, ast::constant::lg2}));
        assert(is_reduceable(B{zero, ast::binary::mul, ast::constant::pi}, zero));
        assert(is_reduceable(B{one, ast::binary::mul, ast::constant::pi}, ast::constant::pi));
        assert(is_reduceable(B{-one, ast::binary::mul, ast::constant::pi}, U{ast::unary::minus, ast::constant::pi}));
        assert(is_reduceable(B{zero, ast::binary::div, ast::constant::pi}, zero));
        assert(is_reduceable(B{zero, ast::binary::mod, ast::constant::pi}, zero));
        assert(is_reduceable(B{one, ast::binary::pow, ast::constant::pi}, one));

        assert(is_reduceable(B{U{ast::unary::minus, ast::constant::pi}, ast::binary::add, zero}, U{ast::unary::minus, ast::constant::pi}));
        assert(is_reduceable(B{U{ast::unary::minus, ast::constant::pi}, ast::binary::add, one}, B{one, ast::binary::sub, ast::constant::pi}));
        assert(is_reduceable(B{U{ast::unary::minus, ast::constant::pi}, ast::binary::sub, zero}, U{ast::unary::minus, ast::constant::pi}));
        assert(is_reduceable(B{U{ast::unary::minus, ast::constant::pi}, ast::binary::sub, one}, U{ast::unary::minus, B{ast::constant::pi, ast::binary::add, one}}));
        assert(is_reduceable(B{U{ast::unary::minus, ast::constant::pi}, ast::binary::mul, zero}, zero));
        assert(is_reduceable(B{U{ast::unary::minus, ast::constant::pi}, ast::binary::mul, one}, U{ast::unary::minus, ast::constant::pi}));
        assert(is_reduceable(B{U{ast::unary::minus, ast::constant::pi}, ast::binary::mul, -one}, ast::constant::pi));
        assert(is_reduceable(B{U{ast::unary::minus, ast::constant::pi}, ast::binary::mul, ast::constant::l2e}, U{ast::unary::minus, B{ast::constant::pi, ast::binary::mul, ast::constant::l2e}}));
        assert(is_reduceable(B{U{ast::unary::minus, ast::constant::pi}, ast::binary::div, one}, U{ast::unary::minus, ast::constant::pi}));
        assert(is_reduceable(B{U{ast::unary::minus, ast::constant::pi}, ast::binary::div, -one}, ast::constant::pi));
        assert(is_reduceable(B{U{ast::unary::minus, ast::constant::pi}, ast::binary::div, ast::constant::l2e}, U{ast::unary::minus, B{ast::constant::pi, ast::binary::div, ast::constant::l2e}}));
        assert(is_reduceable(B{U{ast::unary::minus, ast::constant::pi}, ast::binary::mod, ast::constant::l2e}, U{ast::unary::minus, B{ast::constant::pi, ast::binary::mod, ast::constant::l2e}}));
        assert(is_reduceable(B{U{ast::unary::minus, ast::constant::pi}, ast::binary::pow, zero}, one));
        assert(is_reduceable(B{U{ast::unary::minus, ast::constant::pi}, ast::binary::pow, one}, U{ast::unary::minus, ast::constant::pi}));

        assert(is_reduceable(B{zero, ast::binary::add, rhs_}, rhs_));
        assert(is_reduceable(B{zero, ast::binary::sub, U{ast::unary::minus, ast::constant::pi}}, ast::constant::pi));
        assert(is_reduceable(B{ast::constant::lg2, ast::binary::sub, U{ast::unary::minus, ast::constant::pi}}, B{ast::constant::lg2, ast::binary::add, ast::constant::pi}));
        assert(is_reduceable(B{zero, ast::binary::mul, U{ast::unary::minus, ast::constant::pi}}, zero));
        assert(is_reduceable(B{one, ast::binary::mul, lhs_}, lhs_));
        assert(is_reduceable(B{-one, ast::binary::mul, U{ast::unary::minus, ast::constant::pi}}, ast::constant::pi));
        assert(is_reduceable(B{ast::constant::l2t, ast::binary::mul, U{ast::unary::minus, ast::constant::pi}}, U{ast::unary::minus, B{ast::constant::l2t, ast::binary::mul, ast::constant::pi}}));
        assert(is_reduceable(B{zero, ast::binary::div, U{ast::unary::minus, ast::constant::pi}}, zero));
        assert(is_reduceable(B{ast::constant::l2t, ast::binary::div, U{ast::unary::minus, ast::constant::pi}}, U{ast::unary::minus, B{ast::constant::l2t, ast::binary::div, ast::constant::pi}}));
        assert(is_reduceable(B{zero, ast::binary::mod, U{ast::unary::minus, ast::constant::pi}}, zero));
        assert(is_reduceable(B{ast::constant::l2t, ast::binary::mod, U{ast::unary::minus, ast::constant::pi}}, B{ast::constant::l2t, ast::binary::mod, ast::constant::pi}));
        assert(is_reduceable(B{one, ast::binary::pow, U{ast::unary::minus, ast::constant::pi}}, one));

        assert(is_reduceable(B{G(2), ast::binary::add, G(3)}, G(5)));
        assert(is_reduceable(B{G(2), ast::binary::sub, G(3)}, G(-1)));
        assert(is_reduceable(B{G(2), ast::binary::mul, G(3)}, G(6)));
        assert(is_reduceable(B{G(6), ast::binary::div, G(3)}, G(2)));
        assert(is_reduceable(B{G(3), ast::binary::mod, G(2)}, one));
        assert(is_reduceable(B{G(3), ast::binary::pow, G(2)}, G(9)));
    }

public:

    test()
        : zero_to_one_()
        , uniform_int_(std::numeric_limits< G >::min_exponent - 1, std::numeric_limits< G >::max_exponent - 1)
    {
        set_seed();
    }

    bool
    operator () ()
    try {
        test_floating_point();
        test_constant();
        test_identifier();
        test_unary_expression();
        test_intrinsic();
        test_expression();
        test_unary_expression_branches();
        test_expression_branches();
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


