//
// Created by Adrien Briand on 10/6/2018.
//

#ifndef PREFIX_TREE_ITERATOR_H
#define PREFIX_TREE_ITERATOR_H

#include <type_traits>
#include <utility>
#include <iterator>
#include "util/types.h"

template <typename Node, class Const>
class prefix_tree_iterator
{
public:
    typedef Node node_type;
    typedef Const constness_type;
    typedef prefix_tree_iterator<node_type, constness_type> type;

    typedef typename node_type::value_holder value_type;

    typedef prefix_tree_iterator<node_type, readwrite_type> readwrite_iterator_type;
    typedef typename std::conditional<std::is_same<constness_type, readonly_type>::value, readwrite_type, readonly_type>::type other_constness;
    typedef prefix_tree_iterator<node_type, other_constness> other_type;
    friend other_type;

    static constexpr bool constness = std::is_same<constness_type, readonly_type>::value;
    typedef typename std::conditional<constness, const node_type *, node_type *>::type node_ptr;
    typedef typename std::conditional<constness, typename node_type::const_iterator, typename node_type::iterator>::type node_iterator;
    typedef typename std::conditional<constness, const value_type &, value_type &>::type reference;
    typedef typename std::conditional<constness, const value_type *, value_type *>::type pointer;

    static type make_begin(node_ptr node)
    {
        return type(node, node ? node->get_parent().first : nullptr);
    }

    static type make_end(node_ptr node)
    {
        node_ptr last = node ? node->get_parent().first : nullptr;
        return type(last, last);
    }

    explicit prefix_tree_iterator(node_ptr node, node_ptr last) noexcept
    :last(last)
    {
        while(node != last && !node->get_value())
        {
            node_iterator it = node->begin();
            node_iterator end = node->end();
            for(; it != end && !*it; ++it);
            if(it != end)
            {
                node = it->get();
            }
            else
            {
                node = last;
            }
        }
        current = node;
    }

    prefix_tree_iterator(readwrite_iterator_type && iterator) noexcept
    :current(std::move(iterator.current))
    ,last(std::move(iterator.last))
    {
    }

    reference operator*() const
    {
        return *(current->get_value().get());
    }

    pointer operator->() const
    {
        return current->get_value().get();
    }

    bool operator ==(const type & right) const noexcept
    {
        return current == right.current;
    }

    bool operator !=(const type & right) const noexcept
    {
        return current != right.current;
    }

    type & operator++() noexcept
    {
        if(current != last)
        {
            pointer value = nullptr;
            node_iterator it = current->begin();

            if(current->is_leaf())
            {
                it = current->end();
            }
            do
            {
                node_iterator end = current->end();
                for(; it != end && !*it; ++it);
                if(it != end)
                {
                    current = it->get();
                    value = current->get_value().get();
                    it = current->begin();
                }
                else
                {
                    const auto & parent = current->get_parent();
                    current = parent.first;
                    if(current != last)
                    {
                        it = current->begin();
                        std::advance(it, parent.second + 1);
                    }
                }
            }
            while(current != last && !value);
        }
        return *this;
    }

    const node_ptr get_node() const
    {
        return current;
    }

    node_ptr get_node()
    {
        return current;
    }

private:
    node_ptr current;
    node_ptr const last;
};

#endif //PREFIX_TREE_ITERATOR_H
