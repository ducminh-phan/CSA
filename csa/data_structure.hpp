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

using NodeID = uint32_t;
using TripID = uint32_t;
using Distance = uint32_t;
using Time = uint32_t;

// The constant 1e9 is chosen such that ∞ + ∞ does not overflow
constexpr Time INF = static_cast<Time>(1e9);


struct StopTimeEvent {
    NodeID stop_id;
    Time arrival_time;
    Time departure_time;
    int stop_sequence;

    StopTimeEvent(NodeID sid, Time at, Time dt, int seq) :
            stop_id {sid}, arrival_time {at}, departure_time {dt}, stop_sequence {seq} {};
};

using Events = std::vector<StopTimeEvent>;


struct Transfer {
    NodeID source_id;
    NodeID target_id;
    Time time;

    Transfer(NodeID s, NodeID t, Time tm) : source_id {s}, target_id {t}, time {tm} {};
};


struct HubLink {
    NodeID stop_id;
    NodeID hub_id;
    Time time;

    HubLink(NodeID s, NodeID h, Time tm) : stop_id {s}, hub_id {h}, time {tm} {};
};


template<class T>
class VectorView {
public:
    using Container = std::vector<T>;
    using Iterator = typename Container::const_iterator;

private:
    Iterator _begin, _end;

public:
    VectorView() = default;

    VectorView(const Container& vec, size_t first_idx, size_t last_idx) :
            _begin {vec.begin() + first_idx}, _end {vec.begin() + last_idx} {};

    Iterator begin() const { return _begin; }

    Iterator end() const { return _end; }
};


struct Stop {
    NodeID id;
    VectorView<Transfer> transfers;
    VectorView<Transfer> backward_transfers;
    VectorView<HubLink> in_hubs;
    VectorView<HubLink> out_hubs;

    explicit Stop(NodeID sid) : id {sid} {};
};


class Connection {
private:
    std::tuple<Time, Time, TripID, int> _tuple;

public:
    TripID trip_id;
    NodeID departure_stop_id, arrival_stop_id;
    Time departure_time, arrival_time;
    int stop_sequence;

    Connection(TripID tid, NodeID dsid, NodeID asid, Time dt, Time at, int seq) :
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
    VectorView<Transfer>::Container _transfers;
    VectorView<Transfer>::Container _backward_transfers;
    VectorView<HubLink>::Container _in_hubs;
    VectorView<HubLink>::Container _out_hubs;

    void parse_data();

    void parse_stops();

    void parse_transfers();

    void parse_hubs();

    void parse_connections();

public:
    std::string path;
    std::vector<Connection> connections;
    std::vector<Stop> stops;
    std::size_t max_node_id = 0;
    std::size_t max_trip_id = 0;

    Timetable() {
        path = "../Public-Transit-Data/" + name + "/";
        parse_data();
    }

    void summary() const;
};


Time distance_to_time(const Distance& d);

#endif // DATA_STRUCTURE_HPP
