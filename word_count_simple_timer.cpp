/*******************************************************************************
 * examples/word_count/word_count_simple.cpp
 *
 * Part of Project Thrill - http://project-thrill.org
 *
 * Copyright (C) 2016 Timo Bingmann <tb@panthema.net>
 *
 * All rights reserved. Published under the BSD-2 license in the LICENSE file.
 ******************************************************************************/

#include <thrill/api/read_lines.hpp>
#include <thrill/api/reduce_by_key.hpp>
#include <thrill/api/write_lines.hpp>
#include <tlx/string/split_view.hpp>

#include <iostream>
#include <string>
#include <utility>

//e- add timer and namespace
#include <thrill/common/stats_timer.hpp>
using namespace thrill;

void WordCount(thrill::Context& ctx,
               std::string input, std::string output) {
    using Pair = std::pair<std::string, size_t>;
//e- add timer
common::StatsTimerStart timer;

    auto word_pairs =
        ReadLines(ctx, input)
        .template FlatMap<Pair>(
            // flatmap lambda: split and emit each word
            [](const std::string& line, auto emit) {
                tlx::split_view(' ', line, [&](tlx::string_view sv) {
                                    emit(Pair(sv.to_string(), 1));
                                });
            });
    //e- added timer output
    timer.Microseconds();
    LOG1 << "Worker_ID = " << ctx.local_worker_id()
    << ": time_readLines = "    << timer    << " sec.";

    word_pairs.ReduceByKey(
        // key extractor: the word string
        [](const Pair& p) { return p.first; },
        // commutative reduction: add counters
        [](const Pair& a, const Pair& b) {
            return Pair(a.first, a.second + b.second);
        })
    .Map([](const Pair& p) {
             return p.first + ": "
             + std::to_string(p.second);
         })
    .WriteLines(output);
//e- added timer output
    timer.Seconds();
    LOG1 << "Worker_Id = " << ctx.local_worker_id()
    << ": time_all = "    << timer    << " sec."; 	    
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " <input> <output>" << std::endl;
        return -1;
    }

    return thrill::Run(
        [&](thrill::Context& ctx) { WordCount(ctx, argv[1], argv[2]); });
}

/******************************************************************************/
