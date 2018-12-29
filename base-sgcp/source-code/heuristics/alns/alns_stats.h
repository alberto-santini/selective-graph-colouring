//
// Created by alberto on 14/09/17.
//

#ifndef SGCP_ALNS_STATS_H
#define SGCP_ALNS_STATS_H

#include <vector>
#include <map>
#include <cstdint>

namespace sgcp {
    class ALNSStats {
        std::vector<uint32_t> destroy_methods;
        std::vector<uint32_t> repair_methods;
        std::vector<bool> accepted;
        std::map<uint32_t, float> destroy_pct_accept;
        std::map<uint32_t, float> repair_pct_accept;

        void compute_maps();

    public:
        void add_destroy(uint32_t method_id);
        void add_repair(uint32_t method_id);
        void add_accepted();
        void add_rejected();
        void print_stats();
    };
}

#endif //SGCP_ALNS_STATS_H
