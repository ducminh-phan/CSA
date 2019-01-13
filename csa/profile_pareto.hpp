#ifndef PROFILE_PARETO_HPP
#define PROFILE_PARETO_HPP

#include <algorithm>
#include <iterator>
#include <utility>
#include <vector>

#include "data_structure.hpp"


struct Pair {
    Time dep, arr;

    Pair() : dep {INF}, arr {INF} {};

    Pair(Time d, Time a) : dep {d}, arr {a} {};

    inline bool dominates(const Time& d, const Time& a) const {
        return dep >= d && arr <= a;
    }

    inline bool dominates(const Pair& p) const {
        return dominates(p.dep, p.arr);
    }
};


class ProfilePareto {
public:
    using pair_t = Pair;

private:
    std::vector<pair_t> _container;

public:
    ProfilePareto() {
        _container.reserve(256);

        // Initialise the container with a (∞, ∞) pair
        _container.emplace_back();
    }

    void emplace(const Time& dep, const Time& arr, const bool& check = true) {
        emplace({dep, arr}, check);
    }

    void emplace(const pair_t& p, const bool& check = true) {
        if (check && this->dominates(p)) return;

        // TODO: compare the performance of set with sorted vector

        // Find the position to insert the new pair, since the pairs are sorted
        // in the decreasing order of the departure times
        auto iter = std::lower_bound(_container.begin(), _container.end(), p,
                                     [&](const pair_t& p1, const pair_t& p2) {
                                         return p1.dep > p2.dep;
                                     }
        );

        iter = _container.insert(iter, p);

        // Remove the points that are dominated by p, these points appear only after p
        // in the container
        _container.erase(
                std::remove_if(std::next(iter), _container.end(),
                               [&](const pair_t& _p) { return p.dominates(_p); }),
                _container.end()
        );
    }

    // Check if the new pair is dominated by any of the current pair
    bool dominates(const pair_t& p) const {
        // Here we exploit the property that the pairs in the container are in decreasing order
        // in both departure and arrival time. A pair in the container dominates (dep, arr) iff
        // pair.dep >= dep and pair.arr <= arr. Thus we will find the last pair such that
        // pair.dep >= dep, after that pair, pair.dep < dep and it could not dominate (dep, arr).
        // Similarly, we will find the first pair such that pair.arr <= arr. After finding these
        // two bounds, we iterate in the corresponding range only.

        // The iterator to the first pair such that pair.arr <= arr
        const auto& first = std::lower_bound(_container.begin(), _container.end(), p,
                                             [&](const pair_t& p1, const pair_t& p2) { return p1.arr > p2.arr; });

        // The iterator to the first pair such that pair.dep < dep, which is the pair
        // immediately after the last pair such that pair.dep >= dep
        const auto& last = std::lower_bound(_container.begin(), _container.end(), p,
                                            [&](const pair_t& p1, const pair_t& p2) { return p1.dep >= p2.dep; });

        if (std::distance(first, last) <= 0) {
            return false;
        }

        for (auto iter = first; iter != last; ++iter) {
            if (iter->dominates(p)) {
                return true;
            }
        }

        return false;
    }

    std::vector<pair_t>::reverse_iterator rbegin() {
        return _container.rbegin();
    }

    std::vector<pair_t>::reverse_iterator rend() {
        return _container.rend();
    }

    std::vector<pair_t>::const_iterator begin() const {
        return _container.begin();
    }

    std::vector<pair_t>::const_iterator end() const {
        return _container.end();
    }

    std::size_t size() const {
        return _container.size();
    }
};

#endif // PROFILE_PARETO_HPP
