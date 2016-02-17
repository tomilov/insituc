#pragma once

#include <utility>
#include <iterator>

namespace insituc
{

template< typename container >
struct head_container
{

    container container_;

    constexpr
    auto
    begin()
    {
        return std::begin(container_);
    }

    constexpr
    auto
    end()
    {
        auto last = std::end(container_);
        if (last == std::begin(container_)) {
            return last;
        } else {
            return std::prev(last);
        }
    }

};

template< typename container >
constexpr
head_container< container >
head(container && _container) noexcept(noexcept(container{std::declval< container >()}))
{
    return {std::forward< container >(_container)};
}

}
