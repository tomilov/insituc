#pragma once

#include <type_traits>
#include <ostream>
#include <limits>

#include <cmath>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wfloat-equal"
namespace floating_point_type
{

template< typename F >
class G;

template< typename F >
std::ostream &
operator << (std::ostream & _out, G< F > const g);

template< typename F >
std::istream &
operator >> (std::istream & _out, G< F > & g);

template< typename F >
bool
signbit(G< F > const g);

template< typename F >
int
fpclassify(G< F > const g);

template< typename F >
bool
isnan(G< F > const g);

template< typename F >
bool
isinf(G< F > const g);

template< typename F >
bool
isunordered(G< F > const l, G< F > const r);

template< typename F >
bool
isgreater(G< F > const l, G< F > const r);

template< typename F >
bool
isless(G< F > const l, G< F > const r);

template< typename F >
bool
islessgreater(G< F > const l, G< F > const r);

template< typename F >
G< F >
abs(G< F > const g);

template< typename F >
G< F >
pow(G< F > const b, G< F > const e);

template< typename F >
G< F >
exp(G< F > const g);

template< typename F >
G< F >
exp2(G< F > const g);

template< typename F >
G< F >
log(G< F > const g);

template< typename F >
G< F >
log2(G< F > const g);

template< typename F >
G< F >
log10(G< F > const g);

template< typename F >
G< F >
log1p(G< F > const g);

template< typename F >
G< F >
logb(G< F > const g);

template< typename F >
G< F >
nearbyint(G< F > const g);

template< typename F >
G< F >
trunc(G< F > const g);

template< typename F >
G< F >
sqrt(G< F > const g);

template< typename F >
G< F >
sin(G< F > const g);

template< typename F >
G< F >
cos(G< F > const g);

template< typename F >
G< F >
tan(G< F > const g);

template< typename F >
G< F >
atan(G< F > const g);

template< typename F >
G< F >
atan2(G< F > const x, G< F > const y);

template< typename F >
G< F >
asin(G< F > const g);

template< typename F >
G< F >
acos(G< F > const g);

template< typename F >
G< F >
scalbn(G< F > const g, int e);

template< typename F >
G< F >
fmod(G< F > const x, G< F > const y);

template< typename F >
G< F >
remainder(G< F > const x, G< F > const y);

template< typename F >
class G final
{

    static_assert(std::is_floating_point_v< F >, "F must be fundamental floating point type");

    F f;

    static constexpr F indefinite_ = std::numeric_limits< F >::quiet_NaN();

public :

    template< typename I, typename = std::enable_if_t< (std::is_integral_v< I >) > >
    constexpr
    explicit
    G(I const i) : f(static_cast< F >(i)) { ; }

    constexpr
    G(F const & rhs) : f(rhs) { ; }
    constexpr
    G(F & rhs) : f(rhs) { ; }
    constexpr
    G(F const && rhs) : G(rhs) { ; }
    constexpr
    G(F && rhs) : G(rhs) { rhs = indefinite_; }

    constexpr
    G() : f(indefinite_) { ; }

    constexpr
    G(G const &) = default;
    constexpr
    G(G && rhs) : f(std::move(rhs.f)) { rhs.f = indefinite_; }

    constexpr
    G & operator = (G const &) = default;
    constexpr
    G & operator = (G && rhs) { f = std::move(rhs.f); rhs.f = indefinite_; return *this; }

    constexpr
    G operator - () const & { return static_cast< G >(-f); }
    constexpr
    G operator - () && { f = -f; return std::move(*this); }

    constexpr
    G const & operator + () const & { return *this; }
    constexpr
    G operator + () && { return std::move(*this); }

    constexpr
    G & operator += (G const g) & { f += g.f; return *this; }
    constexpr
    G & operator -= (G const g) & { f -= g.f; return *this; }
    constexpr
    G & operator *= (G const g) & { f *= g.f; return *this; }
    constexpr
    G & operator /= (G const g) & { f /= g.f; return *this; }

    constexpr
    bool operator == (G const g) const { return (f == g.f); }
    constexpr
    bool operator != (G const g) const { return !operator == (g); }

    constexpr
    bool operator < (G const g) const { return (f < g.f); }

    constexpr
    explicit
    operator F const & () const &
    {
        return f;
    }

    constexpr
    explicit
    operator F () &&
    {
        F const f_ = f;
        f = indefinite_;
        return f_;
    }

    friend
    std::ostream &
    operator << (std::ostream & _out, G const g)
    {
        return _out << g.f;
    }

    friend
    std::istream &
    operator >> (std::istream & _out, G & g)
    {
        return _out >> g.f;
    }

    friend
    bool
    signbit(G const g)
    {
        using std::signbit;
        return signbit(g.f);
    }

    friend
    int
    fpclassify(G const g)
    {
        using std::fpclassify;
        return fpclassify(g.f);
    }

    friend
    bool
    isnan(G const g)
    {
        using std::isnan;
        return isnan(g.f);
    }

    friend
    bool
    isinf(G const g)
    {
        using std::isinf;
        return isinf(g.f);
    }

    friend
    bool
    isunordered(G const l, G const r)
    {
        using std::isunordered;
        return isunordered(l.f, r.f);
    }

    friend
    bool
    isgreater(G const l, G const r)
    {
        using std::isgreater;
        return isgreater(l.f, r.f);
    }

    friend
    bool
    isless(G const l, G const r)
    {
        using std::isless;
        return isless(l.f, r.f);
    }

    friend
    bool
    islessgreater(G const l, G const r)
    {
        using std::islessgreater;
        return islessgreater(l.f, r.f);
    }

    friend
    G
    abs(G const g)
    {
        using std::abs;
        return abs(g.f);
    }

    friend
    G
    pow(G const b, G const e)
    {
        using std::pow;
        return pow(b.f, e.f);
    }

    friend
    G
    exp(G const g)
    {
        using std::exp;
        return exp(g.f);
    }

    friend
    G
    exp2(G const g)
    {
        using std::exp2;
        return exp2(g.f);
    }

    friend
    G
    log(G const g)
    {
        using std::log;
        return log(g.f);
    }

    friend
    G
    log2(G const g)
    {
        using std::log2;
        return log2(g.f);
    }

    friend
    G
    log10(G const g)
    {
        using std::log10;
        return log10(g.f);
    }

    friend
    G
    log1p(G const g)
    {
        using std::log1p;
        return log1p(g.f);
    }

    friend
    G
    logb(G const g)
    {
        using std::logb;
        return logb(g.f);
    }

    friend
    G
    nearbyint(G const g)
    {
        using std::nearbyint;
        return nearbyint(g.f);
    }

    friend
    G
    trunc(G const g)
    {
        using std::trunc;
        return trunc(g.f);
    }

    friend
    G
    sqrt(G const g)
    {
        using std::sqrt;
        return sqrt(g.f);
    }

    friend
    G
    sin(G const g)
    {
        using std::sin;
        return sin(g.f);
    }

    friend
    G
    cos(G const g)
    {
        using std::cos;
        return cos(g.f);
    }

    friend
    G
    tan(G const g)
    {
        using std::tan;
        return tan(g.f);
    }

    friend
    G
    atan(G const g)
    {
        using std::atan;
        return atan(g.f);
    }

    friend
    G
    atan2(G const x, G const y)
    {
        using std::atan2;
        return atan2(x.f, y.f);
    }

    friend
    G
    asin(G const g)
    {
        using std::asin;
        return asin(g.f);
    }

    friend
    G
    acos(G const g)
    {
        using std::acos;
        return acos(g.f);
    }

    friend
    G
    scalbn(G const g, int e)
    {
        using std::scalbn;
        return scalbn(g.f, e);
    }

    friend
    G
    fmod(G const x, G const y)
    {
        using std::fmod;
        return fmod(x.f, y.f);
    }

    friend
    G
    remainder(G const x, G const y)
    {
        using std::remainder;
        return remainder(x.f, y.f);
    }

};

template< typename F >
constexpr
G< F >
operator + (G< F > l, G< F > const r)
{
    l += r;
    return std::move(l);
}

template< typename F >
constexpr
G< F >
operator - (G< F > l, G< F > const r)
{
    l -= r;
    return std::move(l);
}

template< typename F >
constexpr
G< F >
operator * (G< F > l, G< F > const r)
{
    l *= r;
    return std::move(l);
}

template< typename F >
constexpr
G< F >
operator / (G< F > l, G< F > const r)
{
    l /= r;
    return std::move(l);
}

}
#pragma clang diagnostic pop

namespace std
{

template< typename F >
struct numeric_limits< ::floating_point_type::G< F > >
        : std::numeric_limits< F >
{

    using G = ::floating_point_type::G< F >;

    static constexpr G min()           noexcept { return std::numeric_limits< F >::min(); }
    static constexpr G lowest()        noexcept { return std::numeric_limits< F >::lowest(); }
    static constexpr G max()           noexcept { return std::numeric_limits< F >::max(); }
    static constexpr G epsilon()       noexcept { return std::numeric_limits< F >::epsilon(); }
    static constexpr G round_error()   noexcept { return std::numeric_limits< F >::round_error(); }
    static constexpr G infinity()      noexcept { return std::numeric_limits< F >::infinity(); }
    static constexpr G quiet_NaN()     noexcept { return std::numeric_limits< F >::quiet_NaN(); }
    static constexpr G signaling_NaN() noexcept { return std::numeric_limits< F >::signaling_NaN(); }
    static constexpr G denorm_min()    noexcept { return std::numeric_limits< F >::denorm_min(); }

};

} // namespace std
