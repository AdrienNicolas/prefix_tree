//
// Created by Adrien Briand on 27/4/2018.
//

#ifndef PREFIX_TREE_CHARSET_H
#define PREFIX_TREE_CHARSET_H

#include<array>
#include<iterator>

template <typename L, typename I, std::size_t N = size_t(std::numeric_limits<L>::max)>
class basic_charset
{
public:
    typedef std::size_t size_type;
    typedef L letter_type;
    typedef I index_type;

    static constexpr size_type size = N;
    static constexpr size_type index_size = N;

    inline const index_type & to_index(const letter_type & c) const noexcept
    {
        return c;
    }

    inline const letter_type & to_letter(const index_type & i) const noexcept
    {
        return i;
    }
};

typedef basic_charset<char, char, 128> ascii_charset;
typedef basic_charset<char, unsigned char, 256> extended_ascii_charset;


template<typename L, typename I, L ...Letters>
class custom_charset
{
public:
    typedef std::size_t size_type;
    typedef L letter_type;
    typedef I index_type;
    static constexpr size_type size = sizeof...(Letters);
    static constexpr letter_type index_to_letter[size] = {Letters...};
    static constexpr index_type * letter_to_index = compute_index(index_to_letter);

private:
    constexpr index_type* compute_index(letter_type letters[])
    {
//        size_type size = sizeof(letters);
//        index_type * result = new index_type[sizeof...(l)];
        size_type index_size = ((size_type)*std::max_element(std::begin(letters), std::end(letters))) + 1;
        index_type * letter_to_index = new index_type[index_size];

        size_type i = 0;
        for(letter_type & l:letters)
        {
            letter_to_index[(size_type)l] = (index_type)i;
        }
        return letter_to_index;
    }
};

typedef custom_charset<char, unsigned char, "abcdefghijklmnopqrstuvwxyz"> lower_case_charset;
lower_case_charset lc;

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
    :letter_to_index()
    ,index_to_letter()
    {
        size_type i = 0;
        std::for_each(start, last, [this, &i](const letter_type & c)
        {
            this->letter_to_index[(size_type)c] = (index_type)i;
            this->index_to_letter[i] = c;
            ++i;
        });
    }

    inline const index_type & to_index(const letter_type & c) const
    {
        return letter_to_index[c];
    }

    inline const letter_type & to_letter(const index_type & i) const
    {
        return index_to_letter[i];
    }

private:
    index_container letter_to_index;
    letter_container index_to_letter;
};

#endif //PREFIX_TREE_CHARSET_H
