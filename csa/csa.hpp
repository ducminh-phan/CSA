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

    void update_departure_stop(const node_id_t& dep_id);

    void update_out_hubs(const node_id_t& arr_id, const Time& arrival_time, const node_id_t& target_id);

public:
    explicit ConnectionScan(const Timetable* timetable_p) : _timetable {timetable_p} {};

    Time
    query(const node_id_t& source_id, const node_id_t& target_id, const Time& departure_time,
          const bool& target_pruning = true);

    ProfilePareto profile_query(const node_id_t& source_id, const node_id_t& target_id);

    void init();

    void clear();
};

#endif // CSA_HPP
