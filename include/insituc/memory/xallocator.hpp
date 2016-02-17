#pragma once
// http://blogs.msdn.com/b/vcblog/archive/2008/08/28/the-mallocator.aspx
// http://eli.thegreenplace.net/2013/11/05/how-to-jit-an-introduction/
// http://sourceforge.net/p/predef/wiki/Architectures/

#include <memory>
#include <limits>
#include <new>
#include <utility>
#include <stdexcept>

#include <cstdint>
#include <cstddef>
#include <cassert>

namespace insituc
{

template< typename type >
struct xallocator
{

    using value_type = type;

    xallocator() = default;

    template< typename rhs >
    constexpr
    xallocator(xallocator< rhs > const &) noexcept
    { ; }

#ifdef PAGESIZE
    [[gnu::assume_aligned(PAGESIZE)]] // -DPAGESIZE=`getconf PAGESIZE` for Linux xor -DPAGESIZE="alignof(std::max_align_t)" for Windows
#endif
    value_type *
    allocate(std::size_t n) const;

    void
    deallocate(value_type * p, std::size_t n) const;

};

template< typename lhs, typename rhs >
constexpr
bool
operator == (xallocator< lhs > const & /*_lhs*/, xallocator< rhs > const & /*_rhs*/) noexcept
{
    return true;
}

template< typename lhs, typename rhs >
constexpr
bool
operator != (xallocator< lhs > const & _lhs, xallocator< rhs > const & _rhs) noexcept
{
    return !(_lhs == _rhs);
}

}

#if defined(_WIN32) || defined(_WIN64)

#include <windows.h>

namespace insituc
{

template< typename type >
auto
xallocator< type >::allocate(std::size_t n) const
-> value_type *
{
    if (n == 0) {
        return nullptr;
    }
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"
    ::LPVOID const p = ::VirtualAlloc(NULL, n * sizeof(value_type), (MEM_RESERVE | MEM_COMMIT), PAGE_EXECUTE_READWRITE);
    if (p == NULL) {
        throw std::bad_alloc{};
    }
#pragma clang diagnostic pop
#ifdef PAGESIZE
    assert(0 == (reinterpret_cast< std::uintptr_t >(p) % PAGESIZE));
#endif
    return static_cast< value_type * >(p);
}

template< typename type >
void
xallocator< type >::deallocate(value_type * p, std::size_t n) const
{
    if (p == nullptr) {
        return;
    }
    if (::VirtualFree(static_cast< ::LPVOID >(p), 0, MEM_RELEASE) == FALSE) {
        throw std::bad_alloc{};
    }
}

}

#elif defined(__linux__)

#include <sys/mman.h>

namespace insituc
{

template< typename type >
auto
xallocator< type >::allocate(std::size_t n) const
-> value_type *
{
    if (n == 0) {
        return nullptr;
    }
    void * p = ::mmap(nullptr, n * sizeof(value_type), (PROT_READ | PROT_WRITE | PROT_EXEC), (MAP_PRIVATE | MAP_ANONYMOUS), -1, 0); // insecure
    if (p == MAP_FAILED) {
        throw std::bad_alloc{};
    }
#ifdef PAGESIZE
    assert(0 == (reinterpret_cast< std::uintptr_t >(p) % PAGESIZE));
#endif
    return static_cast< value_type * >(p);
}

template< typename type >
void
xallocator< type >::deallocate(value_type * p, std::size_t n) const
{
    if (p == nullptr) {
        return;
    }
    if (::munmap(static_cast< void * >(p), n * sizeof(value_type)) == -1) {
        throw std::bad_alloc{};
    }
}

}

#endif
