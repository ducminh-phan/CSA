#ifndef PARETO_SET_HPP
#define PARETO_SET_HPP

#include <algorithm>
#include <iostream>
#include <tuple>
#include <vector>


template<std::size_t n, class... T>
typename std::enable_if<n >= sizeof...(T)>::type
print_tuple(std::ostream&, const std::tuple<T...>&) {}

template<std::size_t n, class... T>
typename std::enable_if<n < sizeof...(T)>::type
print_tuple(std::ostream& os, const std::tuple<T...>& tup) {
    if (n != 0) {
        os << ", ";
    }
    os << std::get<n>(tup);
    print_tuple<n + 1>(os, tup);
}

template<class... T>
std::ostream& operator<<(std::ostream& os, const std::tuple<T...>& tup) {
    os << "(";
    print_tuple<0>(os, tup);
    os << ")";

    return os;
}

template<std::size_t n, class... T>
typename std::enable_if<n == sizeof...(T), bool>::type all_less(const std::tuple<T...>&,
                                                                const std::tuple<T...>&) {
    return true;
}

template<std::size_t n, class... T>
typename std::enable_if<n < sizeof...(T), bool>::type all_less(const std::tuple<T...>& t1,
                                                               const std::tuple<T...>& t2) {
    // Check if all criteria of t1 is less than or equal to those of t2. We start from
    // the first criterion (n = 0), then recursively check the next criterion.
    // In each step, if any criterion of t1 is larger than that of t2, crit_comp is false,
    // and the function returns false immediately by short-circuit evaluation.
    // When we reach n = sizeof...(T), every criterion of t1 is less than or equal to
    // that of t2, then we return true in that case.

    bool crit_comp {std::get<n>(t1) <= std::get<n>(t2)};
    return crit_comp && all_less<n + 1>(t1, t2);
}

template<std::size_t n, class... T>
typename std::enable_if<n == sizeof...(T), bool>::type any_strictly_less(const std::tuple<T...>&,
                                                                         const std::tuple<T...>&) {
    return false;
}

template<std::size_t n, class... T>
typename std::enable_if<n < sizeof...(T), bool>::type any_strictly_less(const std::tuple<T...>& t1,
                                                                        const std::tuple<T...>& t2) {
    // Check if any criterion of t1 is strictly less than that of t2. The logic is similar
    // to that of all_less. However, we need to check if there is any criterion of t1
    // that is strictly less than that of t2. Thus, when n == sizeof...(T), every individual
    // criterion of t1 is >= that of t2, and we return false in that case.

    bool crit_comp {std::get<n>(t1) < std::get<n>(t2)};
    return crit_comp || any_strictly_less<n + 1>(t1, t2);
}

namespace internal {
    template<std::size_t n, class... T>
    typename std::enable_if<n < sizeof...(T), void>::type increment(
            std::tuple<T...>& tup,
            const typename std::tuple_element<n, std::tuple<T...>>::type& val) {
        std::get<n>(tup) += val;
    }
}

// TODO: explain all the template hacks
template<class... T>
class Element {
private:
    std::tuple<T...> _data;

public:
    explicit Element() = default;

    explicit Element(const T& ... crits) : _data {crits...} {};

    bool dominates(const Element& other) const {
        return all_less<0>(this->_data, other._data) && any_strictly_less<0>(this->_data, other._data);
    }

    friend std::ostream& operator<<(std::ostream& out, const Element& elem) {
        out << elem._data;
        return out;
    }

    friend bool operator==(const Element& e1, const Element& e2) {
        return e1._data == e2._data;
    }

    friend bool operator!=(const Element& e1, const Element& e2) {
        return e1._data != e2._data;
    }

    friend bool operator<(const Element& e1, const Element& e2) {
        return e1._data < e2._data;
    }

    friend Element operator+(const Element& e1, const Element& e2) {
        Element retval;
        retval._data = e1._data + e2._data;

        return retval;
    }

    template<std::size_t n>
    void increment(const typename std::tuple_element<n, std::tuple<T...>>::type& val) {
        internal::increment<n>(this->_data, val);
    }

    template<std::size_t n>
    typename std::tuple_element<n, std::tuple<T...>>::type& get() {
        return std::get<n>(_data);
    }
};


template<class... T>
class ParetoSet {
public:
    using element_t = Element<T...>;

private:
    using container_t = std::vector<element_t>;

    container_t _container;

public:
    ParetoSet() {
        // Reserve a number of elements to avoid extra allocation
        _container.reserve(256);
    }

    typename container_t::iterator begin() {
        return _container.begin();
    }

    typename container_t::iterator end() {
        return _container.end();
    }

    typename container_t::const_iterator begin() const {
        return _container.begin();
    }

    typename container_t::const_iterator end() const {
        return _container.end();
    }

    void insert(const element_t& elem) { // https://stackoverflow.com/a/36282956
        // Add the element to the Pareto set only if the current set does not already contain it,
        // and it is not dominated by any of the current element
        if (std::find(_container.begin(), _container.end(), elem) == _container.end() &&
            std::none_of(_container.begin(), _container.end(), [&](const element_t& e) { return e.dominates(elem); })) {
            // If the new point can be inserted to the Pareto set, we need to remove all the elements
            // that are dominated by the newly inserted point
            _container.erase(
                    std::remove_if(
                            _container.begin(), _container.end(),
                            [&](const element_t& e) { return elem.dominates(e); }
                    ),
                    _container.end()
            );

            // Add the element to the internal container
            _container.push_back(elem);
        }
    }

    void emplace(const T& ... args) {
        element_t elem {args...};
        insert(elem);
    };

    void merge(const ParetoSet& other) {
        // Merge another Pareto set by inserting its elements one-by-one
        for (const auto& elem: other._container) {
            this->insert(elem);
        }
    }

    friend bool operator==(const ParetoSet& set1, const ParetoSet& set2) {
        container_t c1, c2;

        c1 = set1._container;
        c2 = set2._container;

        std::sort(c1.begin(), c1.end());
        std::sort(c2.begin(), c2.end());

        return c1 == c2;
    }

    std::size_t size() const {
        return _container.size();
    }
};

#endif // PARETO_SET_HPP
