//
// Created by alberto on 08/05/17.
//

#include "cache.hpp"
#include "stream_utils.hpp"
#include "string_utils.hpp"
#include <sys/stat.h>
#include <string>
#include <vector>
#include <fstream>
#include <cstdio>
#include <iterator>

namespace sgcp {
    namespace cache {
        static const csv_filenames bks_cf{"bks.csv", "tmpbks.csv", "bks.bak"};
        static const csv_filenames init_cf{"init.csv", "tmpinit.csv", "init.bak"};

        bool is_same_file(const std::string s1, const std::string s2) {
            // Do not try this at home!
            auto path1 = string_utils::split(s1, '/');
            auto path2 = string_utils::split(s2, '/');

            return path1.back() == path2.back();
        }

        bool index_exists(const csv_filenames& cf) {
            struct stat _;
            return stat(cf.index, &_) == 0;
        }

        void create_index(const csv_filenames& cf) {
            std::ofstream{cf.index};
        }

        void update_pool(const csv_filenames& cf, ColumnPool& pool, const Graph& g) {
            if(!index_exists(cf)) { return; }

            std::ifstream idx{cf.index};
            auto _ = stream_utils::remember_to_close(idx);

            std::string line;
            while(std::getline(idx, line)) {
                auto tokens = string_utils::split(line, ';');

                if(tokens.empty()) { continue; }

                if(is_same_file(tokens[0], g.data_filename)) {
                    assert(tokens.size() > 1u);
                    std::stringstream ss;

                    for(auto i = 1u; i < tokens.size(); ++i) {
                        auto vertices = string_utils::split(tokens[i], ',');

                        VertexIdSet vertexset{};

                        for(auto vstr : vertices) {
                            ss.str(vstr);
                            uint32_t v = 0u;

                            if(ss >> v) {
                                vertexset.insert(v);
                            }

                            ss.clear();
                        }

                        assert(!vertexset.empty());

                        StableSet stableset{g, vertexset};
                        if(std::find(pool.begin(), pool.end(), stableset) == pool.end()) {
                            pool.push_back(stableset);
                        }
                    }

                    return;
                }
            }
        }

        void bks_update_pool(ColumnPool& pool, const Graph& g) {
            update_pool(bks_cf, pool, g);
        }

        void init_update_pool(ColumnPool& pool, const Graph& g) {
            update_pool(init_cf, pool, g);
        }

        std::stringstream pool_to_ss(const ColumnPool& pool, const Graph& g) {
            std::stringstream ss;
            ss << g.data_filename << ";";

            for(auto cid = 0u; cid < pool.size(); ++cid) {
                const auto& s = pool[cid].get_set();
                std::vector<uint32_t> colour(s.begin(), s.end());

                for(auto vid = 0u; vid < colour.size(); ++vid) {
                    ss << colour[vid];
                    if(vid < colour.size() - 1) { ss << ","; }
                }

                if(cid < pool.size() - 1) { ss << ";"; }
            }

            return ss;
        }

        void update_cache(const csv_filenames &cf, const ColumnPool &pool, const Graph &g) {
            if(!index_exists(cf)) {
                create_index(cf);
            }

            std::ifstream idxin{cf.index};
            std::ofstream idxout{cf.tmp};

            std::string line;
            bool updated = false;
            while(std::getline(idxin, line)) {
                if(!updated) {
                    auto tokens = string_utils::split(line, ';');

                    if(tokens.empty()) { continue; }

                    if(is_same_file(tokens[0], g.data_filename)) {
                        assert(tokens.size() > 1u);

                        if(tokens.size() - 1u > pool.size()) {
                            auto ss = pool_to_ss(pool, g);
                            idxout << ss.str() << "\n";
                        } else {
                            idxout << line << "\n";
                        }

                        updated = true;
                        continue;
                    }
                }

                idxout << line << "\n";
            }

            if(!updated) {
                auto ss = pool_to_ss(pool, g);
                idxout << ss.str() << "\n";
            }

            idxin.close();
            idxout.close();

            std::remove(cf.bak);
            std::rename(cf.index, cf.bak);
            std::remove(cf.index);
            std::rename(cf.tmp, cf.index);
            std::remove(cf.tmp);
        }

        void bks_update_cache(const ColumnPool &pool, const Graph &g) {
            update_cache(bks_cf, pool, g);
        }

        void init_update_cache(const ColumnPool &pool, const Graph &g) {
            update_cache(init_cf, pool, g);
        }
    }
}