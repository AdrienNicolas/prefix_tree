//
// Created by Adrien Briand on 18/8/2018.
//

#ifndef PREFIX_TREE_INITIALIZED_ARRAY_H
#define PREFIX_TREE_INITIALIZED_ARRAY_H

#include <utility>
#include <array>


template<typename T, size_t...Ix, typename... Args>
std::array<T, sizeof...(Ix)> repeat(std::index_sequence<Ix...>, Args &&... args) {
    return {{((void)Ix, T(std::forward<Args>(args)...))...}};
}

template<typename T, size_t N, typename... Args>
std::array<T,N> make_initialized_array(Args &&... args)
{
    return std::array<T, N>(repeat<T>(std::make_index_sequence<N>(), std::forward<Args>(args)...));
};

#endif //PREFIX_TREE_INITIALIZED_ARRAY_H
