#pragma once

#include <insituc/meta/base_types.hpp>

namespace insituc
{
namespace meta
{

enum class mnemocode
{
    ud2,
    fwait,
    finit,
    fninit,
    fstsw,
    fnstsw,
    fstcw,
    fnstcw,
    fldcw,
    fclex,
    fnclex,
    fsave,
    fnsave,
    frstor,
    fstenv,
    fnstenv,
    fldenv,
    ffree,
    ffreep, // undocumented
    fdecstp,
    fincstp,
    fst,
    fstp,
    fld,
    fxch,
    fcmovb,
    fcmove,
    fcmovbe,
    fcmovu,
    fcmovnb,
    fcmovne,
    fcmovnbe,
    fcmovnu,
    fldz,
    fld1,
    fldpi,
    fldl2e,
    fldl2t,
    fldlg2,
    fldln2,
    fild,
    fist,
    fistp,
    fisttp,
    fbld,
    fbstp,
    fcom,
    fcomp,
    fcompp,
    fcomi,
    fcomip,
    fucom,
    fucomp,
    fucompp,
    fucomi,
    fucomip,
    ftst,
    fxam,
    ficom,
    ficomp,
    fabs,
    fadd,
    faddp,
    fchs,
    fdiv,
    fdivp,
    fdivr,
    fdivrp,
    fmul,
    fmulp,
    frndint,
    fsqrt,
    fsub,
    fsubp,
    fsubr,
    fsubrp,
    fiadd,
    fidiv,
    fidivr,
    fimul,
    fisub,
    fisubr,
    fcos,
    fpatan,
    fptan,
    fsin,
    fsincos,
    f2xm1,
    fscale,
    fyl2x,
    fyl2xp1,
    fnop,
    fprem, // TODO: frac() using of this
    fprem1, // The two instructions differ in the way the notional round-to-integer operation is performed. FPREM does it by rounding towards zero, so that the remainder it returns always has the same sign as the original value in ST0; FPREM1 does it by rounding to the nearest integer, so that the remainder always has at most half the magnitude of ST1.
    fxtract,

    trunc, // trunc = fisttp sp - sizeof(std::int64_t); fild sp - sizeof(std::int64_t)

    sp_inc,
    sp_dec,
    bra,
    ket,
    endl,
    alloca_,
    call,
    ret,
    sahf
};

#ifdef __linux__
#define COLOR(foreground, str) __extension__ "\e[1;3" #foreground "m" str "\e[0m"
#else
#define COLOR(foreground, str) str
#endif

constexpr
char_type const *
c_str(mnemocode const _mnemocode) noexcept
{
    switch (_mnemocode) {
    case mnemocode::ud2      : return "ud2";
    case mnemocode::fwait    : return "fwait";
    case mnemocode::finit    : return "finit";
    case mnemocode::fninit   : return "fninit";
    case mnemocode::fstsw    : return "fstsw";
    case mnemocode::fnstsw   : return "fnstsw";
    case mnemocode::fstcw    : return "fstcw";
    case mnemocode::fnstcw   : return "fnstcw";
    case mnemocode::fldcw    : return "fldcw";
    case mnemocode::fclex    : return "fclex";
    case mnemocode::fnclex   : return "fnclex";
    case mnemocode::fsave    : return "fsave";
    case mnemocode::fnsave   : return "fnsave";
    case mnemocode::frstor   : return "frstor";
    case mnemocode::fstenv   : return "fstenv";
    case mnemocode::fnstenv  : return "fnstenv";
    case mnemocode::fldenv   : return "fldenv";
    case mnemocode::ffree    : return "ffree";
    case mnemocode::ffreep   : return "ffreep";
    case mnemocode::fdecstp  : return "fdecstp";
    case mnemocode::fincstp  : return "fincstp";
    case mnemocode::fld      : return "fld";
    case mnemocode::fst      : return "fst";
    case mnemocode::fstp     : return "fstp";
    case mnemocode::fxch     : return "fxch";
    case mnemocode::fcmovb   : return "fcmovb";
    case mnemocode::fcmove   : return "fcmove";
    case mnemocode::fcmovbe  : return "fcmovbe";
    case mnemocode::fcmovu   : return "fcmovu";
    case mnemocode::fcmovnb  : return "fcmovnb";
    case mnemocode::fcmovne  : return "fcmovne";
    case mnemocode::fcmovnbe : return "fcmovnbe";
    case mnemocode::fcmovnu  : return "fcmovnu";
    case mnemocode::fldz     : return "fldz";
    case mnemocode::fld1     : return "fld1";
    case mnemocode::fldpi    : return "fldpi";
    case mnemocode::fldl2e   : return "fldl2e";
    case mnemocode::fldl2t   : return "fldl2t";
    case mnemocode::fldlg2   : return "fldlg2";
    case mnemocode::fldln2   : return "fldln2";
    case mnemocode::fild     : return "fild";
    case mnemocode::fist     : return "fist";
    case mnemocode::fistp    : return "fistp";
    case mnemocode::fisttp   : return "fisttp";
    case mnemocode::fbld     : return "fbld";
    case mnemocode::fbstp    : return "fbstp";
    case mnemocode::fcom     : return "fcom";
    case mnemocode::fcomi    : return "fcomi";
    case mnemocode::fcomip   : return "fcomip";
    case mnemocode::fcomp    : return "fcomp";
    case mnemocode::fcompp   : return "fcompp";
    case mnemocode::ficom    : return "ficom";
    case mnemocode::ficomp   : return "ficomp";
    case mnemocode::ftst     : return "ftst";
    case mnemocode::fucom    : return "fucom";
    case mnemocode::fucomi   : return "fucomi";
    case mnemocode::fucomip  : return "fucomip";
    case mnemocode::fucomp   : return "fucomp";
    case mnemocode::fucompp  : return "fucompp";
    case mnemocode::fxam     : return "fxam";
    case mnemocode::fabs     : return "fabs";
    case mnemocode::fadd     : return "fadd";
    case mnemocode::faddp    : return "faddp";
    case mnemocode::fchs     : return "fchs";
    case mnemocode::fdiv     : return "fdiv";
    case mnemocode::fdivp    : return "fdivp";
    case mnemocode::fdivr    : return "fdivr";
    case mnemocode::fdivrp   : return "fdivrp";
    case mnemocode::fmul     : return "fmul";
    case mnemocode::fmulp    : return "fmulp";
    case mnemocode::frndint  : return "frndint";
    case mnemocode::fsqrt    : return "fsqrt";
    case mnemocode::fsub     : return "fsub";
    case mnemocode::fsubp    : return "fsubp";
    case mnemocode::fsubr    : return "fsubr";
    case mnemocode::fsubrp   : return "fsubrp";
    case mnemocode::fiadd    : return "fiadd";
    case mnemocode::fidiv    : return "fidiv";
    case mnemocode::fidivr   : return "fidivr";
    case mnemocode::fimul    : return "fimul";
    case mnemocode::fisub    : return "fisub";
    case mnemocode::fisubr   : return "fisubr";
    case mnemocode::fcos     : return "fcos";
    case mnemocode::fpatan   : return "fpatan";
    case mnemocode::fptan    : return "fptan";
    case mnemocode::fsin     : return "fsin";
    case mnemocode::fsincos  : return "fsincos";
    case mnemocode::f2xm1    : return "f2xm1";
    case mnemocode::fscale   : return "fscale";
    case mnemocode::fyl2x    : return "fyl2x";
    case mnemocode::fyl2xp1  : return "fyl2xp1";
    case mnemocode::fnop     : return "fnop";
    case mnemocode::fprem    : return "fprem";
    case mnemocode::fprem1   : return "fprem1";
    case mnemocode::fxtract  : return "fxtract";
    case mnemocode::trunc    : return "trunc";
    case mnemocode::sp_inc   : return COLOR(3, "sp_inc");
    case mnemocode::sp_dec   : return COLOR(3, "sp_dec");
    case mnemocode::bra      : return COLOR(2, "bra"   );
    case mnemocode::ket      : return COLOR(2, "ket"   );
    case mnemocode::endl     : return COLOR(4, "endl"  );
    case mnemocode::alloca_  : return COLOR(7, "alloca");
    case mnemocode::call     : return COLOR(1, "call"  );
    case mnemocode::ret      : return COLOR(1, "ret"   );
    case mnemocode::sahf     : return "sahf";
    }
}

#undef COLOR

}
}
