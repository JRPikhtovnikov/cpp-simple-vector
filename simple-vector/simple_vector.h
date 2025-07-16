#pragma once

#include <cassert>
#include <initializer_list>
#include <stdexcept>
#include <utility>
#include "array_ptr.h"

struct ReserveProxyObj {
    explicit ReserveProxyObj(size_t capacity) : capacity(capacity) {}
    size_t capacity;
};

inline ReserveProxyObj Reserve(size_t capacity_to_reserve) {
    return ReserveProxyObj(capacity_to_reserve);
}

template <typename Type>
class SimpleVector {
public:
    using Iterator = Type*;
    using ConstIterator = const Type*;

    SimpleVector() noexcept = default;

    explicit SimpleVector(ReserveProxyObj proxy)
            : capacity_(proxy.capacity),
              items_(proxy.capacity > 0 ? new Type[proxy.capacity] : nullptr) {}

    explicit SimpleVector(size_t size) : size_(size), capacity_(size), items_(size) {
        for (size_t i = 0; i < size; ++i) {
            items_[i] = Type();
        }
    }

    SimpleVector(size_t size, const Type& value) : size_(size), capacity_(size), items_(size) {
        for (size_t i = 0; i < size; ++i) {
            items_[i] = value;
        }
    }

    SimpleVector(std::initializer_list<Type> init)
            : size_(init.size()), capacity_(init.size()), items_(init.size()) {
        size_t i = 0;
        for (const auto& item : init) {
            items_[i++] = item;
        }
    }

    SimpleVector(const SimpleVector& other)
            : size_(other.size_), capacity_(other.size_), items_(other.size_) {
        for (size_t i = 0; i < size_; ++i) {
            items_[i] = other.items_[i];
        }
    }

    SimpleVector(SimpleVector&& other) noexcept
            : size_(other.size_), capacity_(other.capacity_), items_(std::move(other.items_)) {
        other.size_ = 0;
        other.capacity_ = 0;
    }

    ~SimpleVector() = default;

    void Reserve(size_t new_capacity) {
        if (new_capacity <= capacity_) {
            return;
        }

        ArrayPtr<Type> new_items(new_capacity);
        for (size_t i = 0; i < size_; ++i) {
            new_items[i] = std::move(items_[i]);
        }

        items_.swap(new_items);
        capacity_ = new_capacity;
    }

    size_t GetSize() const noexcept { return size_; }
    size_t GetCapacity() const noexcept { return capacity_; }
    bool IsEmpty() const noexcept { return size_ == 0; }

    Type& operator[](size_t index) noexcept {
        assert(index < size_);
        return items_[index];
    }

    const Type& operator[](size_t index) const noexcept {
        assert(index < size_);
        return items_[index];
    }

    Type& At(size_t index) {
        if (index >= size_) {
            throw std::out_of_range("Index out of range");
        }
        return items_[index];
    }

    const Type& At(size_t index) const {
        if (index >= size_) {
            throw std::out_of_range("Index out of range");
        }
        return items_[index];
    }

    void Clear() noexcept { size_ = 0; }

    void Resize(size_t new_size) {
        if (new_size <= size_) {
            size_ = new_size;
            return;
        }

        if (new_size <= capacity_) {
            for (size_t i = size_; i < new_size; ++i) {
                items_[i] = Type();
            }
            size_ = new_size;
            return;
        }

        ArrayPtr<Type> new_items(new_size);
        for (size_t i = 0; i < size_; ++i) {
            new_items[i] = std::move(items_[i]);
        }
        for (size_t i = size_; i < new_size; ++i) {
            new_items[i] = Type();
        }

        items_.swap(new_items);
        size_ = new_size;
        capacity_ = new_size;
    }

    Iterator begin() noexcept { return items_.Get(); }
    Iterator end() noexcept { return items_.Get() + size_; }
    ConstIterator begin() const noexcept { return items_.Get(); }
    ConstIterator end() const noexcept { return items_.Get() + size_; }
    ConstIterator cbegin() const noexcept { return items_.Get(); }
    ConstIterator cend() const noexcept { return items_.Get() + size_; }

    SimpleVector& operator=(const SimpleVector& rhs) {
        if (this != &rhs) {
            SimpleVector temp(rhs);
            swap(temp);
        }
        return *this;
    }

    SimpleVector& operator=(SimpleVector&& rhs) noexcept {
        if (this != &rhs) {
            items_ = std::move(rhs.items_);
            size_ = rhs.size_;
            capacity_ = rhs.capacity_;
            rhs.size_ = 0;
            rhs.capacity_ = 0;
        }
        return *this;
    }

    void PushBack(const Type& item) {
        EmplaceBack(item);
    }

    void PushBack(Type&& item) {
        EmplaceBack(std::move(item));
    }

    template <typename... Args>
    Type& EmplaceBack(Args&&... args) {
        if (size_ >= capacity_) {
            size_t new_capacity = capacity_ == 0 ? 1 : capacity_ * 2;
            ArrayPtr<Type> new_items(new_capacity);
            for (size_t i = 0; i < size_; ++i) {
                new_items[i] = std::move(items_[i]);
            }
            items_.swap(new_items);
            capacity_ = new_capacity;
        }
        items_[size_] = Type(std::forward<Args>(args)...);
        return items_[size_++];
    }

    Iterator Insert(ConstIterator pos, const Type& value) {
        return Emplace(pos, value);
    }

    Iterator Insert(ConstIterator pos, Type&& value) {
        return Emplace(pos, std::move(value));
    }

    template <typename... Args>
    Iterator Emplace(ConstIterator pos, Args&&... args) {
        size_t offset = pos - begin();
        if (size_ >= capacity_) {
            size_t new_capacity = capacity_ == 0 ? 1 : capacity_ * 2;
            ArrayPtr<Type> new_items(new_capacity);
            for (size_t i = 0; i < offset; ++i) {
                new_items[i] = std::move(items_[i]);
            }
            new_items[offset] = Type(std::forward<Args>(args)...);
            for (size_t i = offset; i < size_; ++i) {
                new_items[i + 1] = std::move(items_[i]);
            }
            items_.swap(new_items);
            capacity_ = new_capacity;
        } else {
            for (size_t i = size_; i > offset; --i) {
                items_[i] = std::move(items_[i - 1]);
            }
            items_[offset] = Type(std::forward<Args>(args)...);
        }
        ++size_;
        return begin() + offset;
    }

    void PopBack() noexcept {
        if (size_ > 0) {
            --size_;
        }
    }

    Iterator Erase(ConstIterator pos) {
        size_t offset = pos - begin();
        for (size_t i = offset; i < size_ - 1; ++i) {
            items_[i] = std::move(items_[i + 1]);
        }
        --size_;
        return begin() + offset;
    }

    void swap(SimpleVector& other) noexcept {
        items_.swap(other.items_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
    }

private:
    size_t size_ = 0;
    size_t capacity_ = 0;
    ArrayPtr<Type> items_;
};

template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    if (lhs.GetSize() != rhs.GetSize()) return false;
    for (size_t i = 0; i < lhs.GetSize(); ++i) {
        if (lhs[i] != rhs[i]) return false;
    }
    return true;
}

template <typename Type>
inline bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs == rhs);
}

template <typename Type>
inline bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::lexicographical_compare(
            lhs.begin(), lhs.end(),
            rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(rhs < lhs);
}

template <typename Type>
inline bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return rhs < lhs;
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs < rhs);
}
