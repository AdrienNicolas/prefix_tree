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

template< typename T >
struct array_deleter
{
    void operator ()( T const * p)
    {
        delete[] p;
    }
};

template<class T, class Allocator>
class allocator_deleter
{
public:
    typedef T allocated_type;
    typedef Allocator allocator_type;
    typedef allocator_deleter<allocated_type, allocator_type> type;
    typedef std::size_t size_type;

    typedef allocated_type * allocated_ptr;

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

    void operator()(allocated_ptr t)
    {
        t->~allocated_type();
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
    :buffer(allocator.allocate(length), allocator_deleter<index_type, Allocator>(length, allocator))
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

template<typename K, typename V, class Charset, class Allocator>
class node
{
public:
    typedef std::size_t size_type;
    typedef K key_type;
    typedef V value_type;
    typedef Charset charset_type;
    typedef Allocator allocator_type;

    typedef node<key_type, value_type, charset_type, allocator_type> type;
    typedef typename std::allocator_traits<allocator_type>::template rebind_alloc<type> node_allocator_type;
    typedef allocator_deleter<type, node_allocator_type> node_deleter_type;
    typedef std::unique_ptr<type, node_deleter_type> node_ptr;
    typedef std::pair<type *, size_type> parent_link_type;

    typedef value_type * value_ptr;
    typedef allocator_deleter<value_type, allocator_type> value_deleter_type;
    typedef std::unique_ptr<value_type, value_deleter_type> owned_value_type;
	
	typedef typename charset_type::index_type index_type;
    typedef typename std::allocator_traits<allocator_type>::template rebind_alloc<index_type> prefix_allocator_type;
    typedef prefix<index_type> prefix_type;
    typedef std::pair<prefix_type, node_ptr> pair_type;
    typedef typename prefix_type::iterator key_inner_iterator;
    typedef initialized_array<pair_type, charset_type::size> node_container;
	
    typedef typename node_container::iterator iterator;
    typedef typename node_container::const_iterator const_iterator;

    template<class Node_type, class Iterator>
    static Node_type get(Node_type node, const charset_type & abc, Iterator start, Iterator last)
    {
        while(node && start != last)
        {
            size_type i = (size_type) abc.to_index(*start);
            const pair_type & p = node->next[i];
            node = nullptr;
            if(!p.first.empty())
            {
                key_inner_iterator pi = p.first.begin();
                key_inner_iterator pend = p.first.end();
                for(;pi != pend && start != last && *pi == abc.to_index(*start); ++pi, ++start);
                if(pi == pend)
                {
                    node = p.second.get();
                }
            }
        }
        return node;
    }

    template <typename Iterator>
    static type * insert_node(type * root, node_allocator_type & node_allocator, prefix_allocator_type & prefix_allocator, const charset_type & abc, Iterator start, Iterator last)
    {
        while(start != last)
        {
            size_type i = (size_type)abc.to_index(*start);
            pair_type & p = root->next[i];

            key_inner_iterator pi = p.first.begin();
            key_inner_iterator pend = p.first.end();
            for(; pi != pend && start != last && *pi == abc.to_index(*start); ++pi, ++start);
            if(pi != pend)
            {
                node_ptr jnode(p.second.release(), node_deleter_type(node_allocator));
                prefix_type second_half(p.first, pi);

                root->allocate_node(node_allocator, i, prefix_type(p.first, p.first.begin(), pi));

                p.second->set_node((size_type)*pi, std::move(second_half), std::move(jnode));
            }
            else if(!p.second)
            {
                prefix_type remaining(prefix_allocator, std::distance(start, last));
                index_type * it = remaining.begin();
                for(;start != last;++start,++it)
                {
                    *it = abc.to_index(*start);
                }
                root->allocate_node(node_allocator, i, std::move(remaining) );
            }
            root = p.second.get();
        }
        return root;
    }

    static void remove(type * current, prefix_allocator_type & prefix_allocator)
    {
        current->value.release();
        if(current->parent_link.first && current->empty())
        {
            size_type i = current->parent_link.second;
            current = current->parent_link.first;
            pair_type & p = current->next[i];
            p.first.reset();
            p.second.reset();
            --current->nb_sub_node;
        }
        if(current->parent_link.first && !current->value && current->nb_sub_node == 1)
        {
            iterator it = current->begin();
            for(iterator end = current->end(); it != end && !it->second; ++it);
            type * parent = current->parent_link.first;
            size_type i = current->parent_link.second;
            parent->set_node(i, prefix_type::concat(prefix_allocator, parent->next[i].first, it->first), std::move(it->second));
            current = parent;
        }
    }

    explicit node(parent_link_type && link, owned_value_type && value, node_allocator_type & node_allocator)
    :parent_link(std::move(link))
    ,nb_sub_node(0)
    ,next(prefix_type(),node_ptr(nullptr, node_deleter_type(node_allocator)))
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

    void allocate_node(node_allocator_type & node_allocator, size_type i, prefix_type && prefix)
    {
        pair_type & p = next[i];

        type * new_node = node_allocator.allocate(1);
        new((void *)new_node) type(parent_link_type(this, i), owned_value_type(nullptr, value.get_deleter()), node_allocator);

        p.first = std::move(prefix);
        p.second.reset(new_node);
        ++nb_sub_node;
    }

    void set_node(size_type i, prefix_type && prefix, node_ptr && n)
    {
        pair_type & p = next[i];
        p.first = std::move(prefix);
        p.second.reset(n.release());
        p.second->set_parent(parent_link_type(this, i));
    }

    bool is_leaf() const noexcept
    {
        return nb_sub_node == 0;
    }

    bool empty() const noexcept
    {
        return nb_sub_node == 0 && !value;
    }

    owned_value_type & get_value() noexcept
    {
        return value;
    }

    const owned_value_type & get_value() const noexcept
    {
        return value;
    }

    iterator begin() noexcept
    {
        return next.begin();
    }

    const_iterator begin() const noexcept
    {
        return next.begin();
    }

    iterator end() noexcept
    {
        return next.end();
    }

    const_iterator end() const noexcept
    {
        return next.end();
    }

    void clear() noexcept
    {
        value.reset();
        for(pair_type & n : next)
        {
            n.first.reset();
            n.second.reset();
        }
        nb_sub_node = 0;
    }
private:
    parent_link_type parent_link;
    node_container next;
    owned_value_type value;
    size_t nb_sub_node;
};

struct readonly_type;
struct readwrite_type;

template <typename Container, class Const>
class prefix_tree_iterator
{
    typedef Container owner_type;
	friend owner_type;
public:
    typedef typename Container::value_type value_type;
	typedef Const constness_type;
    typedef prefix_tree_iterator<owner_type, constness_type> type;

    typedef prefix_tree_iterator<owner_type, readwrite_type> readwrite_iterator_type;
    typedef typename std::conditional<std::is_same<constness_type, readonly_type>::value, readwrite_type, readonly_type>::type other_constness;
    typedef prefix_tree_iterator<owner_type, other_constness> other_type;
    friend other_type;
    typedef typename Container::node_type node_type;
	
	static constexpr bool constness = std::is_same<constness_type, readonly_type>::value;
    typedef typename std::conditional<constness, const node_type *, node_type *>::type node_ptr;
    typedef typename std::conditional<constness, typename node_type::const_iterator, typename node_type::iterator>::type node_iterator;
    typedef typename std::conditional<constness, const value_type &, value_type &>::type reference;

	typedef std::pair<node_iterator, node_iterator> node_iterator_pair;
    typedef std::list<node_iterator_pair> node_iterator_container;

    typedef typename readwrite_iterator_type::node_iterator_pair readwrite_node_iterator_pair;

    explicit prefix_tree_iterator(node_ptr node) noexcept
    {
        while(node && !node->get_value())
        {
            node_iterator it = node->begin();
            node_iterator end = node->end();
            for(; it != end && !it->second; ++it);
            if(it != end)
            {
                node = it->second.get();
            }
            else
            {
                node = nullptr;
            }
        }
        current = node;
    }

    prefix_tree_iterator(readwrite_iterator_type && iterator) noexcept
    :current(std::move(iterator.current))
    {
    }

    reference operator*() const
    {
        return *(current->get_value().get());
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
        if(current != nullptr)
        {
            node_ptr node = current;
            node_iterator it = node->begin();
            node_iterator end = node->end();
            if(node->is_leaf())
            {
                const auto & parent = node->get_parent();
                node = parent.first;
                if(node)
                {
                    it = node->begin();
                    end = node->end();
                    std::advance(it, parent.second + 1);
                }
            }
            current = nullptr;
            while(!current && node)
            {
                for(; it != end && !it->second; ++it);
                if(it != end)
                {
                    node = it->second.get();
                    if(node->get_value() == nullptr)
                    {
                        it = node->begin();
                        end = node->end();
                    }
                    else
                    {
                        current = node;
                    }
                }
                else
                {
                    const auto & parent = node->get_parent();
                    node = parent.first;
                    if(node)
                    {
                        it = node->begin();
                        end = node->end();
                        std::advance(it, parent.second + 1);
                    }
                }
            }
        }
        return *this;
    }
	
private:
    node_ptr current;
};

template<class K, class T, class Charset, class Allocator = std::allocator<T> >
class prefix_tree
{
public:
	typedef std::size_t size_type;

    typedef K key_type;
    typedef T mapped_type;
    typedef Charset charset_type;
    typedef Allocator allocator_type;
    typedef prefix_tree<key_type,mapped_type,charset_type,allocator_type> type;
    typedef mapped_type value_type;

    typedef typename charset_type::index_type index_type;
    typedef typename charset_type::letter_type letter_type;

    typedef node<key_type, mapped_type, charset_type, allocator_type> node_type;
    typedef typename node_type::parent_link_type parent_link_type;

    typedef typename std::allocator_traits<allocator_type>::template rebind_alloc<node_type> node_allocator_type;
    typedef typename std::allocator_traits<allocator_type>::template rebind_alloc<index_type> prefix_allocator_type;
    typedef typename node_type::owned_value_type mapped_type_ptr;
    typedef typename node_type::value_deleter_type deleter_type;

    typedef mapped_type & reference;
    typedef prefix<index_type> prefix_type;

    typedef prefix_tree_iterator<type, readonly_type> const_iterator;
    typedef prefix_tree_iterator<type, readwrite_type> iterator;

    explicit prefix_tree(const charset_type & abc = charset_type(), const allocator_type & allocator = allocator_type())
    :abc(abc)
    ,allocator(allocator)
    ,node_allocator(this->allocator)
    ,prefix_allocator(this->allocator)
    ,root(parent_link_type(nullptr,0), mapped_type_ptr(nullptr, deleter_type(this->allocator)), this->node_allocator)
    {
    }
	
    reference at(const key_type & key) const
    {
        const node_type * node = node_type::get(&root, abc, key.begin(), key.end());
        if(!node || !node->get_value())
        {
            throw std::out_of_range("key not found");
        }
        else
        {
            return *node->get_value().get();
        }
    }

    reference at(const key_type & key)
    {
        const node_type * node = node_type::get(&root, abc, key.begin(), key.end());
        if(!node|| !node->get_value())
        {
            throw std::out_of_range("key not found");
        }
        else
        {
            return *node->get_value().get();
        }
    }

    reference operator[] ( const key_type & k)
    {
        mapped_type_ptr & value = node_type::insert_node(&this->root, this->node_allocator, this->prefix_allocator, abc, k.begin(), k.end())->get_value();
        if(!value)
        {
            mapped_type * r = this->allocator.allocate(1);
            new((void *)r) mapped_type();
            value.reset(r);
        }
        return *value.get();
    }

    std::pair<iterator, bool> insert(const key_type & k, const mapped_type & toInsert)
    {
        node_type * node = node_type::insert_node(&this->root, this->node_allocator, this->prefix_allocator, abc, k.begin(), k.end());
        mapped_type_ptr & value = node->get_value();
        mapped_type * result = nullptr;
        if(!value)
        {
            result = this->allocator.allocate(1);
            new((void *) result) mapped_type(toInsert);
            value.reset(result);
        }
        return std::make_pair(iterator(node), result != nullptr);
    }

    std::pair<iterator, bool> insert(const key_type & k, mapped_type && toInsert)
    {
        node_type * node = node_type::insert_node(&this->root, this->node_allocator, this->prefix_allocator, abc, k.begin(), k.end());
        mapped_type_ptr & value = node->get_value();
        mapped_type * result = nullptr;
        if(!value)
        {
            result = this->allocator.allocate(1);
            new((void *) result) mapped_type(std::forward<mapped_type>(toInsert));
            value.reset(result);
        }
        return std::make_pair(iterator(node), result != nullptr);
    }

    template <typename P>
    std::pair<iterator, bool> insert(const key_type & k, P && toInsert)
    {
        node_type * node = node_type::insert_node(&this->root, this->node_allocator, this->prefix_allocator, abc, k.begin(), k.end());
        mapped_type_ptr & value = node->get_value();
        mapped_type * result = nullptr;
        if(!value)
        {
            result = this->allocator.allocate(1);
            new((void *) result) mapped_type(std::forward<P>(toInsert));
            value.reset(result);
        }
        return std::make_pair(iterator(node), result != nullptr);
    }

    template <class... Args>
    mapped_type_ptr insert_emplace(const key_type & k, Args&&... args)
    {
        mapped_type_ptr & value = node_type::insert_node(&this->root, this->node_allocator, this->prefix_allocator, abc, k.begin(), k.end())->get_value();
        mapped_type * result = value.release();

        mapped_type * r = this->allocator.allocate(1);
        new((void *)r) mapped_type(std::forward<Args>(args)...);
        value.reset(r);

        return mapped_type_ptr(result, deleter_type(allocator));
    }

	const_iterator erase(const_iterator pos)
	{
        node_type * to_erase = const_cast<node_type *>(pos.current);
		if(to_erase)
		{
		    ++pos;
		    node_type::remove(to_erase, this->prefix_allocator);
		}
		return pos;
	}
	
	iterator erase(iterator pos)
	{
        node_type * to_erase = pos.current;
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
        node_type * node = node_type::get(&root, abc, key.begin(), key.end());
		if(node)
		{
		    node_type::remove(node, this->prefix_allocator);
			result = size_type(1);
		}
		return result;
	}
	
	iterator begin() noexcept
    {
        return iterator(&root);
    }
	
    const_iterator begin() const noexcept
    {
        return const_iterator(&root);
    }
	
    const_iterator cbegin() const noexcept
    {
        return const_iterator(&root);
    }
	
    iterator end() noexcept
    {
        return iterator(nullptr);
    }
	
    const_iterator end() const noexcept
    {
        return const_iterator(nullptr);
    }

    const_iterator cend() const noexcept
    {
        return const_iterator(nullptr);
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

private:
    const charset_type abc;
    allocator_type allocator;
    node_allocator_type node_allocator;
    prefix_allocator_type prefix_allocator;
    node_type root;
};

#endif //PREFIX_TREE_PREFIX_TREE_H
