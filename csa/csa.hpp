#ifndef CSA_HPP
#define CSA_HPP

#include <vector>

#include "data_structure.hpp"

class ConnectionScan {
private:
    const Timetable* const _timetable;
    bool _use_hl;

public:
    ConnectionScan(const Timetable* timetable_p, bool use_hl) :
            _timetable {timetable_p}, _use_hl {use_hl} {};

    Time query(const node_id_t& source_id, const node_id_t& target_id, const Time& departure_time) const;

    Time backward_query(const node_id_t& source_id, const node_id_t& target_id,
                        const Time& arrival_time) const;
};

#endif // CSA_HPP
