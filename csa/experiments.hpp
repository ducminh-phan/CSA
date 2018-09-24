#ifndef EXPERIMENTS_HPP
#define EXPERIMENTS_HPP

#include <utility> // std::move
#include <vector>

#include "data_structure.hpp"


struct Query {
    uint16_t rank;
    node_id_t source_id;
    node_id_t target_id;
    Time dep;

    Query(uint16_t r, node_id_t s, node_id_t t, Time::value_type d) :
            rank {r}, source_id {s}, target_id {t}, dep {d} {};
};


using Queries = std::vector<Query>;


struct Result {
    uint16_t rank;
    double running_time;
    Time arrival_time;
    std::size_t size;

    Result() : rank {}, running_time {}, arrival_time {}, size {} {};

    Result(uint16_t r, double rt, Time a, std::size_t s) :
            rank {r}, running_time {rt}, arrival_time {a}, size {s} {};
};


using Results = std::vector<Result>;


void write_results(const Results& results, const std::string& name, bool use_hl);


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
