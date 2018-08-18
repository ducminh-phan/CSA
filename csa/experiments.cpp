#include <iomanip>
#include <fstream>

#include "config.hpp"
#include "experiments.hpp"
#include "csa.hpp"
#include "csv_reader.hpp"


void write_results(const Results& results) {
    std::string algo_str = use_hl ? "HLCSA" : "CSA";

    std::ofstream running_time_file {"../" + name + '_' + algo_str + "_running_time.csv"};
    std::ofstream arrival_time_file {"../" + name + '_' + algo_str + "_arrival_times.csv"};

    running_time_file << "running_time\n";
    arrival_time_file << "arrival_time\n";

    running_time_file << std::fixed << std::setprecision(4);

    for (const auto& result: results) {
        running_time_file << result.running_time << '\n';

        arrival_time_file << result.arrival_time << '\n';
    }
}


Queries Experiment::read_queries() {
    Queries queries;
    std::string rank_str = ranked ? "rank_" : "";
    auto queries_file = read_dataset_file<std::ifstream>(_timetable.path + rank_str + "queries.csv");

    for (CSVIterator<uint32_t> iter {queries_file.get()}; iter != CSVIterator<uint32_t>(); ++iter) {
        auto r = static_cast<uint16_t>((*iter)[0]);
        auto s = static_cast<node_id_t>((*iter)[1]);
        auto t = static_cast<node_id_t>((*iter)[2]);
        auto d = static_cast<Time::value_type>((*iter)[3]);

        queries.emplace_back(r, s, t, d);
    }

    return queries;
}


void Experiment::run() const {
    Results res;
    ConnectionScan csa {&_timetable};

    res.resize(_queries.size());
    for (size_t i = 0; i < _queries.size(); ++i) {
        auto query = _queries[i];

        Timer timer;

        Time arrival_time = csa.query(query.source_id, query.target_id, query.dep);

        double running_time = timer.elapsed();

        res[i] = {query.rank, running_time, arrival_time};

        std::cout << i << std::endl;
    }

    write_results(res);

    Profiler::report();
}
