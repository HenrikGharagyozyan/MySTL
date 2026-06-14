#include "mystl/string.hpp"

#include <cstring> // for std::memcpy

namespace mystl
{

    // Default constructor
    String::String() noexcept
        : size_(0)
        , capacity_(SSO_CAPACITY)
    {
        sso_data_[0] = '\0';
    }

    // From C-string
    String::String(const char* s)
    {
        size_ = internal_strlen(s);
        if (size_ <= SSO_CAPACITY)
        {
            capacity_ = SSO_CAPACITY;
            for (size_type i = 0; i < size_; ++i)
                sso_data_[i] = s[i];

            sso_data_[size_] = '\0';
        }
        else
        {
            capacity_ = size_;
            heap_data_ = alloc_.allocate(capacity_ + 1);
            for (size_type i = 0; i < size_; ++i)
                heap_data_[i] = s[i];

            heap_data_[size_] = '\0';
        }
    }

    // Fill constructor
    String::String(size_type count, char ch)
        : size_(count)
    {
        if (size_ <= SSO_CAPACITY)
        {
            capacity_ = SSO_CAPACITY;
            for (size_type i = 0; i < size_; ++i)
                sso_data_[i] = ch;

            sso_data_[size_] = '\0';
        }
        else
        {
            capacity_ = size_;
            heap_data_ = alloc_.allocate(capacity_ + 1);
            for (size_type i = 0; i < size_; ++i)
                heap_data_[i] = ch;

            heap_data_[size_] = '\0';
        }
    }

    // Copy constructor
    String::String(const String& other)
        : size_(other.size_)
        , capacity_(other.capacity_)
    {
        if (other.is_sso())
        {
            for (size_type i = 0; i <= size_; ++i) // Copy including the terminating null character
                sso_data_[i] = other.sso_data_[i];
        }
        else
        {
            heap_data_ = alloc_.allocate(capacity_ + 1);
            for (size_type i = 0; i <= size_; ++i)
                heap_data_[i] = other.heap_data_[i];
        }
    }

    // Move constructor
    String::String(String&& other) noexcept
        : size_(other.size_)
        , capacity_(other.capacity_)
    {
        if (other.is_sso())
        {
            for (size_type i = 0; i <= size_; ++i)
                sso_data_[i] = other.sso_data_[i];
        }
        else
            heap_data_ = other.heap_data_;

        // Bring the stolen object into a valid default SSO state
        other.size_ = 0;
        other.capacity_ = SSO_CAPACITY;
        other.sso_data_[0] = '\0';
    }

    // Destructor
    String::~String()
    {
        if (!is_sso() && heap_data_)
            alloc_.deallocate(heap_data_, capacity_ + 1);
    }

    // Copy Assignment
    String& String::operator=(const String& other)
    {
        if (this == &other)
            return *this;

        // Free the old heap allocation if it existed
        if (!is_sso() && heap_data_)
            alloc_.deallocate(heap_data_, capacity_ + 1);

        size_ = other.size_;
        capacity_ = other.capacity_;

        if (other.is_sso())
        {
            for (size_type i = 0; i <= size_; ++i)
                sso_data_[i] = other.sso_data_[i];
        }
        else
        {
            heap_data_ = alloc_.allocate(capacity_ + 1);
            for (size_type i = 0; i <= size_; ++i)
                heap_data_[i] = other.heap_data_[i];
        }
        return *this;
    }

    // Move Assignment
    String& String::operator=(String&& other) noexcept
    {
        if (this == &other)
            return *this;

        if (!is_sso() && heap_data_)
            alloc_.deallocate(heap_data_, capacity_ + 1);

        size_ = other.size_;
        capacity_ = other.capacity_;

        if (other.is_sso())
        {
            for (size_type i = 0; i <= size_; ++i)
                sso_data_[i] = other.sso_data_[i];
        }
        else
            heap_data_ = other.heap_data_;

        other.size_ = 0;
        other.capacity_ = SSO_CAPACITY;
        other.sso_data_[0] = '\0';

        return *this;
    }

    void String::clear() noexcept
    {
        size_ = 0;
        data_ptr()[0] = '\0';
    }

    void String::reserve(size_type new_capacity)
    {
        // Never reduce capacity via reserve
        if (new_capacity <= capacity_)
            return;

        pointer new_data = alloc_.allocate(new_capacity + 1);

        pointer old_data = data_ptr();

        std::memcpy(new_data, old_data, size_ + 1);

        if (!is_sso())
            alloc_.deallocate(heap_data_, capacity_ + 1);

        heap_data_ = new_data;
        capacity_ = new_capacity;
    }

    void String::push_back(char ch)
    {
        if (size_ == capacity_)
            reserve(capacity_ == 0 ? SSO_CAPACITY : capacity_ * 2);

        pointer data = data_ptr();

        data[size_] = ch;
        data[size_ + 1] = '\0';

        ++size_;
    }

    int String::compare(const String& other) const noexcept
    {
        size_type min_size = (size_ < other.size_) ? size_ : other.size_;
        const char* data1 = data_ptr();
        const char* data2 = other.data_ptr();

        for (size_type i = 0; i < min_size; ++i)
        {
            if (data1[i] < data2[i])
                return -1;
            if (data1[i] > data2[i])
                return 1;
        }

        if (size_ < other.size_)
            return -1;
        if (size_ > other.size_)
            return 1;
        return 0;
    }

    bool operator==(const String& lhs, const String& rhs) noexcept
    {
        if (lhs.size_ != rhs.size_)
            return false;
        return lhs.compare(rhs) == 0;
    }

    bool operator!=(const String& lhs, const String& rhs) noexcept
    {
        return !(lhs == rhs);
    }

    bool operator==(const String& lhs, const char* rhs) noexcept
    {
        if (!rhs)
            return false;
        const char *data = lhs.data_ptr();
        String::size_type i = 0;
        // Compare characters until we hit the end of our string or the C-string
        for (; i < lhs.size_ && rhs[i]; ++i)
        {
            if (data[i] != rhs[i])
                return false;
        }
        // Both must reach their respective ends at the same time
        return i == lhs.size_ && rhs[i] == '\0';
    }

    std::ostream &operator<<(std::ostream &out, const String &str)
    {
        out << str.c_str();
        return out;
    }

} // namespace mystl