#include <memory>
#include <cstddef>
#include <utility>
#include <iostream>

template <typename T, typename Alloc = std::allocator<T>>
class vector 
{
    using traits = std::allocator_traits<Alloc>;
    using pointer = T*;
    using constReference = const T&;

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


    void push_back(const T& value) {
        if (full()) reallocate(capacity() == 0 ? 1 : 2 * capacity());

        std::construct_at(end_, value);
        ++end_;
    }

    template <typename... Args>
    void emplace_back(Args&&... args) {
        if (full())
            reallocate(capacity() == 0 ? 1 : 2 * capacity());

        std::construct_at(end_, std::forward<Args>(args)...);
        ++end_;
    }


    void pop_back(){
        if (end_ == begin_) return;

        --end_;
        std::destroy_at(end_);
    }

    void clear(){
        while (end_ != begin_){
            --end_;
            std::destroy_at(end_);
        }
    }

    T& operator[](size_t i) { return begin_[i]; }
    const T& operator[](size_t i) const { return begin_[i]; }

    constReference at(size_t ind) const {
        if (ind >= size()) {
            throw std::out_of_range("Index is out of range");
        }
        return begin_[ind];
    }


    size_t size() const { return end_ - begin_; }
    size_t capacity() const { return capacity_ - begin_; }

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
};

