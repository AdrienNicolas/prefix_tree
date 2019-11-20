//
// Created by Adrien Briand on 18/8/2018.
//

#ifndef PREFIX_TREE_NODE_H
#define PREFIX_TREE_NODE_H

#include <utility>
#include <type_traits>
#include <iterator>
#include <memory>

#include "util/types.h"
#include "util/initialized_array.h"
#include "iterator.h"

template<typename Node>
struct getter
{
    typedef Node raw_node_type;
    typedef typename std::remove_const<raw_node_type>::type node_type;
    typedef getter<raw_node_type> type;
    typedef typename node_type::prefix_const_iterator prefix_const_iterator;
    typedef typename node_type::charset_type charset_type;
    typedef typename node_type::key_const_iterator key_const_iterator;
    typedef typename node_type::size_type size_type;

    static std::pair<prefix_const_iterator, raw_node_type *> get_node
    (
    prefix_const_iterator pi,
    raw_node_type * node,
    const charset_type & abc,
    key_const_iterator start,
    key_const_iterator last
    )
    {
        bool matching;
        while(node && start != last)
        {
            prefix_const_iterator pend = node->prefix.end();
            for(;pi != pend && start != last && (matching = *pi == abc.to_int_type(*start)); ++pi, ++start);
            if(pi == pend && start != last)
            {
                if(node->next)
                {
                    size_type i = (size_type) abc.to_int_type(*start);
                    node = node->next->operator[](i).get();
                    if(node)
                    {
                        ++start;
                        pi = node->prefix.begin();
                    }
                }
                else
                {
                    node = nullptr;
                }
            }
            else if(!matching)
            {
                node = nullptr;
            }
        }
        return std::make_pair(pi, node);
    }
};

template<typename Node>
inline std::pair<typename Node::prefix_const_iterator, const Node *> get_node
(
typename Node::prefix_const_iterator pi,
const Node * node,
const typename Node::charset_type & abc,
typename Node::key_const_iterator start,
typename Node::key_const_iterator last
)
{
    return getter<const Node>::get_node(pi, node, abc, start, last);
};

template<typename Node>
inline std::pair<typename Node::prefix_const_iterator, Node *> get_node
(
typename Node::prefix_const_iterator pi,
Node * node,
const typename Node::charset_type & abc,
typename Node::key_const_iterator start,
typename Node::key_const_iterator last
)
{
    return getter<Node>::get_node(pi, node, abc, start, last);
};


template<typename Node>
struct inserter
{
    typedef Node node_type;
    typedef inserter<node_type> type;
    typedef typename node_type::node_ptr node_ptr;
    typedef typename node_type::node_deleter_type node_deleter_type;
    typedef typename node_type::size_type size_type;
    typedef typename node_type::prefix_type prefix_type;
    typedef typename node_type::prefixer_type prefixer_type;
    typedef typename node_type::key_const_iterator key_const_iterator;
    typedef typename node_type::prefix_const_iterator prefix_const_iterator;
    typedef typename node_type::node_container_allocator_type node_container_allocator_type;
    typedef typename node_type::node_allocator_type node_allocator_type;
    typedef typename node_type::prefix_allocator_type prefix_allocator_type;
    typedef typename node_type::charset_type charset_type;
    typedef typename node_type::key_type key_type;

    static node_type * insert_node
    (
    node_type * root,
    node_container_allocator_type & node_container_allocator,
    node_allocator_type & node_allocator,
    prefix_allocator_type & prefix_allocator,
    const charset_type & abc,
    const key_type & key,
    key_const_iterator start,
    key_const_iterator last
    )
    {
        node_ptr empty_pair(nullptr, node_deleter_type(node_allocator));
        while(start != last)
        {
            size_type i = (size_type)abc.to_int_type(*start);
            node_ptr & p = root->get_next(i, empty_pair);
            node_type * current = p.get();
            ++start;
            if(current == nullptr)
            {
                prefix_type remaining = prefixer_type::make_prefix(key, std::distance(key.cbegin(), start), std::distance(start, last));
                start = last;

                root->ensure_next(node_container_allocator, node_allocator);
                root->allocate_node(node_allocator, i, std::move(remaining) );
                current = root->next->operator[](i).get();
            }
            else
            {
                prefix_const_iterator pi = current->prefix.begin();
                prefix_const_iterator pend = current->prefix.end();
                for(; pi != pend && start != last && *pi == abc.to_int_type(*start); ++pi, ++start);
                if(pi != pend)
                {
                    node_ptr jnode(p.release(), node_deleter_type(node_allocator));
                    auto length = std::distance(current->prefix.cbegin(), pi);
                    prefix_type first_half = prefixer_type::make_prefix(current->prefix, 0, length);
                    prefix_type second_half = prefixer_type::make_prefix(current->prefix, length, std::distance(pi, pend));

                    node_ptr & new_p = root->allocate_node(node_allocator, i, std::move(first_half));

                    new_p->ensure_next(node_container_allocator, node_allocator);
                    new_p->set_node((size_type)*pi, std::move(second_half), std::move(jnode));
                    current = new_p.get();
                }
            }
            root = current;
        }
        return root;
    }
};

template<typename Node>
inline Node * insert_node
(
Node * root,
typename Node::node_container_allocator_type & node_container_allocator,
typename Node::node_allocator_type & node_allocator,
typename Node::prefix_allocator_type & prefix_allocator,
const typename Node::charset_type & abc,
const typename Node::key_type & key,
typename Node::key_const_iterator start,
typename Node::key_const_iterator last
)
{
    return inserter<Node>::insert_node(root, node_container_allocator, node_allocator, prefix_allocator, abc, key, start, last);
};

template<typename Node, typename Memory>
struct remover
{
    typedef Node node_type;
    typedef Memory memory_management_type;
    typedef remover<node_type, memory_management_type> type;
    typedef typename node_type::prefix_allocator_type prefix_allocator_type;

    static void remove_node(node_type * current, prefix_allocator_type & prefix_allocator);
};

template<typename Node>
struct remover<Node, own_memory>
{
    typedef Node node_type;
    typedef remover<node_type, own_memory> type;
    typedef typename node_type::prefixer_type prefixer_type;
    typedef typename node_type::value_holder_ptr value_holder_ptr;
    typedef typename node_type::size_type size_type;
    typedef typename node_type::node_ptr node_ptr;
    typedef typename node_type::content_const_iterator content_const_iterator;
    typedef typename node_type::iterator iterator;
    typedef typename node_type::prefix_type prefix_type;
    typedef typename node_type::prefix_allocator_type prefix_allocator_type;

    static void remove_node(node_type * current, prefix_allocator_type & prefix_allocator)
    {
        auto size = prefixer_type::length(current->prefix);

        value_holder_ptr toDelete(std::move(current->value));
        if(current->parent_link.first && current->empty())
        {
            size_type i = current->parent_link.second;
            current = current->parent_link.first;
            size += prefixer_type::length(current->prefix) + 1;
            node_ptr & p = current->next->operator[](i);
            p.reset();
            --current->nb_sub_node;
            if(!current->nb_sub_node)
            {
                current->next.reset();
            }
        }
        if(toDelete && current->parent_link.first)
        {
            auto prefix_start = prefixer_type::length(toDelete->first) - size;
            if(!current->value && current->nb_sub_node == 1) // if current only have one sub node -> current is not required anymore
            {
                iterator it = current->begin();
                for(iterator end = current->end(); it != end && !*it; ++it);

                node_type * parent = current->parent_link.first;
                size_type i = current->parent_link.second;
                auto concatenated_size = prefixer_type::length(current->prefix) + prefixer_type::length((*it)->prefix) + 1;

                content_const_iterator content = content_const_iterator::make_begin(current);
                const auto & replacement_key = content->first;
                prefix_type concatenated = prefixer_type::make_prefix(replacement_key, prefix_start, concatenated_size);
                parent->set_node(i, std::move(concatenated), std::move(*it));
                current = parent;
            }
        }
    }
};

template<typename Node>
struct remover<Node, shared_memory>
{
    typedef Node node_type;
    typedef remover<node_type, shared_memory> type;
    typedef typename node_type::prefixer_type prefixer_type;
    typedef typename node_type::value_holder_ptr value_holder_ptr;
    typedef typename node_type::size_type size_type;
    typedef typename node_type::node_ptr node_ptr;
    typedef typename node_type::content_const_iterator content_const_iterator;
    typedef typename node_type::iterator iterator;
    typedef typename node_type::prefix_type prefix_type;
    typedef typename node_type::prefix_allocator_type prefix_allocator_type;

    static void remove_node(node_type * current, prefix_allocator_type & prefix_allocator)
    {
        auto size = prefixer_type::length(current->prefix);

        value_holder_ptr toDelete(std::move(current->value)); // remove value hold by node
        if(current->parent_link.first && current->empty()) // if current is empty remove it
        {
            size_type i = current->parent_link.second;
            current = current->parent_link.first;
            size += prefixer_type::length(current->prefix) + 1;
            node_ptr & p = current->next->operator[](i);
            p.reset();
            --current->nb_sub_node;
            if(!current->nb_sub_node)
            {
                current->next.reset();
            }
        }
        if(toDelete && current->parent_link.first)
        {
            auto prefix_start = prefixer_type::length(toDelete->first) - size;
            if(!current->value && current->nb_sub_node == 1)
            {
                iterator it = current->begin();
                for(iterator end = current->end(); it != end && !*it; ++it);

                node_type * parent = current->parent_link.first;
                size_type i = current->parent_link.second;
                auto concatenated_size = prefixer_type::length(current->prefix) + prefixer_type::length((*it)->prefix) + 1;

                content_const_iterator content = content_const_iterator::make_begin(current);
                const auto & replacement_key = content->first;
                prefix_type concatenated = prefixer_type::make_prefix(replacement_key, prefix_start, concatenated_size);
                parent->set_node(i, std::move(concatenated), std::move(*it));
                current = parent;
                size = prefixer_type::length(current->prefix);
                prefix_start -= (size+1);
            }
            content_const_iterator content = content_const_iterator::make_begin(current);
            while(current->parent_link.first)
            {
                if(prefixer_type::share_memory(current->prefix, prefixer_type::make_prefix(toDelete->first, prefix_start, size)))
                {
                    current->prefix = prefixer_type::make_prefix(content->first, prefix_start, size);
                }
                current = current->parent_link.first;
                size = prefixer_type::length(current->prefix);
                prefix_start -= (size+1);
            }
        }
    }
};

template<typename Node>
inline static void remove_node(Node * current, typename Node::prefix_allocator_type & prefix_allocator)
{
    return remover<Node, typename Node::prefixer_type::prefix_life_cycle_traits>::remove_node(current, prefix_allocator);
}

template<typename K, typename V, class Charset, class Prefixer, class Allocator>
class node
{
public:
    typedef std::size_t size_type;
    typedef K key_type;
    typedef V value_type;
    typedef Charset charset_type;
    typedef Prefixer prefixer_type;
    typedef Allocator allocator_type;

    typedef node<key_type, value_type, charset_type, prefixer_type, allocator_type> type;
    typedef typename std::allocator_traits<allocator_type>::template rebind_alloc<type> node_allocator_type;
    typedef allocator_deleter<node_allocator_type> node_deleter_type;
    typedef std::unique_ptr<type, node_deleter_type> node_ptr;
    typedef std::pair<type *, size_type> parent_link_type;

    typedef value_type * value_ptr;
    typedef std::pair<key_type, value_type> value_holder;
    typedef typename std::allocator_traits<allocator_type>::template rebind_alloc<value_holder> value_holder_allocator_type;
    typedef allocator_deleter<value_holder_allocator_type> value_holder_deleter_type;
    typedef std::unique_ptr<value_holder, value_holder_deleter_type> value_holder_ptr;

    typedef typename charset_type::index_type index_type;
    typedef typename std::allocator_traits<allocator_type>::template rebind_alloc<index_type> prefix_allocator_type;
    typedef typename prefixer_type::prefix_type prefix_type;
    typedef typename prefix_type::const_iterator prefix_const_iterator;
    typedef std::array<node_ptr, charset_type::size> node_container;
    typedef typename std::allocator_traits<allocator_type>::template rebind_alloc<node_container> node_container_allocator_type;
    typedef std::unique_ptr<node_container> node_container_ptr;

    typedef typename node_container::iterator iterator;
    typedef typename node_container::const_iterator const_iterator;

    typedef typename key_type::iterator key_iterator;
    typedef typename key_type::const_iterator key_const_iterator;

    typedef prefix_tree_iterator<type, readonly_type> content_const_iterator;
    typedef prefix_tree_iterator<type, readwrite_type> content_iterator;

    friend getter<const type>;
    friend getter<type>;
    friend inserter<type>;
    friend remover<type, typename prefixer_type::prefix_life_cycle_traits>;

    explicit node(parent_link_type && link, value_holder_ptr && value, node_allocator_type & node_allocator)
    :parent_link(std::move(link))
    ,nb_sub_node(0)
    ,value(std::move(value))
    {
    }

    ~node() noexcept
    {
        clear();
    }

    void set_parent(parent_link_type && link)
    {
        parent_link = std::move(link);
    }

    const parent_link_type & get_parent() const noexcept
    {
        return parent_link;
    }

    void ensure_next(node_container_allocator_type & node_container_allocator, node_allocator_type & node_allocator)
    {
        if(!next)
        {
            node_container * new_next = node_container_allocator.allocate(1);
            new((void *)new_next) node_container(make_initialized_array<node_ptr, charset_type::size>(node_ptr(nullptr, node_deleter_type(node_allocator))));
            next.reset(new_next);
        }
    }

    node_ptr & allocate_node(node_allocator_type & node_allocator, size_type i, prefix_type && prefix)
    {
        node_ptr & p = next->operator[](i);

        type * new_node = node_allocator.allocate(1);
        new((void *)new_node) type(parent_link_type(this, i), value_holder_ptr(nullptr, value.get_deleter()), node_allocator);

        new_node->prefix = std::move(prefix);
        p.reset(new_node);
        ++nb_sub_node;
        return p;
    }

    void set_node(size_type i, prefix_type && prefix, node_ptr && n)
    {
        node_ptr & p = next->operator[](i);
        p.reset(n.release());
        p->prefix = std::move(prefix);
        p->set_parent(parent_link_type(this, i));
    }

    bool is_leaf() const noexcept
    {
        return nb_sub_node == 0;
    }

    bool empty() const noexcept
    {
        return nb_sub_node == 0 && !value;
    }

    value_holder_ptr & get_value() noexcept
    {
        return value;
    }

    const value_holder_ptr & get_value() const noexcept
    {
        return value;
    }

    node_ptr & get_next(size_type i, node_ptr & empty_pair) noexcept
    {
        return next ? next->operator[](i) : empty_pair;
    }

    iterator begin() noexcept
    {
        return next ? next->begin() : nullptr;
    }

    const_iterator begin() const noexcept
    {
        return next ? next->begin() : nullptr;
    }

    iterator end() noexcept
    {
        return next ? next->end() : nullptr;
    }

    const_iterator end() const noexcept
    {
        return next ? next->end() : nullptr;
    }

    prefix_const_iterator prefix_begin() const
    {
        return this->prefix.begin();
    }

    prefix_const_iterator prefix_end() const
    {
        return this->prefix.end();
    }

    void clear() noexcept
    {
        value.reset();
        if(next)
        {
            for (node_ptr &n : *next.get())
            {
                n.reset();
            }
            next.reset();
        }
        nb_sub_node = 0;
    }
private:
    parent_link_type parent_link;
    node_container_ptr next;
    value_holder_ptr value;
    prefix_type prefix;
    size_t nb_sub_node;
};

#endif //PREFIX_TREE_NODE_H
