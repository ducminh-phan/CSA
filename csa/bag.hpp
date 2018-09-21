#ifndef BAG_HPP
#define BAG_HPP

#include <algorithm>
#include <vector>

#include "data_structure.hpp"
#include "utilities.hpp"


struct Element {
    Time arrival_time;
    std::size_t no_transfers = 0;
    Time walking_time;

    explicit Element() = default;

    Element(const Time& a, const std::size_t& t, const Time& w) :
            arrival_time {a}, no_transfers {t}, walking_time {w} {}

    friend bool operator==(const Element& e1, const Element& e2) {
        return e1.arrival_time == e2.arrival_time && e1.no_transfers == e2.no_transfers &&
               e1.walking_time == e2.walking_time;
    }

    friend bool operator<(const Element& e1, const Element& e2) {
        return (e1.arrival_time < e2.arrival_time) ||
               (e1.arrival_time == e2.arrival_time && e1.no_transfers < e2.no_transfers) ||
               (e1.arrival_time == e2.arrival_time && e1.no_transfers == e2.no_transfers &&
                e1.walking_time < e2.walking_time);
    }

    friend Element operator+(const Element& e1, const Element& e2) {
        return {e1.arrival_time + e2.arrival_time, e1.no_transfers + e2.no_transfers,
                e1.walking_time + e2.walking_time};
    }

    bool dominates(const Element& other) const {
        return arrival_time <= other.arrival_time && no_transfers <= other.no_transfers &&
               walking_time <= other.walking_time &&
               (arrival_time < other.arrival_time || no_transfers < other.no_transfers ||
                walking_time < other.walking_time);
    }
};


class ParetoSet {
private:
    using container_t = std::vector<Element>;

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

    void insert(const Element& elem) { // https://stackoverflow.com/a/36282956
        #ifdef PROFILE
        Profiler prof {__func__};
        #endif

        // Add the element to the Pareto set only if the current set does not already contain it,
        // and it is not dominated by any of the current element
        if (std::none_of(_container.begin(), _container.end(),
                         [&](const Element& e) { return e.dominates(elem) || e == elem; })) {
            // If the new point can be inserted to the Pareto set, we need to remove all the elements
            // that are dominated by the newly inserted point
            _container.erase(
                    std::remove_if(
                            _container.begin(), _container.end(),
                            [&](const Element& e) { return elem.dominates(e); }
                    ),
                    _container.end()
            );

            // Add the element to the internal container
            _container.push_back(elem);
        }
    }

    void emplace(const Time& a, const std::size_t& t, const Time& w) {
        Element elem {a, t, w};
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

#endif // BAG_HPP
