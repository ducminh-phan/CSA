#include <algorithm>
#include <vector>

#include "config.hpp"
#include "csa.hpp"


Time ConnectionScan::query(const node_id_t& source_id, const node_id_t& target_id,
                           const Time& departure_time) {
    Element tmp_elem;
    Time tmp_time;

    bags[source_id].emplace(departure_time, 0, Time(0));

    #ifdef PROFILE
    Profiler prof {__func__};
    #endif

    // Walk from the source to all of its neighbours
    if (!use_hl) {
        for (const auto& transfer: _timetable->stops[source_id].transfers) {
            Time arrival_time = departure_time + transfer.time;

            bags[transfer.dest_id].emplace(arrival_time, 0, transfer.time);
            earliest_arrival_time[transfer.dest_id] = arrival_time;
        }
    } else {
        // Propagate the departure time from the source stop to all its out-hubs
        for (const auto& hub_pair: _timetable->stops[source_id].out_hubs) {
            auto& walking_time = hub_pair.first;
            auto& hub_id = hub_pair.second;

            tmp_time = departure_time + walking_time;
            earliest_arrival_time[hub_id] = tmp_time;
            bags[hub_id].emplace(tmp_time, 0, walking_time);

            // Then propagate from the out-hubs to its inverse in-hubs
            for (const auto& inverse_hub_pair: _timetable->inverse_in_hubs[hub_id]) {
                auto& _walking_time = inverse_hub_pair.first;
                auto& stop_id = inverse_hub_pair.second;

                bags[stop_id].emplace(tmp_time + _walking_time, 0, walking_time + _walking_time);

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
    // departure_time -> arrival_time -> departure_stop_id -> arrival_stop_id -> trip_id,
    // the const_iterator obtained will point to the connection we need to find
    Connection dummy_conn {0, 0, 0, departure_time, departure_time};
    auto first_conn_iter = std::lower_bound(_timetable->connections.begin(), _timetable->connections.end(),
                                            dummy_conn);

    for (auto iter = first_conn_iter; iter != _timetable->connections.end(); ++iter) {
        #ifdef PROFILE
        Profiler loop {"Loop"};
        #endif

        const Connection& conn = *iter;
        const auto& arr_id = conn.arrival_stop_id;
        const auto& dep_id = conn.departure_stop_id;
        const Stop& arrival_stop = _timetable->stops[arr_id];

        if (!is_reached[conn.trip_id] && use_hl) {
            #ifdef PROFILE
            Profiler update_in_hubs {"Propagate from in-hubs"};
            #endif

            // Update the earliest arrival time of the departure stop of the connection
            // using its in-hubs
            for (const auto& hub_pair: _timetable->stops[dep_id].in_hubs) {
                auto& walking_time = hub_pair.first;
                auto& hub_id = hub_pair.second;

                tmp_time = earliest_arrival_time[hub_id] + walking_time;

                if (tmp_time < earliest_arrival_time[dep_id]) {
                    earliest_arrival_time[dep_id] = tmp_time;
                }

                #ifdef PROFILE
                Profiler merge_in_hubs {"Merge bags of in-hubs"};
                #endif

                for (const auto& elem: bags[hub_id]) {
                    tmp_elem = elem;

                    // Add walking_time to the arrival time and the total walking time
                    tmp_elem.arrival_time += walking_time;
                    tmp_elem.walking_time += walking_time;

                    bags[dep_id].insert(tmp_elem);
                }
            }
        }

        // Check if the trip containing the connection has been reached,
        // or we can get to the connection's departure stop before its departure
        if (is_reached[conn.trip_id] || earliest_arrival_time[dep_id] <= conn.departure_time) {
            // Mark the trip containing the connection as reached
            is_reached[conn.trip_id] = true;

            // Check if the arrival time to the arrival stop of the connection can be improved
            if (conn.arrival_time < earliest_arrival_time[arr_id]) {
                earliest_arrival_time[arr_id] = conn.arrival_time;

                for (const auto& elem: bags[dep_id]) {
                    tmp_elem = elem;

                    if (tmp_elem.arrival_time >= conn.arrival_time) {
                        tmp_elem.arrival_time = conn.arrival_time;
                    }

                    bags[arr_id].insert(tmp_elem);
                }

                if (!use_hl) {
                    // Update the earliest arrival time of the out-neighbours of the arrival stop
                    for (const auto& transfer: arrival_stop.transfers) {
                        Time transfer_time = transfer.time;

                        // Compute the arrival time at the destination of the transfer
                        tmp_time = conn.arrival_time + transfer_time;

                        if (tmp_time < earliest_arrival_time[transfer.dest_id]) {
                            earliest_arrival_time[transfer.dest_id] = tmp_time;
                        }

                        for (const auto& elem: bags[arr_id]) {
                            tmp_elem = elem;

                            tmp_elem.arrival_time += transfer_time;
                            tmp_elem.no_transfers += 1;
                            tmp_elem.walking_time += transfer_time;

                            bags[transfer.dest_id].insert(tmp_elem);
                        }
                    }
                } else {
                    #ifdef PROFILE
                    Profiler update_out_hubs {"Propagate to out hubs"};
                    #endif

                    // Update the earliest arrival time of the out-hubs of the arrival stop
                    for (const auto& hub_pair: arrival_stop.out_hubs) {
                        auto& walking_time = hub_pair.first;
                        auto& hub_id = hub_pair.second;

                        tmp_time = conn.arrival_time + walking_time;

                        if (tmp_time < earliest_arrival_time[hub_id]) {
                            earliest_arrival_time[hub_id] = tmp_time;
                        }

                        #ifdef PROFILE
                        Profiler merge_out_hubs {"Merge bags of arrival stop to out-hubs"};
                        #endif

                        for (const auto& elem: bags[arr_id]) {
                            tmp_elem = elem;

                            tmp_elem.arrival_time += walking_time;
                            tmp_elem.no_transfers += 1;
                            tmp_elem.walking_time += walking_time;

                            bags[hub_id].insert(tmp_elem);
                        }
                    }
                }
            }
        }
    }

    if (use_hl) {
        for (const auto& hub_pair: _timetable->stops[target_id].in_hubs) {
            auto walking_time = hub_pair.first;
            auto hub_id = hub_pair.second;

            tmp_time = earliest_arrival_time[hub_id] + walking_time;

            if (tmp_time < earliest_arrival_time[target_id]) {
                earliest_arrival_time[target_id] = tmp_time;
            }

            for (const auto& elem: bags[hub_id]) {
                tmp_elem = elem;

                tmp_elem.arrival_time += walking_time;
                tmp_elem.walking_time += walking_time;

                bags[target_id].insert(tmp_elem);
            }
        }
    }

    return earliest_arrival_time[target_id];
}


Time ConnectionScan::backward_query(const node_id_t& source_id, const node_id_t& target_id,
                                    const Time& arrival_time) const {
    std::vector<Time> latest_departure_time;
    std::vector<bool> is_reached;

    latest_departure_time.assign(_timetable->max_node_id + 1, NEG_INF_TIME);
    is_reached.resize(_timetable->max_trip_id + 1);

    // Walk from the target to all of its neighbours
    if (!use_hl) {
        for (const auto& transfer: _timetable->stops[target_id].backward_transfers) {
            latest_departure_time[transfer.dest_id] = arrival_time - transfer.time;
        }
    } else {
        // Propagate the arrival time from the target stop to all its in-hubs
        for (const auto& hub_pair: _timetable->stops[target_id].in_hubs) {
            auto walking_time = hub_pair.first;
            auto hub_id = hub_pair.second;

            auto tmp = arrival_time - walking_time;
            latest_departure_time[hub_id] = tmp;

            // Then propagate from the in-hubs to its inverse out-hubs
            for (const auto& inverse_hub_pair: _timetable->inverse_out_hubs[hub_id]) {
                auto _walking_time = inverse_hub_pair.first;
                auto stop_id = inverse_hub_pair.second;

                latest_departure_time[stop_id] = std::max(
                        latest_departure_time[stop_id],
                        tmp - _walking_time
                );
            }
        }
    }

    // Find the last connection arriving not after arrival_time,
    // first we need to create the "dummy" connection, then use std::lower_bound
    // to get the const_iterator pointing to the first connection in the set of connections
    // which is equivalent or goes after this dummy connection. Since the connections are ordered
    // lexicographically, with the order of attribute:
    // departure_time -> arrival_time -> departure_stop_id -> arrival_stop_id -> trip_id,
    // the const_iterator obtained will point to the first connection whose departure time >=
    // the given arrival_time, which is usually a few iterators behind the optimal iterator,
    // but we wouldn't mind this tiny difference
    Connection dummy_conn {0, 0, 0, arrival_time, arrival_time};
    auto first_conn_iter = std::lower_bound(_timetable->connections.begin(), _timetable->connections.end(),
                                            dummy_conn);

    // Iterator over the connections in the reverse direction, here in the stopping condition,
    // since iter is a forward iterator, and rend is a reverse iterator, we need to compare
    // the address of what iter pointing to and that of rend
    for (auto iter = first_conn_iter; &(*iter) != &(*_timetable->connections.rend()); --iter) {
        const Connection& conn = *iter;

        if (latest_departure_time[source_id] >= conn.arrival_time) break;

        const Stop& arrival_stop = _timetable->stops[conn.arrival_stop_id];
        const Stop& departure_stop = _timetable->stops[conn.departure_stop_id];

        if (use_hl) {
            // Update the latest departure time of the arrival stop of the connection
            // using its out-hubs
            for (const auto& hub_pair: _timetable->stops[conn.arrival_stop_id].in_hubs) {
                auto walking_time = hub_pair.first;
                auto hub_id = hub_pair.second;

                auto tmp = latest_departure_time[hub_id] - walking_time;

                if (tmp < latest_departure_time[source_id]) break;

                if (tmp > latest_departure_time[arrival_stop.id]) {
                    latest_departure_time[arrival_stop.id] = tmp;
                }
            }
        }

        // Check if the trip containing the connection has been reached,
        // or we can get to the connection's arrival stop after its arrival
        if (is_reached[conn.trip_id] || latest_departure_time[conn.arrival_stop_id] >= conn.arrival_time) {
            // Mark the trip containing the connection as reached
            is_reached[conn.trip_id] = true;

            // Check if the departure time to the departure stop of the connection can be improved
            if (conn.departure_time > latest_departure_time[conn.departure_stop_id]) {
                latest_departure_time[conn.departure_stop_id] = conn.departure_time;

                if (!use_hl) {
                    // Update the latest departure time of the in-neighbours of the departure stop
                    for (const auto& transfer: departure_stop.backward_transfers) {
                        // Compute the departure time at the source of the transfer
                        Time tmp = conn.departure_time - transfer.time;

                        // Since the transfers are sorted in the increasing order of walking time,
                        // we can skip the scanning of the transfers as soon as the departure time
                        // of the source is earlier than that of the source stop
                        if (tmp < latest_departure_time[source_id]) break;

                        if (tmp > latest_departure_time[transfer.dest_id]) {
                            latest_departure_time[transfer.dest_id] = tmp;
                        }
                    }
                } else {
                    // Update the latest departure time of the in-hubs of the departure stop
                    for (const auto& hub_pair: departure_stop.in_hubs) {
                        auto walking_time = hub_pair.first;
                        auto hub_id = hub_pair.second;

                        auto tmp = conn.departure_time - walking_time;

                        if (tmp < latest_departure_time[source_id]) break;

                        if (tmp > latest_departure_time[hub_id]) {
                            latest_departure_time[hub_id] = tmp;
                        }
                    }
                }
            }
        }
    }

    return latest_departure_time[source_id];
}


void ConnectionScan::init() {
    bags.resize(_timetable->max_node_id + 1);
    earliest_arrival_time.resize(_timetable->max_node_id + 1);
    is_reached.resize(_timetable->max_trip_id + 1);
}


void ConnectionScan::clear() {
    bags.clear();
    earliest_arrival_time.clear();
    is_reached.clear();
}
