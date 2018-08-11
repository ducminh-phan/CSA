#include <algorithm>
#include <unordered_map>

#include "data_structure.hpp"
#include "csv_reader.hpp"
#include "gzstream.h"


void Timetable::parse_data() {
    Timer timer;

    std::cout << "Parsing the data..." << std::endl;

    parse_stops();
    parse_transfers();
    parse_connections();

    std::cout << "Complete parsing the data." << std::endl;
    std::cout << "Time elapsed: " << timer.elapsed() << timer.unit() << std::endl;
}


void Timetable::parse_stops() {
    auto stop_routes_file = read_dataset_file<igzstream>(path + "stop_routes.csv.gz");

    for (CSVIterator<uint32_t> iter {stop_routes_file.get()}; iter != CSVIterator<uint32_t>(); ++iter) {
        auto stop_id = static_cast<node_id_t>((*iter)[0]);

        // Add a new stop if we encounter a new id, note that we might have a missing id.
        while (stops.size() <= stop_id) {
            stops.emplace_back(static_cast<node_id_t>(stops.size() - 1));
        }
    }
}


void Timetable::parse_transfers() {
    auto transfers_file = read_dataset_file<igzstream>(path + "transfers.csv.gz");

    for (CSVIterator<uint32_t> iter {transfers_file.get()}; iter != CSVIterator<uint32_t>(); ++iter) {
        auto from = static_cast<node_id_t>((*iter)[0]);
        auto to = static_cast<node_id_t>((*iter)[1]);
        auto time = static_cast<Time::value_type>((*iter)[2]);

        stops[from].transfers.emplace_back(to, time);
    }

    for (auto& stop: stops) {
        std::sort(stop.transfers.begin(), stop.transfers.end());
    }
}


void Timetable::parse_connections() {
    auto stop_times_file = read_dataset_file<igzstream>(path + "stop_times.csv.gz");

    std::unordered_map<trip_id_t, Events> trip_events;

    for (CSVIterator<uint32_t> iter {stop_times_file.get()}; iter != CSVIterator<uint32_t>(); ++iter) {
        auto trip_id = static_cast<trip_id_t>((*iter)[0]);
        auto arr = static_cast<Time::value_type>((*iter)[1]);
        auto dep = static_cast<Time::value_type>((*iter)[2]);
        auto stop_id = static_cast<node_id_t>((*iter)[3]);

        trip_events[trip_id].emplace_back(stop_id, arr, dep);
    }

    for (const auto& kv: trip_events) {
        trip_id_t trip_id = kv.first;
        Events events = kv.second;

        for (size_t i = 0; i < events.size() - 1; ++i) {
            node_id_t departure_stop_id = events[i].stop_id;
            node_id_t arrival_stop_id = events[i + 1].stop_id;

            Time departure_time = events[i].departure_time;
            Time arrival_time = events[i + 1].arrival_time;

            connections.emplace_back(trip_id, departure_stop_id, arrival_stop_id, departure_time, arrival_time);
        }
    }

    std::sort(connections.begin(), connections.end());
}


void Timetable::summary() const {
    std::cout << std::string(80, '-') << std::endl;

    std::cout << "Summary of the dataset:" << std::endl;
    std::cout << "Name: " << name << std::endl;

    int count_transfers = 0;
    for (const auto& stop: stops) {
        count_transfers += stop.transfers.size();
    }

    std::cout << stops.size() << " stops" << std::endl;

    std::cout << count_transfers << " transfers" << std::endl;

    std::cout << connections.size() << " connections" << std::endl;

    std::cout << std::string(80, '-') << std::endl;
}
