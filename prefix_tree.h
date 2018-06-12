//
// Created by Adrien Briand on 21/4/2018.
//

#ifndef PREFIX_TREE_PREFIX_TREE_H
#define PREFIX_TREE_PREFIX_TREE_H


#include <memory>
#include <array>
#include <list>
#include <limits>
#include <algorithm>
#include <iterator>

#include "iterator.h"

template<class Allocator>
class unique_allocation
{
public:
    typedef Allocator allocator_type;
    typedef unique_allocation<allocator_type> type;

    typedef typename allocator_type::size_type size_type;
    typedef typename allocator_type::value_type value_type;

    explicit unique_allocation(allocator_type & allocator, size_type n = 1)
    :allocator(allocator)
    ,n(n)
    ,p(allocator.allocate(n))
    {
    }

    ~unique_allocation()
    {
        if(p)
        {
            allocator.deallocate(p, n);
        }
    }

    value_type * get() const noexcept
    {
        return p;
    }

    value_type * release() noexcept
    {
        value_type * result = p;
        p = nullptr;
        return result;
    }

private:
    allocator_type & allocator;
    size_type n;
    value_type * p;
};

template< typename T >
struct array_deleter
{
    void operator ()( T const * p)
    {
        delete[] p;
    }
};

template<class Allocator>
class allocator_deleter
{
public:
    typedef Allocator allocator_type;
    typedef allocator_deleter<allocator_type> type;
    typedef typename allocator_type::size_type size_type;
    typedef typename allocator_type::value_type value_type;

    explicit allocator_deleter(allocator_type & allocator) noexcept
    :length(1)
    ,allocator(allocator)
    {
    }

    allocator_deleter(const size_type length, allocator_type & allocator) noexcept
    :length(length)
    ,allocator(allocator)
    {
    }

    allocator_deleter(const allocator_deleter & deleter) noexcept
    :length(deleter.length)
    ,allocator(deleter.allocator)
    {
    }

    void operator()(value_type * t)
    {
        t->~value_type();
        allocator.deallocate(t, length);
    }

private:
    size_type length;
    allocator_type & allocator;
};

template<typename I>
class prefix
{
public:
    typedef I index_type;
    typedef prefix<index_type> type;
    typedef type reference;
    typedef index_type * index_ptr;
    typedef std::size_t size_type;
    typedef std::shared_ptr<index_type> buffer_ptr;
    typedef index_ptr iterator;

    template<class Allocator>
    static type concat(Allocator & allocator, const type & first, const type & second)
    {
        type result(allocator, first.size() + second.size());
        std::copy(second.begin(), second.end(), std::copy(first.begin(), first.end(), result.begin()));
        return result;
    }

    prefix() noexcept
	:buffer(nullptr)
    ,start(nullptr)
	,last(nullptr)
	{
	}

    template<class Allocator>
	prefix(Allocator & allocator, size_type length)
    :buffer(allocator.allocate(length), allocator_deleter<Allocator>(length, allocator))
    ,start(this->buffer.get())
    ,last(this->start + length)
    {
    }
	
    prefix(const type & toSplit, iterator start) noexcept
    :buffer(toSplit.buffer)
	,start(start)
    ,last(toSplit.end())
    {
    }

    prefix(const type & toSplit, iterator start, iterator last) noexcept
    :buffer(toSplit.buffer)
    ,start(start)
    ,last(last)
    {
    }

    prefix(const type & k) noexcept
    :buffer(k.buffer)
    ,start(k.start)
    ,last(k.last)
    {
    }

    iterator begin() const noexcept
    {
        return start;
    }

    iterator end() const noexcept
    {
        return last;
    }

    size_type size() const noexcept
    {
        return last - start;
    }

    bool empty() const noexcept
    {
        return start == last;
    }

    void reset() noexcept
    {
        buffer.reset();
        start = 0;
        last = 0;
    }

private:
    buffer_ptr buffer;
    index_ptr start;
    index_ptr last;
};

template<typename T, size_t...Ix, typename... Args>
std::array<T, sizeof...(Ix)> repeat(std::index_sequence<Ix...>, Args &&... args) {
    return {{((void)Ix, T(std::forward<Args>(args)...))...}};
}

template<typename T, size_t N>
class initialized_array: public std::array<T, N> {
public:
    template<typename... Args>
    initialized_array(Args &&... args)
    : std::array<T, N>(repeat<T>(std::make_index_sequence<N>(), std::forward<Args>(args)...)) {}
};

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
    typedef initialized_array<node_ptr, charset_type::size> node_container;
    typedef typename std::allocator_traits<allocator_type>::template rebind_alloc<node_container> node_container_allocator_type;
    typedef std::unique_ptr<node_container> node_container_ptr;

    typedef typename node_container::iterator iterator;
    typedef typename node_container::const_iterator const_iterator;

    typedef prefix_tree_iterator<type, readonly_type> content_const_iterator;
    typedef prefix_tree_iterator<type, readwrite_type> content_iterator;

    template<class Node_type, class Iterator>
    static std::pair<prefix_const_iterator, Node_type> get(prefix_const_iterator pi, Node_type node, const charset_type & abc, Iterator start, Iterator last)
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

    template <typename Key, typename Iterator>
    static type * insert_node(type * root, node_container_allocator_type & node_container_allocator, node_allocator_type & node_allocator, prefix_allocator_type & prefix_allocator, const charset_type & abc, Key & key, Iterator start, Iterator last)
    {
        node_ptr empty_pair(nullptr, node_deleter_type(node_allocator));
        while(start != last)
        {
            size_type i = (size_type)abc.to_int_type(*start);
            node_ptr & p = root->get_next(i, empty_pair);
            type * current = p.get();
            if(current == nullptr)
            {
                prefix_type remaining = prefixer_type::make_prefix(key, std::distance(key.begin(), start), std::distance(start, last));
                start = last;
/*                auto it = remaining.begin();
                for(;start != last;++start,++it)
                {
                    *it = abc.to_int_type(*start);
                }*/
                root->ensure_next(node_container_allocator, node_allocator);
                root->allocate_node(node_allocator, i, std::move(remaining) );
                current = root->next->operator[](i).get();
            }
            else
            {
                Iterator from = start;
                prefix_const_iterator pi = current->prefix.begin();
                prefix_const_iterator pend = current->prefix.end();
                for(; pi != pend && start != last && *pi == abc.to_int_type(*start); ++pi, ++start);
                if(pi != pend)
                {
                    node_ptr jnode(p.release(), node_deleter_type(node_allocator));
                    prefix_type first_half = prefixer_type::make_prefix(current->prefix, 0, std::distance(current->prefix.cbegin(), pi));
                    prefix_type second_half = prefixer_type::make_prefix(current->prefix, std::distance(current->prefix.cbegin(), pi), std::distance(pi, pend));

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

    static void remove(type * current, prefix_allocator_type & prefix_allocator)
    {
        auto size = prefixer_type::length(current->prefix);

        value_holder_ptr toDelete(std::move(current->value));
        if(current->parent_link.first && current->empty())
        {
            size_type i = current->parent_link.second;
            current = current->parent_link.first;
            size += prefixer_type::length(current->prefix);
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
            content_const_iterator content = content_const_iterator::make_begin(current);
            const type * replacement = content.get_node();
            const auto & replacement_key = content->first;
            auto prefix_start = prefixer_type::length(toDelete->first) - size;
            if(!current->value && current->nb_sub_node == 1)
            {
                iterator it = current->begin();
                for(iterator end = current->end(); it != end && !*it; ++it);

                type * parent = current->parent_link.first;
                size_type i = current->parent_link.second;
                auto concatenated_size = prefixer_type::length(current->prefix) + prefixer_type::length((*it)->prefix);
                prefix_type concatenated = prefixer_type::make_prefix(replacement_key, prefix_start, concatenated_size);
                parent->set_node(i, std::move(concatenated), std::move(*it));
                current = parent;
            }
        }
    }

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
            new((void *)new_next) node_container(node_ptr(nullptr, node_deleter_type(node_allocator)));
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

struct SharedMemory;
struct OwnMemory;

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
    typedef OwnMemory prefix_life_cycle_traits;
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

template<class K, class T, class Charset, class Allocator = std::allocator<T> >
class prefix_tree_view
{

};

template<class K, class T, class Charset, class Prefixer, class Allocator = std::allocator<T> >
class prefix_tree
{
public:
	typedef std::size_t size_type;

    typedef K key_type;
    typedef T mapped_type;
    typedef Charset charset_type;
    typedef Prefixer prefixer_type;
    typedef Allocator allocator_type;
    typedef prefix_tree<key_type,mapped_type,charset_type,prefixer_type,allocator_type> type;
    typedef mapped_type value_type;

    typedef typename charset_type::index_type index_type;
    typedef typename charset_type::letter_type letter_type;

    typedef node<key_type, mapped_type, charset_type, prefixer_type, allocator_type> node_type;
    typedef typename node_type::parent_link_type parent_link_type;
    typedef typename node_type::node_container node_container;
    typedef typename node_type::prefix_const_iterator prefix_const_iterator;

    typedef typename std::allocator_traits<allocator_type>::template rebind_alloc<node_type> node_allocator_type;
    typedef typename std::allocator_traits<allocator_type>::template rebind_alloc<index_type> prefix_allocator_type;
    typedef typename std::allocator_traits<allocator_type>::template rebind_alloc<node_container> node_container_allocator_type;
    typedef typename node_type::value_holder value_holder;
    typedef typename std::allocator_traits<allocator_type>::template rebind_alloc<value_holder> value_holder_allocator_type;
    typedef typename node_type::value_holder_ptr value_holder_ptr;
    typedef typename node_type::value_holder_deleter_type value_holder_deleter_type;

    typedef mapped_type & reference;
    typedef prefix<index_type> prefix_type;

    typedef prefix_tree_iterator<node_type, readonly_type> const_iterator;
    typedef prefix_tree_iterator<node_type, readwrite_type> iterator;

    explicit prefix_tree(const charset_type & abc = charset_type(), const allocator_type & allocator = allocator_type())
    :abc(abc)
    ,allocator(allocator)
    ,node_allocator(this->allocator)
    ,prefix_allocator(this->allocator)
    ,root(parent_link_type(nullptr,0), value_holder_ptr(nullptr, value_holder_deleter_type(this->allocator)), this->node_allocator)
    {
    }
	
    reference at(const key_type & key) const
    {
        const node_type * node = exact_match(node_type::get(root.prefix_begin(), &root, abc, key.begin(), key.end()));
        if(!node)
        {
            throw std::out_of_range("key not found");
        }
        else
        {
            return node->get_value()->second;
        }
    }

    reference at(const key_type & key)
    {
        const node_type * node = exact_match(node_type::get(root.prefix_begin(), &root, abc, key.begin(), key.end()));
        if(!node)
        {
            throw std::out_of_range("key not found");
        }
        else
        {
            return node->get_value()->second;
        }
    }

    reference operator[] ( const key_type & k)
    {
        unique_allocation a(this->allocator, 1);
        new((void *)a.get()) value_holder(k, mapped_type());
        value_holder_ptr value(a.release(), value_holder_deleter_type(this->allocator));

        node_type * node = node_type::insert_node(&this->root, this->node_container_allocator, this->node_allocator, this->prefix_allocator, abc, value->first, value->first.begin(), value->first.end());
        value_holder_ptr & existing = node->get_value();
        if(!existing)
        {
            existing.reset(value.release());
        }
        return existing->second;
    }

    std::pair<iterator, bool> insert(const key_type & k, const mapped_type & toInsert)
    {
        unique_allocation a(this->allocator, 1);
        new((void *)a.get()) value_holder(k, mapped_type());
        value_holder_ptr value(a.release(), value_holder_deleter_type(this->allocator));

        node_type * node = node_type::insert_node(&this->root, this->node_container_allocator, this->node_allocator, this->prefix_allocator, abc, value->first, value->first.begin(), value->first.end());
        value_holder_ptr & existing = node->get_value();
        if(!existing)
        {
            existing.reset(value.release());
        }
        return std::make_pair(iterator(node), value.get() == nullptr);
    }

    std::pair<iterator, bool> insert(const key_type & k, mapped_type && toInsert)
    {
        unique_allocation a(this->allocator, 1);
        new((void *)a.get()) value_holder(k, std::forward<mapped_type>(toInsert));
        value_holder_ptr value(a.release(), value_holder_deleter_type(this->allocator));

        node_type * node = node_type::insert_node(&this->root, this->node_container_allocator, this->node_allocator, this->prefix_allocator, abc, value->first, value->first.begin(), value->first.end());
        value_holder_ptr & existing = node->get_value();
        if(!existing)
        {
            existing.reset(value.release());
        }
        return std::make_pair(iterator::make_begin(node), value.get() == nullptr);
    }

    template <typename P>
    std::pair<iterator, bool> insert(const key_type & k, P && toInsert)
    {
        unique_allocation a(this->allocator, 1);
        new((void *)a.get()) value_holder(k, std::forward<P>(toInsert));
        value_holder_ptr value(a.release(), value_holder_deleter_type(this->allocator));

        node_type * node = node_type::insert_node(&this->root, this->node_container_allocator, this->node_allocator, this->prefix_allocator, abc, value->first, value->first.begin(), value->first.end());
        value_holder_ptr & existing = node->get_value();
        if(!existing)
        {
            existing.reset(value.release());
        }
        return std::make_pair(iterator::make_begin(node), value.get() == nullptr);
    }

	const_iterator erase(const_iterator pos)
	{
        node_type * to_erase = const_cast<node_type *>(pos.get_node());
		if(to_erase)
		{
		    ++pos;
		    node_type::remove(to_erase, this->prefix_allocator);
		}
		return pos;
	}
	
	iterator erase(iterator pos)
	{
        node_type * to_erase = pos.get_node();
        if(to_erase)
        {
            ++pos;
            node_type::remove(to_erase, this->prefix_allocator);
        }
        return pos;
	}
	
	const_iterator erase( const_iterator first, const_iterator last )
	{
		while(first != last)
		{
            first = erase(first);
		}
		return first;
	}
	
	iterator erase( iterator first, iterator last )
	{
        while(first != last)
        {
            first = erase(first);
        }
		return first;
	}
	
	size_type erase( const key_type& key )
	{
        size_type result = 0;
        node_type * node = exact_match(node_type::get(root.prefix_begin(), &root, abc, key.begin(), key.end()));
		if(node)
		{
		    node_type::remove(node, this->prefix_allocator);
			result = size_type(1);
		}
		return result;
	}
	
	iterator begin() noexcept
    {
        return iterator::make_begin(&root);
    }
	
    const_iterator begin() const noexcept
    {
        return const_iterator::make_begin(&root);
    }
	
    const_iterator cbegin() const noexcept
    {
        return const_iterator::make_begin(&root);
    }
	
    iterator end() noexcept
    {
        return iterator::make_end(&root);
    }
	
    const_iterator end() const noexcept
    {
        return const_iterator::make_end(&root);
    }

    const_iterator cend() const noexcept
    {
        return const_iterator::make_end(&root);
    }
	
    allocator_type get_allocator() const noexcept
    {
        return allocator;
    }

    bool empty() const noexcept
    {
        return root.empty();
    }

    void clear() noexcept
    {
        root.clear();
    }

    size_type count( const key_type & key ) const
    {
        const node_type * node = exact_match(node_type::get(root.prefix_begin(), &root, abc, key.begin(), key.end()));
        return node ? 1 : 0;
    }

    iterator find( const key_type& key )
    {
        return iterator::make_begin(exact_match(node_type::get(root.prefix_begin(), &root, abc, key.begin(), key.end())));
    }

    const_iterator find( const key_type& key ) const
    {
        return const_iterator::make_begin(exact_match(node_type::get(root.prefix_begin(), &root, abc, key.begin(), key.end())));
    }

    const_iterator lower_bound( const key_type & key) const
    {
        return const_iterator::make_begin(node_type::get(root.prefix_begin(), &root, abc, key.begin(), key.end()).second);
    }

    iterator lower_bound( const key_type & key)
    {
        return iterator::make_begin(node_type::get(root.prefix_begin(), &root, abc, key.begin(), key.end()).second);
    }

    const_iterator upper_bound( const key_type & key) const
    {
        std::pair<prefix_const_iterator, const node_type *> p = node_type::get(root.prefix_begin(), &root, abc, key.begin(), key.end());
        const_iterator result(p.second);
        if(exact_match(p))
        {
            ++result;
        }
        return result;
    }

    iterator upper_bound( const key_type & key)
    {
        std::pair<prefix_const_iterator, const node_type *> p = node_type::get(root.prefix_begin(), &root, abc, key.begin(), key.end());
        iterator result(p.second);
        if(exact_match(p))
        {
            ++result;
        }
        return result;
    }

    void start_with( key_type & key ) const
    {
    }

private:
    template<typename NodePtr>
    static NodePtr exact_match(const std::pair<prefix_const_iterator, NodePtr> & p)
    {
        return p.second && p.second->get_value() && p.first == p.second->prefix_end() ? p.second : nullptr;
    }

    const charset_type abc;
    value_holder_allocator_type allocator;
    node_allocator_type node_allocator;
    prefix_allocator_type prefix_allocator;
    node_container_allocator_type node_container_allocator;
    node_type root;
};

#endif //PREFIX_TREE_PREFIX_TREE_H
