#if !defined(OPTIONAL_H)
#define OPTIONAL_H

#include <cassert>

template <typename T>
class optional
{
public:
    optional()
        : m_value()
        , m_condition(false)
    {}

    explicit optional(T v)
        : m_value(std::move(v))
        , m_condition(true)
    {}

    optional(bool cond, T v)
        : m_value(std::move(v))
        , m_condition(cond)
    {}

    optional(const optional& other)
        : m_value(other.m_value)
        , m_condition(other.m_condition)
    {
    }

    optional(optional&& other)
        : m_value(std::move(other.m_value))
        , m_condition(other.m_condition)
    {
    }

    const optional& operator=(const optional& other)
    {
        m_value = other.m_value;
        m_condition = other.m_condition;
        return *this;
    }

    const optional& operator=(optional && other)
    {
        std::swap(m_value, other.m_value);
        std::swap(m_condition, other.m_condition);
        return *this;
    }

    explicit operator bool() const
    {
        return m_condition;
    }

    const T& get() const
    {
        assert(m_condition);
        return m_value;
    }

    T& get()
    {
        assert(m_condition);
        return m_value;
    }

private:
    T m_value;
    bool m_condition;
};

template <typename T>
inline optional<T> make_optional(const T& v)
{
    return optional<T>(v);
}

template <typename T>
inline optional<T> make_optional(bool cond, const T& v)
{
    return optional<T>(cond, v);
}


#endif // OPTIONAL_H