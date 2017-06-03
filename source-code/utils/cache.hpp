#ifndef _BKS_HPP
#define _BKS_HPP

#include <sstream>
#include "../graph.hpp"
#include "../branch-and-price/column_pool.hpp"

namespace sgcp {
    namespace cache {
        struct csv_filenames {
            const char* index;
            const char* tmp;
            const char* bak;
        };

        bool is_same_file(const std::string s1, const std::string s2);

        bool index_exists(const csv_filenames& cf);
        void create_index(const csv_filenames& cf);

        void update_pool(const csv_filenames& cf, ColumnPool& pool, const Graph& g);
        void bks_update_pool(ColumnPool& pool, const Graph& g);
        void init_update_pool(ColumnPool& pool, const Graph& g);

        void update_cache(const csv_filenames &cf, const ColumnPool &pool, const Graph &g);
        void bks_update_cache(const ColumnPool &pool, const Graph &g);
        void init_update_cache(const ColumnPool &pool, const Graph &g);

        std::stringstream pool_to_ss(const ColumnPool& pool, const Graph& g);
    }
}

#endif