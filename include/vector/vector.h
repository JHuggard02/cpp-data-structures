#include <memory>
#include <cstddef>
#include <utility>
#include <iostream>

template <typename T, typename Alloc = std::allocator<T>>
class vector 
{
    using traits = std::allocator_traits<Alloc>;
    using pointer = T*;
    using reference = T&;
    using constReference = const T&;

public:
    class Iterator {
    public:
        using iterator_category = std::random_access_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = T;
        using pointer = T*;
        using reference = T&;

    private:
        pointer ptr;

    public:
        explicit Iterator(pointer p) : ptr(p) {}

        reference operator*() const { return *ptr; }
        pointer operator->() const { return ptr; }

        Iterator& operator++() { ++ptr; return *this; }       
        Iterator operator++(int) { Iterator temp = *this; ++ptr; return temp; } 

        Iterator& operator--() { --ptr; return *this; }     
        Iterator operator--(int) { Iterator temp = *this; --ptr; return temp; } 

        reference operator[](difference_type n) const { return ptr[n]; }

        Iterator operator+(difference_type n) const { return Iterator(ptr + n); }
        Iterator operator-(difference_type n) const { return Iterator(ptr - n); }
        difference_type operator-(const Iterator& other) const { return ptr - other.ptr; }

        bool operator==(const Iterator& other) const { return ptr == other.ptr; }
        bool operator!=(const Iterator& other) const { return ptr != other.ptr; }
        bool operator<(const Iterator& other) const { return ptr < other.ptr; }
        bool operator>(const Iterator& other) const { return ptr > other.ptr; }
        bool operator<=(const Iterator& other) const { return ptr <= other.ptr; }
        bool operator>=(const Iterator& other) const { return ptr >= other.ptr; }

        friend class ConstIterator;
    };

    class ConstIterator {
    public:
        using iterator_category = std::random_access_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = T;
        using pointer           = const T*;
        using reference         = const T&;

    private:
        pointer ptr;

    public:
        explicit ConstIterator(pointer p) : ptr(p) {}
        ConstIterator(const Iterator& it) : ptr(it.ptr) {}

        reference operator*() const { return *ptr; }
        pointer operator->() const { return ptr; }

        ConstIterator& operator++() { ++ptr; return *this; }
        ConstIterator operator++(int) { ConstIterator temp = *this; ++ptr; return temp; }

        ConstIterator& operator--() { --ptr; return *this; }
        ConstIterator operator--(int) { ConstIterator temp = *this; --ptr; return temp; }

        reference operator[](difference_type n) const { return ptr[n]; }

        ConstIterator operator+(difference_type n) const { return ConstIterator(ptr + n); }
        ConstIterator operator-(difference_type n) const { return ConstIterator(ptr - n); }
        difference_type operator-(const ConstIterator& other) const { return ptr - other.ptr; }

        bool operator==(const ConstIterator& other) const { return ptr == other.ptr; }
        bool operator!=(const ConstIterator& other) const { return ptr != other.ptr; }
        bool operator<(const ConstIterator& other) const { return ptr < other.ptr; }
        bool operator>(const ConstIterator& other) const { return ptr > other.ptr; }
        bool operator<=(const ConstIterator& other) const { return ptr <= other.ptr; }
        bool operator>=(const ConstIterator& other) const { return ptr >= other.ptr; }
    };



private:

    pointer begin_ = nullptr;
    pointer end_ = nullptr;
    pointer capacity_ = nullptr;
    Alloc alloc_;

public:
    vector() = default;

    vector(size_t n, const T& value) {
        allocate(n);
        construct_uniform(value, n);
        end_ = begin_ + n;
    }

    vector(const vector& other) {
        allocate(other.capacity());
        end_ = begin_;
        for (size_t i = 0; i < other.size(); ++i, ++end_) {
            std::construct_at(end_, other[i]);
        }
    }

    vector& operator=(const vector& other) {
        if (this == &other) return *this;

        destroy_all();
        deallocate();
        allocate(other.capacity());

        end_ = begin_;
        for (size_t i = 0; i < other.size(); ++i, ++end_) {
            std::construct_at(end_, other[i]);
        }

        return *this;
    }

    vector(vector&& other) noexcept{
        begin_ = other.begin_;
        end_ = other.end_;
        capacity_ = other.capacity_;

        other.begin_ = other.end_ = other.capacity_ = nullptr;
    }

    vector& operator=(vector&& other) noexcept {
        if (this != &other) {
            destroy_all();
            deallocate();

            begin_ = other.begin_;
            end_ = other.end_;
            capacity_ = other.capacity_;

            other.begin_ = other.end_ = other.capacity_ = nullptr;
        }
        return *this;
    }


    ~vector() {
        destroy_all();
        deallocate();
    }

    void swap(vector& other) noexcept {
        std::swap(begin_, other.begin_);
        std::swap(end_, other.end_);
        std::swap(capacity_, other.capacity_);
        std::swap(alloc_, other.alloc_);
    }

    void reserve(size_t n) {
        if (n <= capacity()) return; 

        pointer new_begin = traits::allocate(alloc_, n);
        pointer new_end = new_begin;

        for (pointer p = begin_; p != end_; ++p, ++new_end) {
            std::construct_at(new_end, std::move(*p));
            std::destroy_at(p);
        }

        traits::deallocate(alloc_, begin_, capacity());

        begin_ = new_begin;
        end_ = new_end;
        capacity_ = new_begin + n;
    }

    void shrink_to_fit() {
        if (size() == capacity()) return;
        
        size_t sz = size();
        pointer new_begin = traits::allocate(alloc_, sz);
        pointer new_end = new_begin;

      
        for (pointer p = begin_; p != end_; ++p, ++new_end) {
            std::construct_at(new_end, std::move(*p));
            std::destroy_at(p);
        }

        traits::deallocate(alloc_, begin_, capacity());

        begin_ = new_begin;
        end_ = new_end;          
        capacity_ = begin_ + sz; 
    }

    void assign(size_t n, const T& value){
        clear();
        reserve(n);

        for (size_t i = 0; i < n; ++i){
            std::construct_at(begin_ + i, value);
        }

        end_ = begin_ + n;
    }


    void push_back(const T& value) {
        if (full()) reallocate(capacity() == 0 ? 1 : 2 * capacity());

        std::construct_at(end_, value);
        ++end_;
    }

    void push_back(T&& value) {
        if (full()) reallocate(capacity() == 0 ? 1 : 2 * capacity());
        std::construct_at(end_, std::move(value));
        ++end_;
    }

    template <typename... Args>
    void emplace_back(Args&&... args) {
        if (full())
            reallocate(capacity() == 0 ? 1 : 2 * capacity());

        std::construct_at(end_, std::forward<Args>(args)...);
        ++end_;
    }


    void pop_back() noexcept {
        if (end_ == begin_) return;

        --end_;
        std::destroy_at(end_);
    }

    void clear() noexcept {
        while (end_ != begin_){
            --end_;
            std::destroy_at(end_);
        }
    }

    reference operator[](size_t i) { return begin_[i]; }
    constReference operator[](size_t i) const { return begin_[i]; }

    reference at(size_t ind) {
        if (ind >= size()) throw std::out_of_range("Index is out of range");
        return begin_[ind];
    }

    constReference at(size_t ind) const {
        if (ind >= size()) throw std::out_of_range("Index is out of range");
        return begin_[ind];
    }

    pointer data() { return begin_; }
    const pointer data() const { return begin_; }


    size_t size() const noexcept { return end_ - begin_; }
    size_t capacity() const noexcept { return capacity_ - begin_; }

    Iterator begin() noexcept { return Iterator(begin_); }
    Iterator end() noexcept { return Iterator(end_); }

    ConstIterator begin() const noexcept { return ConstIterator(begin_); }
    ConstIterator end() const noexcept { return ConstIterator(end_); }

    ConstIterator cbegin() const noexcept { return ConstIterator(begin_); }
    ConstIterator cend() const noexcept { return ConstIterator(end_); }

    Iterator insert(ConstIterator pos, constReference value) {
        size_t index = pos - begin_;
        shift_right(index);
        std::construct_at(begin_ + index, value);
        ++end_;
        return Iterator(begin_ + index);
    }

    Iterator erase(ConstIterator pos{
        size_t ind = pos - begin_;
        std::destroy_at(begin_ + ind);
        shift_left(ind);
        --end_;
        return Iterator(begin_ + ind);
    }

    Iterator erase(ConstIterator first, ConstIterator last) {
        size_t start = first - begin_;
        size_t count = last - first;

        for (size_t i = 0; i < count; ++i)
            std::destroy_at(begin_ + start + i);

        for (size_t i = start + count; i < size(); ++i) {
            std::construct_at(begin_ + i - count, std::move(begin_[i]));
            std::destroy_at(begin_ + i);
        }

        end_ -= count;
        return Iterator(begin_ + start);
    }


private:
    

    void allocate(size_t n) {
        begin_ = traits::allocate(alloc_, n);
        end_ = begin_;
        capacity_ = begin_ + n;
    }

    void deallocate() {
        if (begin_) {
            traits::deallocate(alloc_, begin_, capacity() );
            begin_ = end_ = capacity_ = nullptr;
        }
    }

    void construct_uniform(const T& value, size_t n) {
        for (size_t i = 0; i < n; ++i)
            std::construct_at(begin_ + i, value);
        end_ = begin_ + n;
    }

    void destroy_all() {
        for (pointer p = begin_; p != end_; ++p)
            std::destroy_at(p);
        end_ = begin_;
    }

    void reallocate(size_t newCapacity) {
        pointer new_begin = traits::allocate(alloc_, newCapacity);
        pointer new_end = new_begin;

        for (pointer p = begin_; p != end_; ++p, ++new_end) {
            std::construct_at(new_end, std::move(*p));
            std::destroy_at(p);
        }

        traits::deallocate(alloc_, begin_, capacity());

        begin_ = new_begin;
        end_ = new_end;
        capacity_ = new_begin + newCapacity;
    }

    bool full() const {
        return end_ == capacity_;
    }

    void shift_right(size_t index) {
        if (full()) reallocate(capacity() == 0 ? 1 : 2 * capacity());

        for (size_t i = size(); i > index; --i) {
            std::construct_at(begin_ + i, std::move(begin_[i - 1]));
            std::destroy_at(begin_ + i - 1);
        }

    }

    void shift_left(size_t index){
        for (size_t i = index; i < size() - 1; ++i){
            std::construct_at(begin_ + i, std::move(begin_[i + 1]));
            std::destroy_at(begin_ + i + 1);
        }
    }
};

