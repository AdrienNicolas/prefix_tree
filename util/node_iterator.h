//
// Created by Adrien Briand on 18/8/2018.
//

#ifndef PREFIX_TREE_NODE_ITERATOR_H
#define PREFIX_TREE_NODE_ITERATOR_H

#include "types.h"

template <typename Container, class Const>
class all_node_iterator
{
public:
    typedef Container container_type;
    typedef Const constness_type;
    typedef all_node_iterator<container_type, constness_type> type;
    typedef typename container_type::node_type node_type;

    static constexpr bool constness = std::is_same<constness_type, readonly_type>::value;
    typedef typename std::conditional<constness, const node_type *, node_type *>::type node_ptr;
    typedef typename std::conditional<constness, typename node_type::const_iterator, typename node_type::iterator>::type node_iterator;

    all_node_iterator(node_iterator it, node_iterator end)
    :it(it)
    ,end(end)
    {
        for(; it != end && !it->second; ++it);
    }

    type & operator ++()
    {
        for(++it; it != end && !it->second; ++it);
        return *this;
    }

    bool operator ==( const type & right) const
    {
        return it == right.it && end == right.end;
    }

    bool operator !=( const type & right) const
    {
        return it != right.it || end != right.end;
    }

    node_ptr operator *() const
    {
        return it->second.get();
    }

private:
    node_iterator it;
    node_iterator end;
};


template <typename Container, class Const>
class all_node_iterator_generator
{
public:
    typedef Container container_type;
    typedef Const constness_type;
    typedef all_node_iterator_generator<container_type, constness_type> type;
    typedef typename container_type::node_type node_type;

    typedef all_node_iterator<container_type, constness_type> iterator;
    static constexpr bool constness = std::is_same<constness_type, readonly_type>::value;
    typedef typename std::conditional<constness, const node_type *, node_type *>::type node_ptr;
    typedef typename std::conditional<constness, typename node_type::const_iterator, typename node_type::iterator>::type node_iterator;

    iterator begin(node_ptr node) const
    {
        return iterator(node->begin(), node->end());
    }

    iterator end(node_ptr node) const
    {
        return iterator(node->end(), node->end());
    }

    bool select(node_ptr node) const
    {
        return node->get_value();
    }
};

#endif //PREFIX_TREE_NODE_ITERATOR_H
