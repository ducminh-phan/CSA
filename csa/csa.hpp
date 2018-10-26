#ifndef CSA_HPP
#define CSA_HPP

#include <vector>

#include "data_structure.hpp"
#include "profile_pareto.hpp"

class ConnectionScan {
private:
    const Timetable* const _timetable;
    std::vector<Time> earliest_arrival_time;
    std::vector<bool> is_reached;
    std::vector<ProfilePareto> stop_profile;
    std::vector<Time> trip_earliest_time;
    std::vector<Time> walking_time_to_target;

    void update_using_in_hubs(const NodeID& dep_id);

    void update_out_hubs(const NodeID& arr_id, const Time& arrival_time, const NodeID& target_id);

    Time arrival_time_when_transfer(const Connection& connection);

public:
    explicit ConnectionScan(const Timetable* timetable_p) : _timetable {timetable_p} {};

    Time
    query(const NodeID& source_id, const NodeID& target_id, const Time& departure_time,
          const bool& target_pruning = true);

    ProfilePareto profile_query(const NodeID& source_id, const NodeID& target_id);

    void init();

    void clear();
};

#endif // CSA_HPP
