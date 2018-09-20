#ifndef CSA_HPP
#define CSA_HPP

#include <vector>

#include "data_structure.hpp"
#include "bag.hpp"

class ConnectionScan {
private:
    const Timetable* const _timetable;
    std::vector<ParetoSet> bags;
    std::vector<Time> earliest_arrival_time;
    std::vector<bool> is_reached;

public:
    explicit ConnectionScan(const Timetable* timetable_p) : _timetable {timetable_p} {};

    Time query(const node_id_t& source_id, const node_id_t& target_id, const Time& departure_time);

    Time backward_query(const node_id_t& source_id, const node_id_t& target_id,
                        const Time& arrival_time) const;

    void init();

    void clear();
};

#endif // CSA_HPP
