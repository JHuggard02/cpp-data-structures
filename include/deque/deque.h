#include <memory>
#include <iostream>
#include <iterator>

template <typename T, typename Alloc = std::allocator<T>>
class deque {
public:
    using value_type = T;
    using pointer = T*;
    using reference = T&;

    using traits = std::allocator_traits<Alloc>;
    using segment_alloc = typename traits::template rebind_alloc<T>;
    using map_alloc = typename traits::template rebind_alloc<pointer>;

    static constexpr size_t block_size =
        (sizeof(T) < 256) ? 4096 / sizeof(T) : 16;

    
    template <typename U, typename Pointer, typename Reference>
    struct deque_iterator {
        using iterator_category = std::random_access_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = U;
        using pointer = Pointer;
        using reference = Reference;

        pointer curr = nullptr;
        pointer first = nullptr;
        pointer last = nullptr;
        pointer* node = nullptr;

        reference operator*() const { return *curr; }
        pointer operator->() const { return curr; }

        deque_iterator& operator++() {
            ++curr;
            if (curr == last) {   
                set_node(node + 1);
                curr = first;
            }
            return *this;
        }

        deque_iterator& operator--() {
            if (curr == first) { 
                set_node(node - 1);
                curr = last;
            }
            --curr;
            return *this;
        }

        bool operator==(const deque_iterator& other) const {
            return curr == other.curr;
        }
        bool operator!=(const deque_iterator& other) const {
            return !(*this == other);
        }

    private:
        void set_node(pointer* new_node) {
            node = new_node;
            first = *new_node;
            last = first + block_size;
        }

        friend class deque;
    };

private:
    pointer* map_;                 
    size_t map_size_;              
    size_t num_segments_;          

    deque_iterator<T, pointer, reference> start_;
    deque_iterator<T, pointer, reference> finish_;

    segment_alloc seg_alloc_;
    map_alloc map_alloc_;

public:
    deque()
        : map_(nullptr), map_size_(0), num_segments_(0) {
        reserve_map(8); 
        map_[0] = allocate_segment();
        start_.set_node(map_);
        start_.curr = start_.first;
        finish_ = start_;
    }

    ~deque() {
        clear();
        if (map_) {
            map_alloc_.deallocate(map_, map_size_);
        }
    }

    void push_back(const T& value) {
        if (finish_.curr == finish_.last) {
            if (num_segments_ + 1 >= map_size_) {
                resize_map(map_size_ * 2);
            }
            map_[num_segments_] = allocate_segment();
            finish_.set_node(map_ + num_segments_);
            finish_.curr = finish_.first;
            ++num_segments_;
        }
        traits::construct(seg_alloc_, finish_.curr, value);
        ++finish_;
    }

    void push_front(const T& value) {
        if (start_.curr == start_.first) {
            if (num_segments_ + 1 >= map_size_) {
                resize_map(map_size_ * 2);
            }
            for (size_t i = num_segments_; i > 0; --i) {
                map_[i] = map_[i - 1];
            }
            map_[0] = allocate_segment();
            ++num_segments_;
            start_.set_node(map_);
            start_.curr = start_.last;
        }
        --start_;
        traits::construct(seg_alloc_, start_.curr, value);
    }

    void print_debug() {
        auto it = start_;
        while (it != finish_) {
            std::cout << *it << " ";
            ++it;
        }
        std::cout << "\n";
    }

private:
    pointer allocate_segment() {
        return traits::allocate(seg_alloc_, block_size);
    }

    void reserve_map(size_t n) {
        map_ = traits::allocate(map_alloc_, n);
        map_size_ = n;
    }

    void resize_map(size_t new_size) {
        pointer* new_map = traits::allocate(map_alloc_, new_size);
        for (size_t i = 0; i < num_segments_; ++i) {
            new_map[i] = map_[i];
        }
        map_alloc_.deallocate(map_, map_size_);
        map_ = new_map;
        map_size_ = new_size;
    }

    void clear() {
        for (size_t i = 0; i < num_segments_; ++i) {
            traits::deallocate(seg_alloc_, map_[i], block_size);
        }
        num_segments_ = 0;
    }
};

