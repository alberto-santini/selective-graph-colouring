//
// Created by alberto on 14/09/17.
//

#include "alns_stats.h"

#include <iostream>
#include <cassert>

namespace sgcp {
    void ALNSStats::print_stats() {
        compute_maps();

        std::cout << "Repair methods:" << std::endl;
        for (auto [key, val] : repair_pct_accept) {
            std::cout << key << ", " << val << std::endl;
        }
        std::cout << "Destroy methods:" << std::endl;
        for (auto [key, val] : destroy_pct_accept) {
            std::cout << key << ", " << val << std::endl;
        }
    }

    void ALNSStats::compute_maps() {
        assert(repair_methods.size() == destroy_methods.size());
        assert(accepted.size() == destroy_methods.size());

        auto has_key = [] (const auto& map, uint32_t key) -> bool {
            return map.find(key) != map.end();
        };

        auto set_val = [&has_key] (auto& map, uint32_t key, float val) -> void {
            if (!has_key(map, key)) { map[key] = 0.0f; }
            map[key] += val;
        };

        std::map<uint32_t, float> destroy_tot;
        std::map<uint32_t, float> repair_tot;

        for (auto i = 0u; i < accepted.size(); ++i) {
            auto destroy = destroy_methods[i];
            auto repair = repair_methods[i];

            set_val(destroy_tot, destroy, 1.0);
            set_val(repair_tot, repair, 1.0);

            if (accepted[i]) {
                set_val(destroy_pct_accept, destroy, 1.0);
                set_val(repair_pct_accept, repair, 1.0);
            }
        }

        for(auto [key, val] : destroy_pct_accept) {
            std::cout << "destroy method " << key << " was called " << destroy_tot[key] << " times, and the solution was accepted " << val << " times." << std::endl;
            destroy_pct_accept[key] = val / destroy_tot[key];
        }

        for(auto [key, val] : repair_pct_accept) {
            std::cout << "repair method " << key << " was called " << repair_tot[key] << " times, and the solution was accepted " << val << " times." << std::endl;
            repair_pct_accept[key] = val / repair_tot[key];
        }
    }

    void ALNSStats::add_destroy(uint32_t method_id) {
        destroy_methods.push_back(method_id);
    }

    void ALNSStats::add_repair(uint32_t method_id) {
        repair_methods.push_back(method_id);
    }

    void ALNSStats::add_accepted() {
        accepted.push_back(true);
    }

    void ALNSStats::add_rejected() {
        accepted.push_back(false);
    }
}