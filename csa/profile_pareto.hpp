#ifndef PROFILE_PARETO_HPP
#define PROFILE_PARETO_HPP

#include <algorithm>
#include <cassert>
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

    void emplace(const Time& dep, const Time& arr) {
        if (this->dominates(dep, arr)) return;

        // TODO: compare the performance of set with sorted vector

        // Find the position to insert the new pair, since the pairs are sorted
        // in the decreasing order of the departure times
        auto p = pair_t {dep, arr};

        auto iter = std::lower_bound(_container.begin(), _container.end(), p,
                                     [&](const Pair& p1, const Pair& p2) {
                                         return p1.dep > p2.dep;
                                     }
        );

        iter = _container.insert(iter, {dep, arr});

        // Remove the points that are dominated by p, these points appear only after p
        // in the container
        _container.erase(
                std::remove_if(std::next(iter), _container.end(),
                               [&](const Pair& _p) { return p.dominates(_p); }),
                _container.end()
        );
    }

    // Check if the new pair is dominated by any of the current pair
    bool dominates(const Time& dep, const Time& arr) {
        for (const auto& pair: _container) {
            if (pair.dominates(dep, arr)) {
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
};

#endif // PROFILE_PARETO_HPP
