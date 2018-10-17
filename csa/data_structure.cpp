#include <algorithm>
#include <cmath>
#include <unordered_map>

#include "config.hpp"
#include "data_structure.hpp"
#include "csv.h"
#include "gzstream.h"


void Timetable::parse_data() {
    Timer timer;

    std::cout << "Parsing the data..." << std::endl;

    parse_stops();

    if (use_hl) {
        parse_hubs();
    } else {
        parse_transfers();
    }

    parse_connections();

    std::cout << "Complete parsing the data." << std::endl;
    std::cout << "Time elapsed: " << timer.elapsed() << timer.unit() << std::endl;
}


void Timetable::parse_stops() {
    igzstream stop_routes_file_stream {(path + "stop_routes.csv.gz").c_str()};
    io::CSVReader<1> stop_routes_reader {"stop_routes.csv", stop_routes_file_stream};
    stop_routes_reader.read_header(io::ignore_extra_column, "stop_id");

    node_id_t stop_id;

    while (stop_routes_reader.read_row(stop_id)) {

        // Add a new stop if we encounter a new id, note that we might have a missing id.
        while (stops.size() <= stop_id) {
            stops.emplace_back(static_cast<node_id_t>(stops.size()));
        }
    }

    max_node_id = stops.back().id;
}


void Timetable::parse_transfers() {
    igzstream transfers_file_stream {(path + "transfers.csv.gz").c_str()};
    io::CSVReader<3> transfers_reader {"transfers.csv", transfers_file_stream};
    transfers_reader.read_header(io::ignore_no_column, "from_stop_id", "to_stop_id", "min_transfer_time");

    node_id_t from;
    node_id_t to;
    Time time;

    while (transfers_reader.read_row(from, to, time)) {
        stops[from].transfers.emplace_back(to, time);
        stops[to].backward_transfers.emplace_back(from, time);

        max_node_id = std::max(max_node_id, static_cast<std::size_t>(from));
        max_node_id = std::max(max_node_id, static_cast<std::size_t>(to));
    }

    for (auto& stop: stops) {
        std::sort(stop.transfers.begin(), stop.transfers.end());
        std::sort(stop.backward_transfers.begin(), stop.backward_transfers.end());
    }
}


void Timetable::parse_hubs() {
    inverse_in_hubs.resize(max_node_id + 1);

    igzstream in_hubs_file_stream {(path + "in_hubs.gr.gz").c_str()};
    io::CSVReader<3, io::trim_chars<>, io::no_quote_escape<' '>> in_hubs_reader {"in_hubs.gr", in_hubs_file_stream};
    in_hubs_reader.set_header("node_id", "stop_id", "distance");

    node_id_t node_id;
    node_id_t stop_id;
    distance_t distance;

    while (in_hubs_reader.read_row(node_id, stop_id, distance)) {
        auto time = distance_to_time(distance);

        if (node_id > max_node_id) {
            max_node_id = node_id;
            inverse_in_hubs.resize(max_node_id + 1);
        }

        stops[stop_id].in_hubs.emplace_back(time, node_id);
        inverse_in_hubs[node_id].emplace_back(time, stop_id);
    }

    inverse_out_hubs.resize(max_node_id + 1);

    igzstream out_hubs_file_stream {(path + "out_hubs.gr.gz").c_str()};
    io::CSVReader<3, io::trim_chars<>, io::no_quote_escape<' '>> out_hubs_reader {"out_hubs.gr", out_hubs_file_stream};
    out_hubs_reader.set_header("stop_id", "node_id", "distance");

    while (out_hubs_reader.read_row(stop_id, node_id, distance)) {
        auto time = distance_to_time(distance);

        if (node_id > max_node_id) {
            max_node_id = node_id;
            inverse_out_hubs.resize(max_node_id + 1);
        }

        stops[stop_id].out_hubs.emplace_back(time, node_id);
        inverse_out_hubs[node_id].emplace_back(time, stop_id);
    }

    for (auto& stop: stops) {
        std::sort(stop.in_hubs.begin(), stop.in_hubs.end());
        std::sort(stop.out_hubs.begin(), stop.out_hubs.end());
    }
}


void Timetable::parse_connections() {
    igzstream stop_times_file_stream {(path + "stop_times.csv.gz").c_str()};
    io::CSVReader<5> stop_times_reader {"stop_times.csv", stop_times_file_stream};
    stop_times_reader.read_header(io::ignore_no_column, "trip_id", "arrival_time", "departure_time", "stop_id",
                                  "stop_sequence");

    trip_id_t trip_id;
    Time arr, dep;
    node_id_t stop_id;
    int stop_sequence;

    std::unordered_map<trip_id_t, Events> trip_events;

    while (stop_times_reader.read_row(trip_id, arr, dep, stop_id, stop_sequence)) {
        trip_events[trip_id].emplace_back(stop_id, arr, dep, stop_sequence);

        max_trip_id = std::max(max_trip_id, static_cast<std::size_t>(trip_id));
    }

    for (const auto& kv: trip_events) {
        trip_id = kv.first;
        Events events = kv.second;

        for (size_t i = 0; i < events.size() - 1; ++i) {
            node_id_t departure_stop_id = events[i].stop_id;
            node_id_t arrival_stop_id = events[i + 1].stop_id;

            Time departure_time = events[i].departure_time;
            Time arrival_time = events[i + 1].arrival_time;

            int seq = events[i].stop_sequence;

            connections.emplace_back(trip_id, departure_stop_id, arrival_stop_id, departure_time, arrival_time, seq);
        }
    }

    std::sort(connections.begin(), connections.end());
}

void Timetable::summary() const {
    std::cout << std::string(80, '-') << std::endl;

    std::cout << "Summary of the dataset:" << std::endl;
    std::cout << "Name: " << name << std::endl;

    int count_transfers = 0;
    int count_hubs = 0;
    for (const auto& stop: stops) {
        count_transfers += stop.transfers.size();
        count_hubs += stop.in_hubs.size();
        count_hubs += stop.out_hubs.size();
    }

    std::cout << stops.size() << " stops" << std::endl;

    if (use_hl) {
        std::cout.setf(std::ios::fixed, std::ios::floatfield);
        std::cout.precision(3);
        std::cout << count_hubs / static_cast<double>(stops.size()) << " hubs in average" << std::endl;
    } else {
        std::cout << count_transfers << " transfers" << std::endl;
    }

    std::cout << connections.size() << " connections" << std::endl;

    std::cout << std::string(80, '-') << std::endl;
}


Time distance_to_time(const distance_t& d) {
    static const double v {4.0};  // km/h

    return Time {static_cast<Time>(std::lround(9 * d / 25 / v))};
}
