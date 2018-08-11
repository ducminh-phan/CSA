#include <algorithm>
#include <vector>

#include "csa.hpp"


const node_id_t MAX_STATIONS = 100000;
const trip_id_t MAX_TRIPS = 1000000;


Time ConnectionScan::query(const node_id_t& source_id, const node_id_t& target_id,
                           const Time& departure_time) const {
    std::vector<Time> earliest_arrival_time;
    std::vector<bool> is_reached;

    earliest_arrival_time.resize(MAX_STATIONS);
    is_reached.resize(MAX_TRIPS);

    // Walk from the source to all of its neighbours
    for (const auto& transfer: _timetable->stops[source_id].transfers) {
        earliest_arrival_time[transfer.dest_id] = departure_time + transfer.time;
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
        auto conn = *iter;

        if (earliest_arrival_time[target_id] <= conn.departure_time) break;

        // Check if the trip containing the connection has been reached,
        // or we can get to the connection's departure stop before its departure
        if (is_reached[conn.trip_id] || earliest_arrival_time[conn.departure_stop_id] <= conn.departure_time) {
            // Mark the trip containing the connection as reached
            is_reached[conn.trip_id] = true;

            // Check if the arrival time to the arrival stop of the connection can be improved
            if (conn.arrival_time < earliest_arrival_time[conn.arrival_stop_id]) {
                earliest_arrival_time[conn.arrival_stop_id] = conn.arrival_time;

                // Update the earliest arrival time of the out-neighbours of the arrival stop
                for (const auto& transfer: _timetable->stops[conn.arrival_stop_id].transfers) {
                    // Compute the arrival time at the destination of the transfer
                    Time tmp = conn.arrival_time + transfer.time;

                    // Since the transfers are sorted in the increasing order of walking time,
                    // we can skip the scanning of the transfers as soon as the arrival time
                    // of the destination is later than that of the target stop
                    if (tmp > earliest_arrival_time[target_id]) break;

                    if (tmp < earliest_arrival_time[transfer.dest_id]) {
                        earliest_arrival_time[transfer.dest_id] = tmp;
                    }
                }
            }
        }
    }

    return earliest_arrival_time[target_id];
}
