#ifndef DATA_STRUCTURE_HPP
#define DATA_STRUCTURE_HPP

#include <cstdint>
#include <iostream>
#include <limits>
#include <set>
#include <tuple>
#include <vector>

#include "config.hpp"
#include "utilities.hpp"

using node_id_t = uint32_t;
using trip_id_t = int32_t;
using distance_t = uint32_t;
using Time = int32_t;

// The constants 1e9 and -1e9 are chosen such that ∞ + ∞ does not overflow
constexpr Time INF = static_cast<Time>(1e9);
constexpr Time NEG_INF = static_cast<Time>(-1e9);


struct StopTimeEvent {
    node_id_t stop_id;
    Time arrival_time;
    Time departure_time;
    int stop_sequence;

    StopTimeEvent(node_id_t sid, Time at, Time dt, int seq) :
            stop_id {sid}, arrival_time {at}, departure_time {dt}, stop_sequence {seq} {};
};

using Events = std::vector<StopTimeEvent>;


struct Transfer {
    node_id_t dest_id;
    Time time;

    Transfer(node_id_t dest, Time time) : dest_id {dest}, time {time} {};

    friend bool operator<(const Transfer& t1, const Transfer& t2) {
        return (t1.time < t2.time) || ((t1.time == t2.time) && (t1.dest_id < t2.dest_id));
    }
};


using hubs_t = std::vector<std::pair<Time, node_id_t>>;


struct Stop {
    node_id_t id;
    std::vector<Transfer> transfers;
    std::vector<Transfer> backward_transfers;
    hubs_t in_hubs;
    hubs_t out_hubs;

    explicit Stop(node_id_t sid) : id {sid} {};
};


class Connection {
private:
    std::tuple<Time, Time, trip_id_t, int> _tuple;

public:
    trip_id_t trip_id;
    node_id_t departure_stop_id, arrival_stop_id;
    Time departure_time, arrival_time;
    int stop_sequence;

    Connection(trip_id_t tid, node_id_t dsid, node_id_t asid, Time dt, Time at, int seq) :
            trip_id {tid}, departure_stop_id {dsid}, arrival_stop_id {asid},
            departure_time {dt}, arrival_time {at}, stop_sequence {seq} {
        _tuple = {departure_time, arrival_time, trip_id, stop_sequence};
    };

    friend bool operator<(const Connection& conn1, const Connection& conn2) {
        return conn1._tuple < conn2._tuple;
    }

    friend bool operator==(const Connection& conn1, const Connection& conn2) {
        return conn1._tuple == conn2._tuple;
    }
};


class Timetable {
private:
    void parse_data();

    void parse_stops();

    void parse_transfers();

    void parse_hubs();

    void parse_connections();

public:
    std::string path;
    std::vector<Connection> connections;
    std::vector<Stop> stops;
    std::vector<hubs_t> inverse_in_hubs;
    std::vector<hubs_t> inverse_out_hubs;
    std::size_t max_node_id = 0;
    std::size_t max_trip_id = 0;

    Timetable() {
        path = "../Public-Transit-Data/" + name + "/";
        parse_data();
    }

    void summary() const;
};


Time distance_to_time(const distance_t& d);

#endif // DATA_STRUCTURE_HPP
