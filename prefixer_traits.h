//
// Created by Adrien Briand on 18/8/2018.
//

#ifndef PREFIX_TREE_PREFIXER_TRAITS_H
#define PREFIX_TREE_PREFIXER_TRAITS_H

#include <memory>
#include <string>
#include <string_view>

template<class K, class SubK, class MemoryManagement>
class prefixer_traits
{
public:
    typedef K key_type;
    typedef SubK prefix_type;
    typedef MemoryManagement prefix_life_cycle_traits;
    typedef prefixer_traits<key_type, prefix_type, prefix_life_cycle_traits> type;

    typedef typename prefix_type::size_type size_type;

    static prefix_type make_prefix(const key_type & key, size_type start, size_type length);
    static size_type length(const prefix_type & prefix);
};

template<class CharT, class Traits = std::char_traits<CharT>, class Allocator = std::allocator<CharT> >
class basic_string_prefixer_traits
{
public:
    typedef std::basic_string<CharT, Traits, Allocator> key_type;
    typedef key_type prefix_type;
    typedef own_memory prefix_life_cycle_traits;
    typedef prefixer_traits<key_type, prefix_type, prefix_life_cycle_traits> type;

    typedef typename prefix_type::size_type size_type;

    static prefix_type make_prefix(const key_type & key, size_type start, size_type length)
    {
        return prefix_type(key, start, length);
    }
    static size_type length(const prefix_type & prefix)
    {
        return prefix.length();
    }
};

typedef basic_string_prefixer_traits<char> string_prefixer_traits;

template<class CharT, class Traits = std::char_traits<CharT>, class Allocator = std::allocator<CharT> >
class basic_string_view_prefixer_traits
{
public:
    typedef std::basic_string<CharT, Traits, Allocator> key_type;
    typedef std::basic_string_view<CharT, Traits> prefix_type;
    typedef shared_memory prefix_life_cycle_traits;
    typedef prefixer_traits<key_type, prefix_type, prefix_life_cycle_traits> type;

    typedef typename prefix_type::size_type size_type;

    static prefix_type make_prefix(const key_type & key, size_type start, size_type length)
    {
        return prefix_type(key.c_str()+start, length);
    }
    static size_type length(const prefix_type & prefix)
    {
        return prefix.length();
    }
    static bool share_memory(const prefix_type & left, const prefix_type & right)
    {
        return left.data() == right.data();
    }
};

#endif //PREFIX_TREE_PREFIXER_TRAITS_H
