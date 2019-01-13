#ifndef EXPERIMENTS_HPP
#define EXPERIMENTS_HPP

#include <utility> // std::move
#include <vector>

#include "data_structure.hpp"


struct Query {
    uint16_t rank;
    NodeID source_id;
    NodeID target_id;
    Time dep;

    Query(uint16_t r, NodeID s, NodeID t, Time d) :
            rank {r}, source_id {s}, target_id {t}, dep {d} {};
};


using Queries = std::vector<Query>;


struct Result {
    uint16_t rank;
    double running_time;
    Time arrival_time;
    std::size_t n_journey;

    Result() : rank {}, running_time {}, arrival_time {}, n_journey {} {};

    Result(uint16_t r, double rt, Time a, std::size_t n) :
            rank {r}, running_time {rt}, arrival_time {a}, n_journey {n} {};
};


using Results = std::vector<Result>;


class Experiment {
private:
    const Timetable _timetable;
    const Queries _queries;

    Queries read_queries();

public:
    Experiment() : _timetable {}, _queries {read_queries()} {
        _timetable.summary();
    }

    void run() const;
};

#endif // EXPERIMENTS_HPP
