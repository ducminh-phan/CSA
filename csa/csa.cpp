#include <algorithm>
#include <vector>

#include "config.hpp"
#include "csa.hpp"


Time ConnectionScan::query(const node_id_t& source_id, const node_id_t& target_id,
                           const Time& departure_time, const bool& target_pruning) {
    Time tmp_time;

    #ifdef PROFILE
    Profiler prof {__func__};
    #endif

    // Walk from the source to all of its neighbours
    if (!use_hl) {
        for (const auto& transfer: _timetable->stops[source_id].transfers) {
            earliest_arrival_time[transfer.dest_id] = departure_time + transfer.time;
        }
    } else {
        // Propagate the departure time from the source stop to all its out-hubs
        for (const auto& hub_pair: _timetable->stops[source_id].out_hubs) {
            const auto& walking_time = hub_pair.first;
            const auto& hub_id = hub_pair.second;

            tmp_time = departure_time + walking_time;
            earliest_arrival_time[hub_id] = tmp_time;

            // Then propagate from the out-hubs to its inverse in-hubs
            for (const auto& inverse_hub_pair: _timetable->inverse_in_hubs[hub_id]) {
                const auto& _walking_time = inverse_hub_pair.first;
                const auto& stop_id = inverse_hub_pair.second;

                earliest_arrival_time[stop_id] = std::min(
                        earliest_arrival_time[stop_id],
                        tmp_time + _walking_time
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
                for (const auto& hub_pair: _timetable->stops[target_id].in_hubs) {
                    const auto& walking_time = hub_pair.first;
                    const auto& hub_id = hub_pair.second;

                    tmp_time = earliest_arrival_time[hub_id] + walking_time;

                    if (tmp_time < earliest_arrival_time[target_id]) {
                        earliest_arrival_time[target_id] = tmp_time;
                    }
                }
            }

            break;
        }

        if (!is_reached[conn.trip_id] && use_hl) {
            update_departure_stop(dep_id);
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
    walking_time_to_target.assign(_timetable->max_trip_id + 1, INF);
}


void ConnectionScan::clear() {
    earliest_arrival_time.clear();
    is_reached.clear();

    stop_profile.clear();
    trip_earliest_time.clear();
    walking_time_to_target.clear();
}


void ConnectionScan::update_departure_stop(const node_id_t& dep_id) {
    // Update the earliest arrival time of the departure stop of the connection
    // using its in-hubs

    #ifdef PROFILE
    Profiler prof {__func__};
    #endif

    Time tmp_time;

    for (const auto& hub_pair: _timetable->stops[dep_id].in_hubs) {
        const auto& walking_time = hub_pair.first;
        const auto& hub_id = hub_pair.second;

        tmp_time = earliest_arrival_time[hub_id] + walking_time;

        // We cannot use early stopping here since earliest_arrival_time[hub_id] is not
        // a constant, thus tmp_time is not increasing

        if (tmp_time < earliest_arrival_time[dep_id]) {
            earliest_arrival_time[dep_id] = tmp_time;
        }
    }
}


void ConnectionScan::update_out_hubs(const node_id_t& arr_id, const Time& arrival_time,
                                     const node_id_t& target_id) {
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

            if (tmp_time < earliest_arrival_time[transfer.dest_id]) {
                earliest_arrival_time[transfer.dest_id] = tmp_time;
            }
        }
    } else {
        // Update the earliest arrival time of the out-hubs of the arrival stop
        for (const auto& hub_pair: arrival_stop.out_hubs) {
            const auto& walking_time = hub_pair.first;
            const auto& hub_id = hub_pair.second;

            tmp_time = arrival_time + walking_time;

            if (tmp_time > earliest_arrival_time[target_id]) break;

            if (tmp_time < earliest_arrival_time[hub_id]) {
                earliest_arrival_time[hub_id] = tmp_time;
            }
        }
    }
}


ProfilePareto ConnectionScan::profile_query(const node_id_t& source_id,
                                            const node_id_t& target_id) {
    // Run a normal query with departure_time 0 and do not target-prune to scan all connections
    query(source_id, target_id, 0, false);

    // Handle final footpaths
    for (const auto& transfer: _timetable->stops[target_id].backward_transfers) {
        walking_time_to_target[transfer.dest_id] = transfer.time;
    }

    const auto& first = _timetable->connections.rbegin();
    const auto& last = _timetable->connections.rend();

    Time t1, t2, t3, t_conn;

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

        // Arrival time when transferring to another route using the same stop. Since in the profile,
        // both the departure time and arrival time are in decreasing order, we only need to find
        // the last pair with departure time at least conn_iter->arrival_time
        // TODO: compare the performance of linear search and binary search
        ProfilePareto::pair_t p;
        auto _first = stop_profile[conn_iter->arrival_stop_id].rbegin();
        auto _last = stop_profile[conn_iter->arrival_stop_id].rend();
        for (auto iter = _first; iter != _last; ++iter) {
            // We are iterating in the reverse order, starting from the back of the profile vector,
            // thus the first profile pair with departure time >= the arrival time of the connection
            // will be the pair we need
            if (iter->dep >= conn_iter->arrival_time) {
                p = *iter;
                break;
            }
        }

        t3 = p.arr;

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

            for (const auto& transfer: _timetable->stops[conn_iter->departure_stop_id].backward_transfers) {
                stop_profile[transfer.dest_id].emplace(conn_iter->departure_time - transfer.time, t_conn);
            }
        }

        trip_earliest_time[conn_iter->trip_id] = t_conn;
    }

    return stop_profile[source_id];
}
