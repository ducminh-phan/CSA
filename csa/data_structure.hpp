#ifndef DATA_STRUCTURE_HPP
#define DATA_STRUCTURE_HPP

#include <cstdint>
#include <iostream>
#include <limits>
#include <set>
#include <tuple>
#include <vector>

#include "utilities.hpp"

using node_id_t = uint32_t;
using trip_id_t = int32_t;
using Time = uint32_t;

const node_id_t MAX_STATIONS = 100000;
const Time INF_TIME = std::numeric_limits<Time>::max();


struct StopTimeEvent {
    node_id_t stop_id;
    Time arrival_time;
    Time departure_time;

    StopTimeEvent(node_id_t sid, Time at, Time dt) :
            stop_id {sid}, arrival_time {at}, departure_time {dt} {};
};

using Events = std::vector<StopTimeEvent>;


struct Transfer {
    node_id_t dest;
    Time time;

    Transfer(node_id_t dest, Time time) : dest {dest}, time {time} {};

    friend bool operator<(const Transfer& t1, const Transfer& t2) {
        return (t1.time < t2.time) || ((t1.time == t2.time) && (t1.dest < t2.dest));
    }
};


struct Stop {
    node_id_t id;
    std::vector<Transfer> transfers;


    explicit Stop(node_id_t sid) : id {sid} {};
};


class Connection {
private:
    std::tuple<Time, Time, node_id_t, node_id_t, trip_id_t> _tuple() const {
        return {departure_time, arrival_time, departure_stop_id, arrival_stop_id, trip_id};
    }

public:
    trip_id_t trip_id;
    node_id_t departure_stop_id, arrival_stop_id;
    Time departure_time, arrival_time;

    Connection(trip_id_t tid, node_id_t dsid, node_id_t asid, Time dt, Time at) :
            trip_id {tid}, departure_stop_id {dsid}, arrival_stop_id {asid},
            departure_time {dt}, arrival_time {at} {};

    friend bool operator<(const Connection& conn1, const Connection& conn2) {
        return conn1._tuple() < conn2._tuple();
    }

    friend bool operator==(const Connection& conn1, const Connection& conn2) {
        return conn1._tuple() == conn2._tuple();
    }
};


class Timetable {
private:
    std::string _name;
    std::string _path;

    void parse_data();

    void parse_stops();

    void parse_transfers();

    void parse_connections();

public:
    std::set<Connection> connections;
    std::vector<Stop> stops;

    explicit Timetable(std::string name) : _name {std::move(name)} {
        _path = "../Public-Transit-Data/" + _name + "/";
        parse_data();
    }

    void summary() const;
};


#endif // DATA_STRUCTURE_HPP
