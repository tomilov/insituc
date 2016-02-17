#pragma once

#include <utility>

namespace insituc
{

template< typename container, typename ...arguments >
container
append(arguments &&... _arguments) // initializer_list currently is not move-awared
{
    container container_;
    (container_.emplace_back(std::forward< arguments >(_arguments)), ...);
    return container_;
}

}
