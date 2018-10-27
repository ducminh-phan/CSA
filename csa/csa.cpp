#include <algorithm>
#include <vector>

#include "config.hpp"
#include "csa.hpp"


Time ConnectionScan::query(const NodeID& source_id, const NodeID& target_id,
                           const Time& departure_time, const bool& target_pruning) {
    Time tmp_time;

    #ifdef PROFILE
    Profiler prof {__func__};
    #endif

    // Walk from the source to all of its neighbours
    if (!use_hl) {
        for (const auto& transfer: _timetable->stops[source_id].transfers) {
            earliest_arrival_time[transfer.target_id] = departure_time + transfer.time;
        }
    } else {
        // Propagate the departure time from the source stop to all its out-hubs
        for (const auto& hub_link: _timetable->stops[source_id].out_hubs) {
            const auto& walking_time = hub_link.time;
            const auto& hub_id = hub_link.hub_id;

            tmp_time = departure_time + walking_time;
            earliest_arrival_time[hub_id] = tmp_time;
        }

        for (const auto& stop: _timetable->stops) {
            const auto& stop_id = stop.id;

            for (const auto& hub_link: stop.in_hubs) {
                const auto& walking_time = hub_link.time;
                const auto& hub_id = hub_link.hub_id;

                earliest_arrival_time[stop_id] = std::min(
                        earliest_arrival_time[stop_id],
                        earliest_arrival_time[hub_id] + walking_time
                );
            }
        }
    }

    // Find the first connection departing not before departure_time,
    // first we need to create the "dummy" connection, then use std::lower_bound
    // to get the const_iterator pointing to the first connection in the set of connections
    // which is equivalent or goes after this dummy connection. Since the connections are ordered
    // lexicographically, with the order of attribute:
    // departure_time -> arrival_time -> trip_id -> the order of the connection in the trip,
    // the const_iterator obtained will point to the connection we need to find
    Connection dummy_conn {0, 0, 0, departure_time, departure_time, 0};
    const auto& first_conn_iter = std::lower_bound(_timetable->connections.begin(), _timetable->connections.end(),
                                                   dummy_conn);

    for (auto iter = first_conn_iter; iter != _timetable->connections.end(); ++iter) {
        #ifdef PROFILE
        Profiler loop {"Loop"};
        #endif

        const Connection& conn = *iter;
        const auto& arr_id = conn.arrival_stop_id;
        const auto& dep_id = conn.departure_stop_id;

        if (target_pruning && earliest_arrival_time[target_id] <= conn.departure_time) {
            // We need to check if earliest_arrival_time[target_id] can still be improved
            // before break out of the loop
            if (use_hl) {
                update_using_in_hubs(target_id);
            }

            break;
        }

        if (!is_reached[conn.trip_id] && use_hl) {
            update_using_in_hubs(dep_id);
        }

        // Check if the trip containing the connection has been reached,
        // or we can get to the connection's departure stop before its departure
        if (is_reached[conn.trip_id] || earliest_arrival_time[dep_id] <= conn.departure_time) {
            // Mark the trip containing the connection as reached
            is_reached[conn.trip_id] = true;

            // Check if the arrival time to the arrival stop of the connection can be improved
            if (conn.arrival_time < earliest_arrival_time[arr_id]) {
                earliest_arrival_time[arr_id] = conn.arrival_time;

                update_out_hubs(arr_id, conn.arrival_time, target_id);
            }
        }
    }

    return earliest_arrival_time[target_id];
}


void ConnectionScan::init() {
    earliest_arrival_time.assign(_timetable->max_node_id + 1, INF);
    is_reached.resize(_timetable->max_trip_id + 1);

    stop_profile.resize(_timetable->max_node_id + 1);
    trip_earliest_time.assign(_timetable->max_trip_id + 1, INF);
    walking_time_to_target.assign(_timetable->max_node_id + 1, INF);
}


void ConnectionScan::clear() {
    earliest_arrival_time.clear();
    is_reached.clear();

    stop_profile.clear();
    trip_earliest_time.clear();
    walking_time_to_target.clear();
}


void ConnectionScan::update_using_in_hubs(const NodeID& dep_id) {
    // Update the earliest arrival time of the departure stop of the connection
    // or the target stop using its in-hubs

    #ifdef PROFILE
    Profiler prof {__func__};
    #endif

    Time tmp_time;

    for (const auto& hub_link: _timetable->stops[dep_id].in_hubs) {
        const auto& walking_time = hub_link.time;
        const auto& hub_id = hub_link.hub_id;

        tmp_time = earliest_arrival_time[hub_id] + walking_time;

        // We cannot use early stopping here since earliest_arrival_time[hub_id] is not
        // a constant, thus tmp_time is not increasing

        if (tmp_time < earliest_arrival_time[dep_id]) {
            earliest_arrival_time[dep_id] = tmp_time;
        }
    }
}


void ConnectionScan::update_out_hubs(const NodeID& arr_id, const Time& arrival_time,
                                     const NodeID& target_id) {
    #ifdef PROFILE
    Profiler prof {__func__};
    #endif

    const Stop& arrival_stop = _timetable->stops[arr_id];

    Time tmp_time;

    if (!use_hl) {
        // Update the earliest arrival time of the out-neighbours of the arrival stop
        for (const auto& transfer: arrival_stop.transfers) {
            // Compute the arrival time at the destination of the transfer
            tmp_time = arrival_time + transfer.time;

            // Since the transfers are sorted in the increasing order of walking time,
            // we can skip the scanning of the transfers as soon as the arrival time
            // of the destination is later than that of the target stop
            if (tmp_time > earliest_arrival_time[target_id]) break;

            if (tmp_time < earliest_arrival_time[transfer.target_id]) {
                earliest_arrival_time[transfer.target_id] = tmp_time;
            }
        }
    } else {
        // Update the earliest arrival time of the out-hubs of the arrival stop
        for (const auto& hub_link: arrival_stop.out_hubs) {
            const auto& walking_time = hub_link.time;
            const auto& hub_id = hub_link.hub_id;

            tmp_time = arrival_time + walking_time;

            if (tmp_time > earliest_arrival_time[target_id]) break;

            if (tmp_time < earliest_arrival_time[hub_id]) {
                earliest_arrival_time[hub_id] = tmp_time;
            }
        }
    }
}


ProfilePareto ConnectionScan::profile_query(const NodeID& source_id,
                                            const NodeID& target_id) {
    // Run a normal query with departure_time 0 and do not target-prune to scan all connections
    query(source_id, target_id, 0, false);

    // Handle final footpaths
    if (!use_hl) {
        for (const auto& transfer: _timetable->stops[target_id].backward_transfers) {
            walking_time_to_target[transfer.target_id] = transfer.time;
        }
    } else {
        for (const auto& hub_link: _timetable->stops[target_id].in_hubs) {
            const auto& walking_time = hub_link.time;
            const auto& hub_id = hub_link.hub_id;

            walking_time_to_target[hub_id] = walking_time;
        }

        for (const auto& stop: _timetable->stops) {
            for (const auto& hub_link: stop.out_hubs) {
                const auto& walking_time = hub_link.time;
                const auto& hub_id = hub_link.hub_id;

                walking_time_to_target[stop.id] = std::min(walking_time_to_target[stop.id],
                                                           walking_time_to_target[hub_id] + walking_time);
            }
        }
    }

    const auto& first = _timetable->connections.rbegin();
    const auto& last = _timetable->connections.rend();

    Time t1, t2, t3, t3h, t_conn;

    // Iterate over the connection in the decreasing order by departure time
    for (auto conn_iter = first; conn_iter != last; ++conn_iter) {
        // Skip the connection if its trip was not reached during the normal query
        if (!is_reached[conn_iter->trip_id]) {
            continue;
        }

        // Arrival time when walking from the arrival stop to the target
        t1 = conn_iter->arrival_time + walking_time_to_target[conn_iter->arrival_stop_id];

        // Arrival time when remaining seated on the trip of the current connection
        t2 = trip_earliest_time[conn_iter->trip_id];

        // Arrival time when transferring
        t3 = arrival_time_from_node(conn_iter->arrival_stop_id, conn_iter->arrival_time);

        // Arrival time when walking to the out-hubs
        if (use_hl) {
            for (const auto& hub_link: _timetable->stops[conn_iter->arrival_stop_id].out_hubs) {
                // When walking from the arrival stop to the out-hub h, we arrive at h
                // at time conn_iter->arrival_time + hub_link.time
                t3h = arrival_time_from_node(hub_link.hub_id,
                                             conn_iter->arrival_time + hub_link.time);
                t3 = std::min(t3, t3h);
            }
        }

        // Arrival time when starting with the current connection
        t_conn = std::min({t1, t2, t3});

        ProfilePareto::pair_t conn_pair {conn_iter->departure_time, t_conn};

        // Source domination
        if (stop_profile[source_id].dominates(conn_pair)) {
            continue;
        }

        // Handle transfers and initial footpaths
        if (!stop_profile[conn_iter->departure_stop_id].dominates(conn_pair)) {
            // We do not need to check if conn_pair is dominated again
            stop_profile[conn_iter->departure_stop_id].emplace(conn_pair, false);

            if (!use_hl) {
                for (const auto& transfer: _timetable->stops[conn_iter->departure_stop_id].backward_transfers) {
                    stop_profile[transfer.target_id].emplace(conn_iter->departure_time - transfer.time, t_conn);
                }
            } else {
                for (const auto& hub_link: _timetable->stops[conn_iter->departure_stop_id].in_hubs) {
                    stop_profile[hub_link.hub_id].emplace(conn_iter->departure_time - hub_link.time, t_conn);
                }
            }
        }

        trip_earliest_time[conn_iter->trip_id] = t_conn;
    }

    return stop_profile[source_id];
}


// Arrival time at the target when we start from any node at a given arrival time at the node.
// Since in the profile, both the departure time and arrival time are in decreasing order,
// we only need to find the last pair with departure time at least the given arrival_time
// TODO: compare the performance of linear search and binary search
Time ConnectionScan::arrival_time_from_node(const NodeID& node_id, const Time& arrival_time) {
    ProfilePareto::pair_t p;

    auto first = stop_profile[node_id].rbegin();
    auto last = stop_profile[node_id].rend();

    for (auto iter = first; iter != last; ++iter) {
        // We are iterating in the reverse order, starting from the back of the profile vector,
        // thus the first profile pair with departure time >= the arrival time of the connection
        // will be the pair we need
        if (iter->dep >= arrival_time) {
            p = *iter;
            break;
        }
    }

    return p.arr;
}
