#pragma once

#include <utility>
#include <iterator>

namespace insituc
{

template< typename container >
struct tail_container
{

    container container_;

    constexpr
    auto
    begin()
    {
        auto first = std::begin(container_);
        if (first == std::end(container_)) {
            return first;
        } else {
            return std::next(first);
        }
    }

    constexpr
    auto
    end()
    {
        return std::end(container_);
    }

};

template< typename container >
constexpr
tail_container< container >
tail(container && _container) noexcept(noexcept(container{std::declval< container >()}))
{
    return {std::forward< container >(_container)};
}

}
