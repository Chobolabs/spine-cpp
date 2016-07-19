// shared_vector.hpp - v1.0.0 - public domain
// authored in 2016 by Borislav Stanimirov / Chobolabs
//
// This is a simple wrapper of std::vector providing a shared vector interface
// The class is basically the same as shared_ptr<std::vector<T>>. It only provides
// methods and operators with the same signature as std::vector so a transition
// could be seemless.
//
//
// VERSION_HISTORY
//
// 1.00 (2016-07-19) Initial version
//
//
// LICENSE
//
// This software is public domain.
//
//
// USAGE
//
// Include and use in place of std::vector.
// define _SHARED_VECTOR_NAMESPACE to a namespace you like
// otherwise the namespace of the vector would be `chobo`
//
#pragma once

#include <vector>
#include <memory>

#if !defined(_SHARED_VECTOR_NAMESPACE)
#   define _SHARED_VECTOR_NAMESPACE chobo
#endif

namespace _SHARED_VECTOR_NAMESPACE
{

template <typename T, typename Alloc = std::allocator<T>>
class shared_vector
{
public:
    typedef std::vector<T, Alloc> vector;

    typedef T value_type;
    typedef Alloc allocator_type;
    typedef typename vector::size_type size_type;
    typedef typename vector::difference_type difference_type;
    typedef typename allocator_type::reference reference;
    typedef typename allocator_type::const_reference const_reference;
    typedef typename allocator_type::pointer pointer;
    typedef typename allocator_type::const_pointer const_pointer;
    typedef typename vector::iterator iterator;
    typedef typename vector::const_iterator const_iterator;
    typedef typename std::reverse_iterator<iterator> reverse_iterator;
    typedef typename std::reverse_iterator<const_iterator> const_reverse_iterator;


    explicit shared_vector(const allocator_type& alloc = allocator_type())
        : m_vector(std::make_shared<vector>())
    {}
    explicit shared_vector(const std::shared_ptr<vector>& ptr)
        : m_vector(ptr)
    {}
    explicit shared_vector(const vector& vec)
        : m_vector(std::make_shared(vec))
    {}

    explicit shared_vector(size_type n)
        : m_vector(std::make_shared<vector>(n))
    {}
    shared_vector(size_type n, const value_type& val, const allocator_type& alloc = allocator_type())
        : m_vector(std::make_shared<vector>(n, val, alloc))
    {}

    template <class InputIterator>
    shared_vector(InputIterator first, InputIterator last, const allocator_type& alloc = allocator_type())
        : m_vector(std::make_shared(first, last, alloc))
    {}

    shared_vector(const shared_vector& other)
        : m_vector(other.m_vector)
    {}

    shared_vector(shared_vector&& other)
        : m_vector(std::move(other))
    {}

    vector& vec() { return *m_vector; }
    const vector& vec() const { return *m_vector; }

    iterator begin() noexcept{ return m_vector->begin(); }
    const_iterator begin() const noexcept{ return m_vector->begin(); }
    iterator end() noexcept{ return m_vector->end(); }
    const_iterator end() const noexcept{ return m_vector->end(); }

    reverse_iterator rbegin() noexcept{ return m_vector->rbegin(); }
    const_reverse_iterator rbegin() const noexcept{ return m_vector->rbegin(); }
    reverse_iterator rend() noexcept{ return m_vector->rend(); }
    const_reverse_iterator rend() const noexcept{ return m_vector->rend(); }

    const_iterator cbegin() const noexcept{ return m_vector->cbegin(); }
    const_iterator cend() const noexcept{ return m_vector->cend(); }
    const_reverse_iterator crbegin() const noexcept{ return m_vector->crbegin(); }
    const_reverse_iterator crend() const noexcept{ return m_vector->crend(); }

    size_type size() const noexcept{ return m_vector->size(); }
    size_type max_size() const noexcept{ return m_vector->max_size(); }
    void resize(size_type n) { m_vector->resize(n); }
    void resize(size_type n, const value_type& val) { m_vector->resize(n, val); }
    size_type capacity() const noexcept{ return m_vector->capacity(); }
    bool empty() const noexcept{ return m_vector->empty(); }
    void reserve(size_type n) { m_vector->reserve(n); }
    void shrink_to_fit() { m_vector->shrink_to_fit(); }

    reference operator[] (size_type n) { return m_vector->operator[](n); }
    const_reference operator[] (size_type n) const { return m_vector->operator[](n); }
    reference at(size_type n) { return m_vector->at(n); }
    const_reference at(size_type n) const { return m_vector->at(n); }
    reference front() { return m_vector->front(); }
    const_reference front() const { return m_vector->front(); }
    reference back() { return m_vector->back(); }
    const_reference back() const { return m_vector->back(); }
    value_type* data() noexcept{ return m_vector->data(); }
    const value_type* data() const noexcept{ return m_vector->data(); }

        template <class InputIterator>
    void assign(InputIterator first, InputIterator last) { m_vector->assign(first, last); }
    void assign(size_type n, const value_type& val) { m_vector->assign(n, val); }
    void assign(std::initializer_list<value_type> il) { m_vector->asign(il); }

    void push_back(const value_type& val) { m_vector->push_back(val); }
    void push_back(value_type&& val) { m_vector->push_back(std::forward<value_type>(val)); }

    void pop_back() { m_vector->pop_back(); }

    iterator insert(const_iterator position, const value_type& val) { return m_vector->insert(position, val); }
    iterator insert(const_iterator position, size_type n, const value_type& val) { return m_vector->insert(position, n, val); }
    template <class InputIterator>
    iterator insert(const_iterator position, InputIterator first, InputIterator last) { return m_vector->insert(position, first, last); }
    iterator insert(const_iterator position, value_type&& val) { return m_vector->insert(position, std::forward<value_type>(val)); }
    iterator insert(const_iterator position, std::initializer_list<value_type> il) { return m_vector->insert(il); }

    iterator erase(const_iterator position) { return m_vector->erase(position); }
    iterator erase(const_iterator first, const_iterator last) { return m_vector->erase(first, last); }

    void swap(shared_vector& other) { std::swap(m_vector, other.m_vector); }

    void clear() noexcept{ m_vector->clear(); }

        template <class... Args>
    iterator emplace(const_iterator position, Args&&... args) { m_vector->emplace(position, std::forward<Args>(args)...); }
    template <class... Args>
    void emplace_back(Args&&... args) { m_vector->emplace_back(std::forward<Args>(args)...); }

    allocator_type get_allocator() const noexcept{ return m_vector->get_allocator(); }

    shared_vector& operator=(const shared_vector& other)
    {   
        m_vector = other.m_vector;
        return *this;
    }


private:
    std::shared_ptr<vector> m_vector;
};

template <class T, class Alloc>
bool operator== (const shared_vector<T, Alloc>& lhs, const shared_vector<T, Alloc>& rhs)
{
    return lhs.m_vector == rhs.m_vector;
}
template <class T, class Alloc>
bool operator!= (const shared_vector<T, Alloc>& lhs, const shared_vector<T, Alloc>& rhs)
{
    return lhs.m_vector != rhs.m_vector;
}
template <class T, class Alloc>
bool operator<  (const shared_vector<T, Alloc>& lhs, const shared_vector<T, Alloc>& rhs)
{
    return lhs.m_vector < rhs.m_vector;
}
template <class T, class Alloc>
bool operator<= (const shared_vector<T, Alloc>& lhs, const shared_vector<T, Alloc>& rhs)
{
    return lhs.m_vector <= rhs.m_vector;
}
template <class T, class Alloc>
bool operator>(const shared_vector<T, Alloc>& lhs, const shared_vector<T, Alloc>& rhs)
{
    return lhs.m_vector > rhs.m_vector;
}
template <class T, class Alloc>
bool operator>= (const shared_vector<T, Alloc>& lhs, const shared_vector<T, Alloc>& rhs)
{
    return lhs.m_vector >= rhs.m_vector;
}

}
