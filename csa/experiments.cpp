#include <iomanip>
#include <fstream>

#include "config.hpp"
#include "experiments.hpp"
#include "csa.hpp"
#include "csv.h"
#include "gzstream.h"


void write_results(const Results& results) {
    std::string algo_str = use_hl ? "HLCSA" : "CSA";

    std::ofstream running_time_file {"../" + name + '_' + algo_str + "_running_time.csv"};
    std::ofstream arrival_time_file {"../" + name + '_' + algo_str + "_arrival_times.csv"};

    running_time_file << "running_time\n";
    arrival_time_file << "arrival_time\n";

    running_time_file << std::fixed << std::setprecision(4);

    double total_running_time = 0;

    for (const auto& result: results) {
        running_time_file << result.running_time << '\n';
        total_running_time += result.running_time;

        arrival_time_file << result.arrival_time << '\n';
    }

    std::cout << "Average running time: " << total_running_time / results.size() << Timer().unit() << '\n';
}


Queries Experiment::read_queries() {
    Queries queries;
    std::string rank_str = ranked ? "rank_" : "";

    igzstream queries_file_stream {(_timetable.path + rank_str + "queries.csv").c_str()};
    io::CSVReader<4> queries_file_reader {"queries.csv", queries_file_stream};
    queries_file_reader.read_header(io::ignore_no_column, "rank", "source", "target", "time");

    uint16_t r;
    NodeID s, t;
    Time d;

    while (queries_file_reader.read_row(r, s, t, d)) {
        queries.emplace_back(r, s, t, d);
    }

    return queries;
}


void Experiment::run() const {
    Results res;
    ConnectionScan csa {&_timetable};
    Time arrival_time {INF};
    ProfilePareto prof;

    res.resize(_queries.size());
    for (size_t i = 0; i < _queries.size(); ++i) {
        auto query = _queries[i];
        csa.init();

        Timer timer;

        if (!profile) {
            arrival_time = csa.query(query.source_id, query.target_id, query.dep);
        } else {
            prof = csa.profile_query(query.source_id, query.target_id);
        }

        double running_time = timer.elapsed();

        res[i] = {query.rank, running_time, arrival_time};
        csa.clear();

        std::cout << i << std::endl;
    }

    write_results(res);

    Profiler::report();
}
