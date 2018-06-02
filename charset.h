//
// Created by Adrien Briand on 27/4/2018.
//

#ifndef PREFIX_TREE_CHARSET_H
#define PREFIX_TREE_CHARSET_H

#include<array>
#include<iterator>
#include<limits>
#include<string>

template <typename CharTrait, std::size_t N = size_t(std::numeric_limits<typename CharTrait::char_type>::max())>
class char_traits_charset
{
public:
    typedef std::size_t size_type;
    typedef CharTrait char_trait;
    typedef typename char_trait::char_type letter_type;
    typedef typename char_trait::int_type index_type;

    static constexpr size_type size = N;
    static constexpr size_type index_size = N;

    inline index_type to_int_type(const letter_type & c) const noexcept
    {
        return char_trait::to_int_type(c);
    }

    inline letter_type to_char_type(const index_type & i) const noexcept
    {
        return char_trait::to_char_type(i);
    }
};

typedef char_traits_charset<std::char_traits<char> > ascii_charset;
typedef char_traits_charset<std::char_traits<char>, 256> extended_ascii_charset;

template<typename T, T * String>
class letter_range
{
public:
    typedef T letter_type;
    typedef const T* string_type;
    typedef const letter_type * const_iterator;
    typedef std::size_t size_type;

    static constexpr string_type range = String;
    static constexpr size_type length = sizeof(String)/sizeof(letter_type);

    constexpr const_iterator begin()
    {
        return range;
    }

    constexpr const_iterator end()
    {
        return range + length;
    }
};

template<typename ...Ranges>
std::size_t range_length(Ranges ...ranges);

template<>
std::size_t range_length()
{
    return 0;
}

template<typename Range, typename ...Ranges>
std::size_t range_length()
{
    return Range::length + range_length<Ranges...>();
}


template<typename L, typename I, typename ...Ranges>
class custom_charset
{
public:
    typedef std::size_t size_type;
    typedef L letter_type;
    typedef I index_type;
    static constexpr size_type size = range_length<Ranges...>();
    static constexpr letter_type int_to_char[size] = {Ranges::data...};
    static constexpr index_type * char_to_int = compute_index(int_to_char);

private:
    constexpr index_type* compute_index(letter_type letters[])
    {
//        size_type size = sizeof(letters);
//        index_type * result = new index_type[sizeof...(l)];
        size_type index_size = ((size_type)*std::max_element(std::begin(letters), std::end(letters))) + 1;
        index_type * char_to_int = new index_type[index_size];

        size_type i = 0;
        for(letter_type & l:letters)
        {
            char_to_int[(size_type)l] = (index_type)i;
        }
        return char_to_int;
    }
};

//typedef custom_charset<char, unsigned char, "abcdefghijklmnopqrstuvwxyz"> lower_case_charset;
//lower_case_charset lc;

template<typename L, typename I, std::size_t N, std::size_t M = std::numeric_limits<L>::max>
class generic_charset
{
public:
    typedef std::size_t size_type;
    static constexpr size_type size = N;
    static constexpr size_type index_size = M;
    typedef L letter_type;
    typedef I index_type;
    typedef std::array<letter_type, size> letter_container;
    typedef std::array<index_type, index_size> index_container;
    typedef generic_charset<letter_type, index_type, size, index_size> type;

    template < typename Iterator>
    generic_charset(Iterator start, Iterator last)
    :char_to_int()
    ,int_to_char()
    {
        size_type i = 0;
        std::for_each(start, last, [this, &i](const letter_type & c)
        {
            this->char_to_int[(size_type)c] = (index_type)i;
            this->int_to_char[i] = c;
            ++i;
        });
    }

    inline const index_type & to_int_type(const letter_type & c) const
    {
        return char_to_int[c];
    }

    inline const letter_type & to_char_type(const index_type & i) const
    {
        return int_to_char[i];
    }

private:
    index_container char_to_int;
    letter_container int_to_char;
};

#endif //PREFIX_TREE_CHARSET_H
