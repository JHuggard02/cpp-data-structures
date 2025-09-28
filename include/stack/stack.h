#include <memory>
#include <stdexcept>
#include <iterator>
#include <type_traits>
#include <initializer_list>

template<typename T, typename Allocator = std::allocator<T>>
class stack {
public:
    using value_type = T;
    using allocator_type = Allocator;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = typename std::allocator_traits<Allocator>::pointer;
    using const_pointer = typename std::allocator_traits<Allocator>::const_pointer;

    class iterator {
        friend class stack;
    private:
        pointer ptr_;
        
    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using reference = T&;
        
        iterator() noexcept : ptr_(nullptr) {}
        explicit iterator(pointer ptr) noexcept : ptr_(ptr) {}
        
        reference operator*() const noexcept { return *ptr_; }
        pointer operator->() const noexcept { return ptr_; }
        
        iterator& operator++() noexcept { ++ptr_; return *this; }
        iterator operator++(int) noexcept { iterator tmp(*this); ++ptr_; return tmp; }
        iterator& operator--() noexcept { --ptr_; return *this; }
        iterator operator--(int) noexcept { iterator tmp(*this); --ptr_; return tmp; }
        
        iterator& operator+=(difference_type n) noexcept { ptr_ += n; return *this; }
        iterator& operator-=(difference_type n) noexcept { ptr_ -= n; return *this; }
        
        iterator operator+(difference_type n) const noexcept { return iterator(ptr_ + n); }
        iterator operator-(difference_type n) const noexcept { return iterator(ptr_ - n); }
        
        difference_type operator-(const iterator& other) const noexcept { return ptr_ - other.ptr_; }
        
        reference operator[](difference_type n) const noexcept { return ptr_[n]; }
        
        bool operator==(const iterator& other) const noexcept { return ptr_ == other.ptr_; }
        bool operator!=(const iterator& other) const noexcept { return ptr_ != other.ptr_; }
        bool operator<(const iterator& other) const noexcept { return ptr_ < other.ptr_; }
        bool operator<=(const iterator& other) const noexcept { return ptr_ <= other.ptr_; }
        bool operator>(const iterator& other) const noexcept { return ptr_ > other.ptr_; }
        bool operator>=(const iterator& other) const noexcept { return ptr_ >= other.ptr_; }
    };
    
    class const_iterator {
        friend class stack;
    private:
        const_pointer ptr_;
        
    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = const T*;
        using reference = const T&;
        
        const_iterator() noexcept : ptr_(nullptr) {}
        explicit const_iterator(const_pointer ptr) noexcept : ptr_(ptr) {}
        const_iterator(const iterator& it) noexcept : ptr_(it.ptr_) {}
        
        const_reference operator*() const noexcept { return *ptr_; }
        const_pointer operator->() const noexcept { return ptr_; }
        
        const_iterator& operator++() noexcept { ++ptr_; return *this; }
        const_iterator operator++(int) noexcept { const_iterator tmp(*this); ++ptr_; return tmp; }
        const_iterator& operator--() noexcept { --ptr_; return *this; }
        const_iterator operator--(int) noexcept { const_iterator tmp(*this); --ptr_; return tmp; }
        
        const_iterator& operator+=(difference_type n) noexcept { ptr_ += n; return *this; }
        const_iterator& operator-=(difference_type n) noexcept { ptr_ -= n; return *this; }
        
        const_iterator operator+(difference_type n) const noexcept { return const_iterator(ptr_ + n); }
        const_iterator operator-(difference_type n) const noexcept { return const_iterator(ptr_ - n); }
        
        difference_type operator-(const const_iterator& other) const noexcept { return ptr_ - other.ptr_; }
        
        const_reference operator[](difference_type n) const noexcept { return ptr_[n]; }
        
        bool operator==(const const_iterator& other) const noexcept { return ptr_ == other.ptr_; }
        bool operator!=(const const_iterator& other) const noexcept { return ptr_ != other.ptr_; }
        bool operator<(const const_iterator& other) const noexcept { return ptr_ < other.ptr_; }
        bool operator<=(const const_iterator& other) const noexcept { return ptr_ <= other.ptr_; }
        bool operator>(const const_iterator& other) const noexcept { return ptr_ > other.ptr_; }
        bool operator>=(const const_iterator& other) const noexcept { return ptr_ >= other.ptr_; }
    };
    
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

private:
    static constexpr size_type INITIAL_CAPACITY = 16;
    
    pointer data_;
    size_type size_;
    size_type capacity_;
    [[no_unique_address]] allocator_type alloc_;
    
    using alloc_traits = std::allocator_traits<allocator_type>;
    
    void grow() {
        size_type new_capacity = capacity_ == 0 ? INITIAL_CAPACITY : capacity_ * 2;
        pointer new_data = alloc_traits::allocate(alloc_, new_capacity);
        
        if constexpr (std::is_nothrow_move_constructible_v<T>) {
            for (size_type i = 0; i < size_; ++i) {
                alloc_traits::construct(alloc_, new_data + i, std::move(data_[i]));
                alloc_traits::destroy(alloc_, data_ + i);
            }
        } else {
            size_type constructed = 0;
            try {
                for (; constructed < size_; ++constructed) {
                    alloc_traits::construct(alloc_, new_data + constructed, data_[constructed]);
                }
                for (size_type i = 0; i < size_; ++i) {
                    alloc_traits::destroy(alloc_, data_ + i);
                }
            } catch (...) {
                for (size_type i = 0; i < constructed; ++i) {
                    alloc_traits::destroy(alloc_, new_data + i);
                }
                alloc_traits::deallocate(alloc_, new_data, new_capacity);
                throw;
            }
        }
        
        if (data_) {
            alloc_traits::deallocate(alloc_, data_, capacity_);
        }
        
        data_ = new_data;
        capacity_ = new_capacity;
    }
    
    void destroy_all() noexcept {
        for (size_type i = 0; i < size_; ++i) {
            alloc_traits::destroy(alloc_, data_ + i);
        }
        if (data_) {
            alloc_traits::deallocate(alloc_, data_, capacity_);
        }
    }

public:
    explicit stack(const allocator_type& alloc = allocator_type()) 
        : data_(nullptr), size_(0), capacity_(0), alloc_(alloc) {}
    
    explicit stack(size_type count, const allocator_type& alloc = allocator_type())
        : stack(alloc) {
        reserve(count);
    }
    
    stack(size_type count, const T& value, const allocator_type& alloc = allocator_type())
        : stack(alloc) {
        reserve(count);
        for (size_type i = 0; i < count; ++i) {
            push(value);
        }
    }
    
    template<typename InputIt>
    stack(InputIt first, InputIt last, const allocator_type& alloc = allocator_type())
        : stack(alloc) {
        for (; first != last; ++first) {
            push(*first);
        }
    }
    
    stack(std::initializer_list<T> init, const allocator_type& alloc = allocator_type())
        : stack(init.begin(), init.end(), alloc) {}
    
    stack(const stack& other) 
        : stack(alloc_traits::select_on_container_copy_construction(other.alloc_)) {
        reserve(other.size_);
        for (size_type i = 0; i < other.size_; ++i) {
            alloc_traits::construct(alloc_, data_ + i, other.data_[i]);
            ++size_;
        }
    }
    
    stack(stack&& other) noexcept
        : data_(other.data_), size_(other.size_), capacity_(other.capacity_), alloc_(std::move(other.alloc_)) {
        other.data_ = nullptr;
        other.size_ = 0;
        other.capacity_ = 0;
    }
    
    ~stack() {
        destroy_all();
    }
    
    stack& operator=(const stack& other) {
        if (this != &other) {
            stack tmp(other);
            swap(tmp);
        }
        return *this;
    }
    
    stack& operator=(stack&& other) noexcept {
        if (this != &other) {
            destroy_all();
            data_ = other.data_;
            size_ = other.size_;
            capacity_ = other.capacity_;
            alloc_ = std::move(other.alloc_);
            
            other.data_ = nullptr;
            other.size_ = 0;
            other.capacity_ = 0;
        }
        return *this;
    }
    
    stack& operator=(std::initializer_list<T> init) {
        clear();
        for (const auto& value : init) {
            push(value);
        }
        return *this;
    }
    
    reference top() {
        if (empty()) throw std::out_of_range("stack::top(): stack is empty");
        return data_[size_ - 1];
    }
    
    const_reference top() const {
        if (empty()) throw std::out_of_range("stack::top(): stack is empty");
        return data_[size_ - 1];
    }
    
    reference operator[](size_type pos) noexcept {
        return data_[pos];
    }
    
    const_reference operator[](size_type pos) const noexcept {
        return data_[pos];
    }
    
    reference at(size_type pos) {
        if (pos >= size_) throw std::out_of_range("stack::at(): index out of range");
        return data_[pos];
    }
    
    const_reference at(size_type pos) const {
        if (pos >= size_) throw std::out_of_range("stack::at(): index out of range");
        return data_[pos];
    }
    
    bool empty() const noexcept { return size_ == 0; }
    size_type size() const noexcept { return size_; }
    size_type capacity() const noexcept { return capacity_; }
    
    void reserve(size_type new_cap) {
        if (new_cap > capacity_) {
            pointer new_data = alloc_traits::allocate(alloc_, new_cap);
            
            if constexpr (std::is_nothrow_move_constructible_v<T>) {
                for (size_type i = 0; i < size_; ++i) {
                    alloc_traits::construct(alloc_, new_data + i, std::move(data_[i]));
                    alloc_traits::destroy(alloc_, data_ + i);
                }
            } else {
                size_type constructed = 0;
                try {
                    for (; constructed < size_; ++constructed) {
                        alloc_traits::construct(alloc_, new_data + constructed, data_[constructed]);
                    }
                    for (size_type i = 0; i < size_; ++i) {
                        alloc_traits::destroy(alloc_, data_ + i);
                    }
                } catch (...) {
                    for (size_type i = 0; i < constructed; ++i) {
                        alloc_traits::destroy(alloc_, new_data + i);
                    }
                    alloc_traits::deallocate(alloc_, new_data, new_cap);
                    throw;
                }
            }
            
            if (data_) {
                alloc_traits::deallocate(alloc_, data_, capacity_);
            }
            
            data_ = new_data;
            capacity_ = new_cap;
        }
    }
    
    void shrink_to_fit() {
        if (size_ < capacity_) {
            if (size_ == 0) {
                destroy_all();
                data_ = nullptr;
                capacity_ = 0;
            } else {
                pointer new_data = alloc_traits::allocate(alloc_, size_);
                
                for (size_type i = 0; i < size_; ++i) {
                    alloc_traits::construct(alloc_, new_data + i, std::move(data_[i]));
                    alloc_traits::destroy(alloc_, data_ + i);
                }
                
                alloc_traits::deallocate(alloc_, data_, capacity_);
                data_ = new_data;
                capacity_ = size_;
            }
        }
    }
    
    void push(const T& value) {
        if (size_ == capacity_) {
            grow();
        }
        alloc_traits::construct(alloc_, data_ + size_, value);
        ++size_;
    }
    
    void push(T&& value) {
        if (size_ == capacity_) {
            grow();
        }
        alloc_traits::construct(alloc_, data_ + size_, std::move(value));
        ++size_;
    }
    
    template<typename... Args>
    reference emplace(Args&&... args) {
        if (size_ == capacity_) {
            grow();
        }
        alloc_traits::construct(alloc_, data_ + size_, std::forward<Args>(args)...);
        return data_[size_++];
    }
    
    void pop() {
        if (empty()) throw std::out_of_range("stack::pop(): stack is empty");
        --size_;
        alloc_traits::destroy(alloc_, data_ + size_);
    }
    
    void clear() noexcept {
        for (size_type i = 0; i < size_; ++i) {
            alloc_traits::destroy(alloc_, data_ + i);
        }
        size_ = 0;
    }
    
    void swap(stack& other) noexcept {
        std::swap(data_, other.data_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
        std::swap(alloc_, other.alloc_);
    }
    
    iterator begin() noexcept { return iterator(data_); }
    const_iterator begin() const noexcept { return const_iterator(data_); }
    const_iterator cbegin() const noexcept { return const_iterator(data_); }
    
    iterator end() noexcept { return iterator(data_ + size_); }
    const_iterator end() const noexcept { return const_iterator(data_ + size_); }
    const_iterator cend() const noexcept { return const_iterator(data_ + size_); }
    
    reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
    const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
    const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(end()); }
    
    reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
    const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
    const_reverse_iterator crend() const noexcept { return const_reverse_iterator(begin()); }
    
    allocator_type get_allocator() const noexcept { return alloc_; }
    
    bool operator==(const stack& other) const {
        return size_ == other.size_ && 
               std::equal(begin(), end(), other.begin());
    }
    
    bool operator!=(const stack& other) const {
        return !(*this == other);
    }
    
    bool operator<(const stack& other) const {
        return std::lexicographical_compare(begin(), end(), 
                                          other.begin(), other.end());
    }
    
    bool operator<=(const stack& other) const {
        return !(other < *this);
    }
    
    bool operator>(const stack& other) const {
        return other < *this;
    }
    
    bool operator>=(const stack& other) const {
        return !(*this < other);
    }
};

template<typename T, typename Alloc>
void swap(stack<T, Alloc>& lhs, stack<T, Alloc>& rhs) noexcept {
    lhs.swap(rhs);
}

template<typename T, typename Alloc>
typename stack<T, Alloc>::iterator operator+(
    typename stack<T, Alloc>::iterator::difference_type n,
    typename stack<T, Alloc>::iterator it) noexcept {
    return it + n;
}

template<typename T, typename Alloc>
typename stack<T, Alloc>::const_iterator operator+(
    typename stack<T, Alloc>::const_iterator::difference_type n,
    typename stack<T, Alloc>::const_iterator it) noexcept {
    return it + n;
}

