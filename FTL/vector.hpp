// All content copyright (C) Allan Deutsch 2017. All rights reserved.

#pragma once

#include "allocator.hpp" // ftl::default_allocator

#include <limits> // needed for allocator::max_size
#include <iterator> // ::std::reverse_iterator<>
#include <utility> // ::std::distance
#include <memory>
#include <cassert>
#include <algorithm> // rotate
namespace ftl {
  // vector implementation with ::std::vector parity
  template<typename T, typename Alloc = default_allocator<T>>
  class vector {
  public:
    // type aliases
    using size_type = std::size_t;
    using allocator_type = Alloc;
    using value_type = T;
    using iterator = T*;
    using const_iterator = const T *;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    // constructors
    // default
    vector();
    explicit vector(const allocator_type& alloc);
    // fill
    explicit vector(size_type n, const allocator_type& alloc = allocator_type{});
    vector(size_type n, const value_type& val, const allocator_type& alloc = allocator_type{});
    // range
    template<typename InputIterator>
    vector(InputIterator first, InputIterator last, const allocator_type& alloc = allocator_type{});
    // copy
    vector(const vector<T, Alloc> &other);
    vector(const vector<T, Alloc>& other, const allocator_type& alloc);
    // move
    vector(vector<T, Alloc> &&other);
    vector(vector<T, Alloc> &&other, const allocator_type& alloc);
    // initializer list
    vector(::std::initializer_list<value_type> il, const allocator_type& alloc = allocator_type{});

    virtual ~vector();

    // assignment
    vector<T, Alloc>& operator=(const vector<T, Alloc> &other);
    vector<T, Alloc>& operator=(vector<T, Alloc> &&other);
    vector<T, Alloc>& operator=(::std::initializer_list<value_type> il);

    // iterators
    iterator begin() const noexcept;
    iterator end() const noexcept;
    const_iterator cbegin() const noexcept;
    const_iterator cend() const noexcept;
    reverse_iterator rbegin() const noexcept;
    reverse_iterator rend() const noexcept;
    const_reverse_iterator crbegin() const noexcept;
    const_reverse_iterator crend() const noexcept;

    // Modifiers
    void push_back(const T &data);
    void push_back(T &&data);
    void pop_back();
    template<typename... Args>
    void emplace_back(Args&&... args);

    template<typename InputIterator>
    void assign(InputIterator first, InputIterator last);
    void assign(size_type n, const value_type &val);
    void assign(::std::initializer_list<value_type> il);

    iterator insert(const_iterator position, const value_type &val);
    iterator insert(const_iterator position, size_type n, const value_type &val);
    template<typename InputIterator>
    iterator insert(const_iterator position, InputIterator first, InputIterator last);
    iterator insert(const_iterator position, value_type &&val);
    iterator insert(const_iterator position, ::std::initializer_list<value_type> il);

    virtual iterator erase(iterator position);
    virtual iterator erase(iterator first, iterator last);

    template<typename Vector>
    void swap(Vector &other);
    void swap(vector<T, Alloc> &other);

    void clear() noexcept;

    template<typename... Args>
    iterator emplace(const_iterator position, Args&&... args);

    // element access
    reference front() const;
    reference back() const;
    pointer data() const noexcept;
    reference at(size_type n) noexcept;
    const_reference at(size_type n) const noexcept;
    reference operator[](size_type n);
    const_reference operator[](size_type n) const;

    // capacity
    size_type size() const noexcept;
    size_type capacity() const noexcept;
    size_type max_size() const noexcept;
    bool empty() const noexcept;
    void resize(size_type elements);
    void resize(size_type elements, const value_type &val);
    virtual void reserve(size_type elements);
    void shrink_to_fit();

    // allocator
    allocator_type get_allocator() const noexcept;

  protected:
    vector(iterator Begin, iterator End, size_type Capacity, const allocator_type &alloc = default_allocator<value_type>{});

    virtual void grow();
    bool full() const noexcept;


    pointer m_begin{ nullptr };
    pointer m_end{ nullptr };
    size_type m_capacity{ 0u };
    allocator_type m_alloc{};
  };

  // constructors
  // default
  template<typename T, typename Alloc>
  vector<T, Alloc>::vector() { }
  template<typename T, typename Alloc>
  vector<T, Alloc>::vector(const allocator_type& alloc)
    : m_alloc(alloc) {
  }
  // fill
  template<typename T, typename Alloc>
  vector<T, Alloc>::vector(size_type n, const allocator_type& alloc)
    : m_alloc(alloc) {
    reserve(n);
  }
  template<typename T, typename Alloc>
  vector<T, Alloc>::vector(size_type n, const value_type& val, const allocator_type& alloc)
    : m_alloc(alloc) {
    assign(n, val);
  }
  // range
  template<typename T, typename Alloc>
  template<typename InputIterator>
  vector<T, Alloc>::vector(InputIterator first, InputIterator last, const allocator_type& alloc)
    : m_alloc(alloc) {
    assign(first, last);
  }
  // copy
  template<typename T, typename Alloc>
  vector<T, Alloc>::vector(const vector<T, Alloc> &other)
    : m_alloc(other.get_allocator()) {
    assign(other.begin(), other.end());
  }

  template<typename T, typename Alloc>
  vector<T, Alloc>::vector(const vector<T, Alloc>& other, const allocator_type& alloc)
    : m_alloc(alloc) {
    assign(other.begin(), other.end());
  }
  // move
  template<typename T, typename Alloc>
  vector<T, Alloc>::vector(vector<T, Alloc> &&other)
    : m_begin(other.m_begin)
    , m_end(other.m_end)
    , m_capacity(other.m_capacity) {
    other.m_begin = nullptr;
    other.m_end = nullptr;
    other.m_capacity = 0;
  }
  template<typename T, typename Alloc>
  vector<T, Alloc>::vector(vector<T, Alloc> &&other, const allocator_type& alloc)
    : m_begin(other.begin())
    , m_end(other.end())
    , m_capacity(other.capacity())
    , m_alloc(alloc) {
    other.m_begin = other.m_end = nullptr;
    other.m_capacity = 0u;
  }
  // initializer list
  template<typename T, typename Alloc>
  vector<T, Alloc>::vector(::std::initializer_list<value_type> il, const allocator_type& alloc)
    : m_alloc(alloc) {
    reserve(il.size());
    assign(il);
  }

  // protected for inline_vector
  template<typename T, typename Alloc>
  vector<T, Alloc>::vector(iterator Begin, iterator End, size_type Capacity, const allocator_type &alloc)
    : m_begin(Begin)
    , m_end(End)
    , m_capacity(Capacity)
    , m_alloc(alloc) {
  }

  template<typename T, typename Alloc>
  vector<T, Alloc>::~vector() {
    if(this->capacity() != 0){ 
      clear();
      m_alloc.deallocate(m_begin, m_capacity);
    }
    m_begin = nullptr;
    m_end = nullptr;
    m_capacity = 0;
  }

  // assignment
  template<typename T, typename Alloc>
  vector<T, Alloc>& vector<T, Alloc>::operator=(const vector<T, Alloc> &other) {
    clear();
    assign(other.begin(), other.end());
    return *this;
  }
  template<typename T, typename Alloc>
  vector<T, Alloc>& vector<T, Alloc>::operator=(vector<T, Alloc> &&other) {
    clear();
    m_begin = other.m_begin;
    m_end = other.m_end;
    m_capacity = other.m_capacity;
    other.m_begin = nullptr;
    other.m_end = nullptr;
    other.m_capacity = 0;
    return *this;
  }
  template<typename T, typename Alloc>
  vector<T, Alloc>& vector<T, Alloc>::operator=(::std::initializer_list<T> il) {
    clear();
    assign(il);
    return *this;
  }

  template<typename T, typename Alloc>
  typename vector<T, Alloc>::size_type vector<T, Alloc>::size() const noexcept {
    return m_end - m_begin;
  }

  template<typename T, typename Alloc>
  typename vector<T, Alloc>::size_type vector<T, Alloc>::capacity() const noexcept {
    return m_capacity;
  }

  template<typename T, typename Alloc>
  bool vector<T, Alloc>::full() const noexcept {
    return static_cast<unsigned long long>(::std::distance(m_begin, m_end)) >= m_capacity;
  }

  template<typename T, typename Alloc>
  typename vector<T, Alloc>::size_type vector<T, Alloc>::max_size() const noexcept {
    return m_alloc.max_size();
  }
  template<typename T, typename Alloc>
  bool vector<T, Alloc>::empty() const noexcept {
    return m_begin == m_end;
  }

  template<typename T, typename Alloc>
  void vector<T, Alloc>::resize(size_type elements) {
    resize(elements, value_type{});
  }

  template<typename T, typename Alloc>
  void vector<T, Alloc>::resize(size_type elements, const value_type &val) {
    if (capacity < elements) {
      reserve(elements);
    }
    if (elements > size) {
      for (size_type i{ size() }; i < elements; ++i) {
        push_back(val);
      }
    }
    else {
      while (size() > elements) {
        pop_back();
      }
    }
  }

  template<typename T, typename Alloc>
  void vector<T, Alloc>::reserve(size_type elements) {
    if (capacity() >= elements) return;

    pointer new_buffer = m_alloc.allocate(elements);
    pointer temp_begin = begin(), temp_end = end();
    m_begin = m_end = new_buffer;
    size_type old_capacity{ capacity() };
    m_capacity = elements;
    assign(temp_begin, temp_end);

    for (auto it{ temp_begin }; it != temp_end; ++it) {
      m_alloc.destroy(&*it);
    }
    m_alloc.deallocate(temp_begin, old_capacity);
  }
  template<typename T, typename Alloc>
  void vector<T, Alloc>::shrink_to_fit() {
    // The behavior of this function is implementation defined.
    // This implementation chooses to avoid reallocating into a smaller space.
  }

  template<typename T, typename Alloc>
  typename vector<T, Alloc>::iterator vector<T, Alloc>::begin() const noexcept {
    return m_begin;
  }

  template<typename T, typename Alloc>
  typename vector<T, Alloc>::iterator vector<T, Alloc>::end() const noexcept {
    return m_end;
  }

  template<typename T, typename Alloc>
  typename vector<T, Alloc>::const_iterator vector<T, Alloc>::cbegin() const noexcept {
    return m_begin;
  }
  template<typename T, typename Alloc>
  typename vector<T, Alloc>::const_iterator vector<T, Alloc>::cend() const noexcept {
    return m_end;
  }

  template<typename T, typename Alloc>
  typename vector<T, Alloc>::reverse_iterator vector<T, Alloc>::rbegin() const noexcept {
    return reverse_iterator{ m_end - 1 };
  }
  template<typename T, typename Alloc>
  typename vector<T, Alloc>::reverse_iterator vector<T, Alloc>::rend() const noexcept {
    return reverse_iterator{ m_begin - 1 };
  }
  template<typename T, typename Alloc>
  typename vector<T, Alloc>::const_reverse_iterator vector<T, Alloc>::crbegin() const noexcept {
    return const_reverse_iterator{ m_end - 1 };
  }
  template<typename T, typename Alloc>
  typename vector<T, Alloc>::const_reverse_iterator vector<T, Alloc>::crend() const noexcept {
    return const_reverse_iterator{ m_begin - 1 };
  }

  template<typename T, typename Alloc>
  void vector<T, Alloc>::pop_back() {
    m_alloc.destroy(--m_end);
  }

  template<typename T, typename Alloc>
  void vector<T, Alloc>::push_back(const T &data) {
    if (full()) grow();
    m_alloc.construct(m_end++, data);
  }

  template<typename T, typename Alloc>
  void vector<T, Alloc>::push_back(T &&data) {
    if (full()) grow();
    m_alloc.construct(m_end++, std::forward<T&&>(data));
  }

  template<typename T, typename Alloc>
  template<typename... Args>
  void vector<T, Alloc>::emplace_back(Args&&... args) {
    if (full()) grow();
    m_alloc.construct(m_end, ::std::forward<Args>(args)...);
    ++m_end;
  }





  template<typename T, typename Alloc>
  template<typename InputIterator>
  void vector<T, Alloc>::assign(InputIterator first, InputIterator last) {
    clear();
    reserve(static_cast<size_type>(::std::distance(first, last)));
    for (auto it{ first }; it != last; ++it) {
      emplace_back(*it);
    }
  }

  template<typename T, typename Alloc>
  void vector<T, Alloc>::assign(size_type n, const value_type &val) {
    clear();
    reserve(n);
    for (size_type i{ 0 }; i < n; ++i) {
      emplace_back(std::forward<decltype(val)>(val));
    }
  }

  template<typename T, typename Alloc>
  void vector<T, Alloc>::assign(::std::initializer_list<typename vector<T, Alloc>::value_type> il) {
    clear();
    reserve(il.size());
    for (auto it{ ::std::begin(il) }; it != ::std::end(il); ++it) {
      emplace_back(*it);
    }
  }

  template<typename T, typename Alloc>
  typename vector<T, Alloc>::iterator vector<T, Alloc>::insert(const_iterator position, const value_type &val) {
    iterator it{ const_cast<iterator>(position) };
    emplace_back(val);
    ::std::rotate(it, end() - 1, end());
    return it;
  }

  template<typename T, typename Alloc>
  typename vector<T, Alloc>::iterator vector<T, Alloc>::insert(const_iterator position, size_type n, const value_type &val) {
    iterator it{ const_cast<iterator>(position) }, n_first{ end() };
    for (size_type i{ 0 }; i < n; ++i) {
      emplace_back(val);
    }
    ::std::rotate(it, n_first, end());
    return it;
  }

  template<typename T, typename Alloc>
  template<typename InputIterator>
  typename vector<T, Alloc>::iterator vector<T, Alloc>::insert(const_iterator position, InputIterator first, InputIterator last) {
    iterator it{ const_cast<iterator>(position) }, n_first{ end() };
    for (auto it{ first }; it != last; ++it) {
      emplace_back(*it);
    }
    ::std::rotate(it, n_first, end());
    return it;
  }

  template<typename T, typename Alloc>
  typename vector<T, Alloc>::iterator vector<T, Alloc>::insert(const_iterator position, value_type &&val) {
    iterator it{ const_cast<iterator>(position) };
    emplace_back(val);
    ::std::rotate(it, end() - 1, end());
    return it;
  }

  template<typename T, typename Alloc>
  typename vector<T, Alloc>::iterator vector<T, Alloc>::insert(const_iterator position, ::std::initializer_list<value_type> il) {
    return insert(position, ::std::begin(il), ::std::end(il));
  }

  template<typename T, typename Alloc>
  typename vector<T, Alloc>::iterator vector<T, Alloc>::erase(iterator position) {
    iterator follow{ position };
    iterator lead{ follow + 1 };
    for (; lead != end(); ++follow, ++lead) {
      m_alloc.destroy(follow);
      m_alloc.construct(follow, ::std::move(*lead));
    }
    pop_back();
    return const_cast<iterator>(position);
  }

  template<typename T, typename Alloc>
  typename vector<T, Alloc>::iterator vector<T, Alloc>::erase(iterator first, iterator last) {
    iterator follow{ first };
    iterator lead{ last };

    for (; lead != end() && follow != last; ++lead, ++follow) {
      m_alloc.destroy(follow);
      m_alloc.construct(follow, ::std::move(*lead));
    }
    iterator new_end{ follow };
    while (end() != new_end) {
      pop_back();
    }
    m_end = new_end;
    return const_cast<iterator>(first);
  }

  template<typename T, typename Alloc>
  template<typename Vector>
  void vector<T, Alloc>::swap(Vector &other) {
    static_assert(::std::is_same<value_type, typename Vector::value_type>::value, "Swapping the elements of two vectors requires that they have the same value type.");

    if (size() > other.size()) {
      size_type _diff{ size() - other.size() };
      size_type _count{ 0 };
      for (; _count < size() - _diff; ++_count) {
        std::swap(this->at(_count), other.at(_count));
      }
      size_type _new_end{ _count };
      for (; _count < size(); ++_count) {
        other.emplace_back(std::move(this->at(_count)));
      }
      while (other.size() > _count) {
        pop_back();
      }
    }
    else { //if (other.size() > size()) {
      size_type _diff{ other.size() - size() };
      size_type _count{ 0 };
      for (; _count < other.size() - _diff; ++_count) {
        std::swap(this->at(_count), other.at(_count));
      }
      size_type _new_end{ _count };
      for (; _count < other.size(); ++_count) {
        emplace_back(std::move(other.at(_count)));
      }
      while (other.size() > _count) {
        pop_back();
      }
    }

  }
  template<typename T, typename Alloc>
  void vector<T, Alloc>::swap(vector<T, Alloc> &other) {
    ::std::swap(this->m_begin, other.m_begin);
    ::std::swap(this->m_end, other.m_end);
    ::std::swap(this->m_capacity, other.m_capacity);
    ::std::swap(this->m_alloc, other.m_alloc);
  }

  template<typename T, typename Alloc>
  void vector<T, Alloc>::clear() noexcept {
    while (!this->empty()) pop_back();
  }

  template<typename T, typename Alloc>
  template<typename... Args>
  typename vector<T, Alloc>::iterator vector<T, Alloc>::emplace(const_iterator position, Args&&... args) {
    iterator it{ const_cast<iterator>(position) }, n_first{ end() };
    emplace_back(::std::forward<Args>(args)...);
    ::std::rotate(it, n_first, end());
    return it;
  }

  template<typename T, typename Alloc>
  void vector<T, Alloc>::grow() {
    size_type new_capacity{ (capacity() != 0) ? (capacity() * 2) : 10 };
    reserve(new_capacity);
  }




  template<typename T, typename Alloc>
  typename vector<T, Alloc>::reference vector<T, Alloc>::front() const {
    return *m_begin;
  }
  template<typename T, typename Alloc>
  typename vector<T, Alloc>::reference vector<T, Alloc>::back() const {
    return *(m_end - 1);
  }
  template<typename T, typename Alloc>
  typename vector<T, Alloc>::pointer vector<T, Alloc>::data() const noexcept {
    return m_begin;
  }
  template<typename T, typename Alloc>
  typename vector<T, Alloc>::reference vector<T, Alloc>::at(size_type n) noexcept {
    assert(n < size());
    return *(m_begin + n);
  }
  template<typename T, typename Alloc>
  typename vector<T, Alloc>::const_reference vector<T, Alloc>::at(size_type n) const noexcept {
    assert(n < size());
    return *(m_begin + n);
  }
  template<typename T, typename Alloc>
  typename vector<T, Alloc>::reference vector<T, Alloc>::operator[](size_type n) {
    return *(m_begin + n);
  }
  template<typename T, typename Alloc>
  typename vector<T, Alloc>::const_reference vector<T, Alloc>::operator[](size_type n) const {
    return *(m_begin + n);
  }

  template<typename T, typename Alloc>
  typename vector<T, Alloc>::allocator_type vector<T, Alloc>::get_allocator() const noexcept {
    return m_alloc;
  }


  // inline_vector is a vector derivative with a built in storage buffer for the first N elements,
  // where N is specified as a non-type template parameter.
  template <typename T, std::size_t N, typename Alloc = default_allocator<T>>
  class inline_vector : public vector<T, Alloc> {
  public:
    using value_type = T;
    using pointer = typename vector<T, Alloc>::pointer;
    using size_type = typename vector<T, Alloc>::size_type;
    inline_vector()
      : vector<T, Alloc>(reinterpret_cast<pointer>(inline_buffer),
        reinterpret_cast<pointer>(inline_buffer),
        N)
    {}
    virtual ~inline_vector() override;

    virtual void reserve(size_type elements) override;
  protected:
  private:
    char inline_buffer[sizeof(value_type) * N];
  };
  template<typename T, std::size_t N, typename Alloc>
  inline_vector<T, N, Alloc>::~inline_vector() {
    this->clear();
    if (!(this->m_begin == (pointer)this->inline_buffer)) {
      this->m_alloc.deallocate(this->m_begin, this->m_capacity);
    }
    this->m_begin = nullptr;
    this->m_end = nullptr;
    this->m_capacity = 0;
  }
  template<typename T, std::size_t N, typename Alloc>
  void inline_vector<T, N, Alloc>::reserve(size_type elements) {
    if (this->capacity() >= elements) return;
    // The special case where the inline buffer is the current storage needs to be handled.
    // The buffer shouldn't be deallocated, so the special case for that is implemented here.
    if (this->m_begin == reinterpret_cast<pointer>(inline_buffer)) {

      pointer new_buffer = this->m_alloc.allocate(elements);
      pointer temp_begin = this->begin(), temp_end = this->end();
      this->m_begin = this->m_end = new_buffer;
      size_type old_capacity{ this->capacity() };
      this->m_capacity = elements;
      this->assign(temp_begin, temp_end);

      for (auto it{ temp_begin }; it != temp_end; ++it) {
        this->m_alloc.destroy(it);
      }
    }
    else {
      // If the current buffer isn't the inline buffer, we can just use the default grow() method.
      vector<T, Alloc>::reserve(elements);
    }
  }
  
template<typename T, typename Alloc = default_allocator<T>>
class unordered_vector : public vector<T, Alloc> {
public:
  using iterator = typename vector<T, Alloc>::iterator;
  
  using vector<T, Alloc>::vector;
  iterator erase(iterator position) override;
  iterator erase(iterator first, iterator last) override;

};
  

template<typename T, typename Alloc>
typename vector<T, Alloc>::iterator unordered_vector<T, Alloc>::erase(iterator position) {
  iterator it{ position };
  if (it != this->m_end - 1) {
    *it = this->back();
  }
  this->pop_back();
  return const_cast<iterator>(position);
}

template<typename T, typename Alloc>
typename vector<T, Alloc>::iterator unordered_vector<T, Alloc>::erase(iterator first, iterator last) {
  assert(first >= this->m_begin && "Iterator out of range.");
  assert(last <= this->m_end && "Iterator out of range.");
   const iterator temp_end{ last }, temp_begin{ first };
  if (temp_end == this->m_end) {
    while (this->m_end >= temp_begin)
      this->pop_back();
    return const_cast<iterator>(first);
  }
  iterator it{ temp_begin };
  for (; it != last && last != this->m_end; ++it) {
    *it = this->back();
    this->pop_back();
  }
  while (it++ != last) {
    this->pop_back();
  }
  return (temp_begin);
}
} // namespace ftl
