#pragma once

#include <utility>
#include <iterator>

namespace insituc
{

template< typename container >
struct reversed_container
{

    container container_;

    constexpr
    auto
    begin()
    {
        return std::rbegin(container_);
    }

    constexpr
    auto
    end()
    {
        return std::rend(container_);
    }

};

template< typename container >
constexpr
reversed_container< container >
reverse(container && _container) noexcept(noexcept(container{std::declval< container >()}))
{
    return {std::forward< container >(_container)};
}

}
