#include <iomanip>
#include <fstream>

#include "config.hpp"
#include "experiments.hpp"
#include "csa.hpp"
#include "csv.h"


void write_results(const Results& results) {
    std::string profile_prefix = profile ? "p" : "";
    std::string hub_prefix = use_hl ? "HL" : "";
    std::string algo_str = profile_prefix + hub_prefix + "CSA";

    std::ofstream stats_file {"../" + name + '_' + algo_str + "_stats.csv"};

    stats_file << "running_time";

    if (profile) {
        stats_file << ",n_journey\n";
    } else {
        stats_file << ",arrival_time\n";
    }

    stats_file << std::fixed << std::setprecision(4);

    double total_running_time = 0;

    for (const auto& result: results) {
        stats_file << result.running_time;
        total_running_time += result.running_time;

        if (profile) {
            stats_file << ',' << result.n_journey << '\n';
        } else {
            stats_file << ',' << result.arrival_time << '\n';
        }
    }

    std::cout << "Average running time: " << total_running_time / results.size() << Timer().unit() << '\n';
}


Queries Experiment::read_queries() {
    Queries queries;
    std::string rank_str = ranked ? "rank_" : "";

    std::ifstream queries_file_stream {_timetable.path + rank_str + "queries.csv"};
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
    std::size_t n_journey {0};

    res.resize(_queries.size());
    for (size_t i = 0; i < _queries.size(); ++i) {
        auto query = _queries[i];
        csa.init();

        Timer timer;

        if (!profile) {
            arrival_time = csa.query(query.source_id, query.target_id, query.dep);
        } else {
            prof = csa.profile_query(query.source_id, query.target_id);
            n_journey = prof.size();
        }

        double running_time = timer.elapsed();

        res[i] = {query.rank, running_time, arrival_time, n_journey};
        csa.clear();

        std::cout << i << std::endl;
    }

    write_results(res);

    Profiler::report();
}
