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


class Time {
public:
    using value_type = int32_t;

private:
    value_type _val;

public:
    const static value_type inf = std::numeric_limits<value_type>::max();

    const static value_type neg_inf = std::numeric_limits<value_type>::min();

    Time() : _val {inf} {};

    explicit Time(value_type val) : _val {val} {}

    const value_type& val() const { return _val; }

    friend Time operator+(const Time& t1, const Time& t2) {
        // Make sure ∞ + c = ∞
        if (t1._val == inf || t2._val == inf) return {};

        // _val is small compared to inf, so we don't have to worry about overflow here
        return Time(t1._val + t2._val);
    }

    friend Time operator-(const Time& t1, const Time& t2) {
        if (t1._val < t2._val) return Time(neg_inf);

        return Time(t1._val - t2._val);
    }

    friend bool operator<(const Time& t1, const Time& t2) { return t1._val < t2._val; }

    friend bool operator>(const Time& t1, const Time& t2) { return t1._val > t2._val; }

    friend bool operator>=(const Time& t1, const Time& t2) { return !(t1 < t2); }

    friend bool operator<=(const Time& t1, const Time& t2) { return !(t1 > t2); }

    friend bool operator==(const Time& t1, const Time& t2) { return t1._val == t2._val; }

    Time& operator=(const value_type val) {
        this->_val = val;
        return *this;
    }

    Time& operator=(const Time& other) = default;

    friend std::ostream& operator<<(std::ostream& out, const Time& t) {
        out << t._val;
        return out;
    }
};


struct StopTimeEvent {
    node_id_t stop_id;
    Time arrival_time;
    Time departure_time;

    StopTimeEvent(node_id_t sid, Time::value_type at, Time::value_type dt) :
            stop_id {sid}, arrival_time {at}, departure_time {dt} {};
};

using Events = std::vector<StopTimeEvent>;


struct Transfer {
    node_id_t dest_id;
    Time time;

    Transfer(node_id_t dest, Time::value_type time) : dest_id {dest}, time {time} {};

    friend bool operator<(const Transfer& t1, const Transfer& t2) {
        return (t1.time < t2.time) || ((t1.time == t2.time) && (t1.dest_id < t2.dest_id));
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
    void parse_data();

    void parse_stops();

    void parse_transfers();

    void parse_connections();

public:
    std::string name;
    std::string path;
    std::vector<Connection> connections;
    std::vector<Stop> stops;

    explicit Timetable(std::string name_) : name {std::move(name_)} {
        path = "../Public-Transit-Data/" + name + "/";
        parse_data();
    }

    void summary() const;
};


#endif // DATA_STRUCTURE_HPP
