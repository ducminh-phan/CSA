#include <algorithm>
#include <map>
#include <unordered_map>

#include "data_structure.hpp"
#include "csv.h"
#include "gzstream.h"


template<class T>
using CountMap = std::map<T, size_t>;

template<class T>
std::pair<CountMap<T>, CountMap<T>> find_indices(CountMap<T> counts) {
    CountMap<T> firsts;
    CountMap<T> lasts;
    size_t cumsum = 0;

    for (const auto& elem: counts) {
        firsts[elem.first] = cumsum;
        cumsum += elem.second;
        lasts[elem.first] = cumsum;
    }

    return std::make_pair(firsts, lasts);
}


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

    NodeID stop_id;

    while (stop_routes_reader.read_row(stop_id)) {

        // Add a new stop if we encounter a new id, note that we might have a missing id.
        while (stops.size() <= stop_id) {
            stops.emplace_back(static_cast<NodeID>(stops.size()));
        }
    }

    max_node_id = stops.back().id;
}


void Timetable::parse_transfers() {
    igzstream transfers_file_stream {(path + "transfers.csv.gz").c_str()};
    io::CSVReader<3> transfers_reader {"transfers.csv", transfers_file_stream};
    transfers_reader.read_header(io::ignore_no_column, "from_stop_id", "to_stop_id", "min_transfer_time");

    NodeID source_id;
    NodeID target_id;
    Time time;

    std::map<NodeID, size_t> source_count;
    std::map<NodeID, size_t> target_count;

    while (transfers_reader.read_row(source_id, target_id, time)) {
        _transfers.emplace_back(source_id, target_id, time);

        source_count[source_id] += 1;
        target_count[target_id] += 1;

        max_node_id = std::max(max_node_id, static_cast<std::size_t>(source_id));
        max_node_id = std::max(max_node_id, static_cast<std::size_t>(target_id));
    }

    _backward_transfers = _transfers;

    // We need to sort by source_id first to collect transfers having the same source_id
    std::sort(_transfers.begin(), _transfers.end(),
              [](const Transfer& t1, const Transfer& t2) {
                  return std::make_tuple(t1.source_id, t1.time, t1.target_id) <
                         std::make_tuple(t2.source_id, t2.time, t2.target_id);
              });

    // Sort by target_id first to collect backward transfers having the same target_id
    std::sort(_backward_transfers.begin(), _backward_transfers.end(),
              [](const Transfer& t1, const Transfer& t2) {
                  return std::make_tuple(t1.target_id, t1.time, t1.source_id) <
                         std::make_tuple(t2.target_id, t2.time, t2.source_id);
              });

    // Compute the cumulative sum from the counts
    auto indices_pair = find_indices(source_count);
    auto firsts = indices_pair.first;
    auto lasts = indices_pair.second;

    for (auto& stop: stops) {
        stop.transfers = {_transfers, firsts[stop.id], lasts[stop.id]};
    }

    indices_pair = find_indices(target_count);
    firsts = indices_pair.first;
    lasts = indices_pair.second;

    for (auto& stop: stops) {
        stop.backward_transfers = {_backward_transfers, firsts[stop.id], lasts[stop.id]};
    }
}


void Timetable::parse_hubs() {
    igzstream in_hubs_file_stream {(path + "in_hubs.gr.gz").c_str()};
    io::CSVReader<3, io::trim_chars<>, io::no_quote_escape<' '>> in_hubs_reader {"in_hubs.gr", in_hubs_file_stream};
    in_hubs_reader.set_header("node_id", "stop_id", "distance");

    NodeID node_id;
    NodeID stop_id;
    Time walking_time;

    std::map<NodeID, size_t> in_hubs_stop_count;

    while (in_hubs_reader.read_row(node_id, stop_id, walking_time)) {
        in_hubs_stop_count[stop_id] += 1;

        if (node_id > max_node_id) {
            max_node_id = node_id;
        }

        _in_hubs.emplace_back(stop_id, node_id, walking_time);
    }

    igzstream out_hubs_file_stream {(path + "out_hubs.gr.gz").c_str()};
    io::CSVReader<3, io::trim_chars<>, io::no_quote_escape<' '>> out_hubs_reader {"out_hubs.gr", out_hubs_file_stream};
    out_hubs_reader.set_header("stop_id", "node_id", "distance");

    std::map<NodeID, size_t> out_hubs_stop_count;

    while (out_hubs_reader.read_row(stop_id, node_id, walking_time)) {
        out_hubs_stop_count[stop_id] += 1;

        if (node_id > max_node_id) {
            max_node_id = node_id;
        }

        _out_hubs.emplace_back(stop_id, node_id, walking_time);
    }

    std::sort(_in_hubs.begin(), _in_hubs.end(),
              [](const HubLink& t1, const HubLink& t2) {
                  return std::make_tuple(t1.stop_id, t1.time, t1.hub_id) <
                         std::make_tuple(t2.stop_id, t2.time, t2.hub_id);
              });

    std::sort(_out_hubs.begin(), _out_hubs.end(),
              [](const HubLink& t1, const HubLink& t2) {
                  return std::make_tuple(t1.stop_id, t1.time, t1.hub_id) <
                         std::make_tuple(t2.stop_id, t2.time, t2.hub_id);
              });

    // Compute the cumulative sum from the counts
    auto indices_pair = find_indices(in_hubs_stop_count);
    auto firsts = indices_pair.first;
    auto lasts = indices_pair.second;

    for (auto& stop: stops) {
        stop.in_hubs = {_in_hubs, firsts[stop.id], lasts[stop.id]};
    }

    indices_pair = find_indices(out_hubs_stop_count);
    firsts = indices_pair.first;
    lasts = indices_pair.second;

    for (auto& stop: stops) {
        stop.out_hubs = {_out_hubs, firsts[stop.id], lasts[stop.id]};
    }
}


void Timetable::parse_connections() {
    igzstream stop_times_file_stream {(path + "stop_times.csv.gz").c_str()};
    io::CSVReader<5> stop_times_reader {"stop_times.csv", stop_times_file_stream};
    stop_times_reader.read_header(io::ignore_no_column, "trip_id", "arrival_time", "departure_time", "stop_id",
                                  "stop_sequence");

    TripID trip_id;
    Time arr, dep;
    NodeID stop_id;
    int stop_sequence;

    std::unordered_map<TripID, Events> trip_events;

    while (stop_times_reader.read_row(trip_id, arr, dep, stop_id, stop_sequence)) {
        trip_events[trip_id].emplace_back(stop_id, arr, dep, stop_sequence);

        max_trip_id = std::max(max_trip_id, static_cast<std::size_t>(trip_id));
    }

    for (const auto& kv: trip_events) {
        trip_id = kv.first;
        Events events = kv.second;

        for (size_t i = 0; i < events.size() - 1; ++i) {
            NodeID departure_stop_id = events[i].stop_id;
            NodeID arrival_stop_id = events[i + 1].stop_id;

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

    size_t count_transfers = _transfers.size();
    size_t count_hubs = _in_hubs.size() + _out_hubs.size();

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
