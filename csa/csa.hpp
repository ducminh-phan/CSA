#ifndef CSA_HPP
#define CSA_HPP

#include <vector>

#include "data_structure.hpp"

class ConnectionScan {
private:
    const Timetable* const _timetable;

public:
    explicit ConnectionScan(const Timetable* timetable_p) : _timetable {timetable_p} {};

    Time query(const node_id_t& source_id, const node_id_t& target_id, const Time& departure_time) const;

    Time backward_query(const node_id_t& source_id, const node_id_t& target_id,
                        const Time& arrival_time) const;
};

#endif // CSA_HPP
