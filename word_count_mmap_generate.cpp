/*******************************************************************************
 * Hermann Hessling (HTW Berlin), Jan. 2020, based on
 * examples/word_count/word_count_simple.cpp
 * Part of Project Thrill - http://project-thrill.org
 * Copyright (C) 2016 Timo Bingmann <tb@panthema.net>
 * All rights reserved. Published under the BSD-2 license in the LICENSE file.
 ******************************************************************************/
#include <thrill/api/reduce_by_key.hpp>
#include <thrill/api/write_lines.hpp>
#include <tlx/string/split_view.hpp>

#include <string>
#include <utility>
#include <vector>

using namespace thrill;

//hh
#include <thrill/api/equal_to_dia.hpp>
#include <thrill/common/stats_timer.hpp>
#include <thrill/api/generate.hpp>
#include <iostream>     // for std::cout, std::endl
#include <unistd.h>     // _SC_PAGE_SIZE is defined and sysconf() is declared
#include <stdlib.h>     // for atoi
#include <stdio.h>      // lseek?
#include <sys/mman.h>   // for mmap (to do: check difference to mmap64)
#include <fcntl.h>      // for open

void WordCount(thrill::Context& ctx,
               const char* srcAddr, size_t file_size, std::string out_file) {

    using Pair = std::pair<std::string, size_t>;

    common::StatsTimerStart time_distribute;

    size_t page_size   = (size_t)sysconf(_SC_PAGESIZE);
    size_t n_page      = file_size / page_size;  // no. of "complete" pages in the input file
    size_t page_delta  = file_size - n_page * page_size ;

    char* tmp_page = (char *) malloc(sizeof(char) * (page_size+1)); // allocate memory on heap
    if ( !tmp_page || tmp_page  == NULL ){
      LOG1 << "Error: tmp_page - memory allocation failed.";
    }

    // Split input data (= memory mapped file) into "small" pages (Linux: 1 page = 4096 B)
    // To do: check the size of items on the period of time_distribute
    auto file_blocks = Generate(ctx, n_page + 1 /* number of items to be generated  */,
		[&]( size_t i_position = 0L ) {
		i_position++;
		size_t size_true = page_size;
                if( i_position == n_page+1 ) size_true = page_delta;
                for( size_t i = 0; i < size_true; i++){
		  *(tmp_page + i) = *(srcAddr + (i_position-1) * page_size + i );
	        };
                // Debug
		// std::cout << "Generate: id=" << i_Worker << ", i_position=" << i_position
                //           << ", size_true=" << size_true << std::endl;
                *(tmp_page + size_true) = '\0'; // add null terminator at end of page
                return tmp_page;
    });

    /*
    // Debug: is memory mapped file distributed correctly to worker?
    file_blocks.Map([&ctx](const std::string& p) {
        return p;
    })
    .WriteLines(out_file);
    */

    time_distribute.Seconds();
    LOG1 << "time_distribute[sec]= "    << time_distribute    ;
    common::StatsTimerStart time_split;

    auto word_pairs = file_blocks.template FlatMap<Pair>(
           // lambda: split line into single words and emit the pair (word, 1)
           [](const std::string& line, auto emit) {
               tlx::split_view(' ', line, [&](tlx::string_view sv) {
                                emit(Pair(sv.to_string(), 1));
           });
    });


    time_split.Seconds();
    LOG1 << "time_split[sec]= "    << time_split    ;
    common::StatsTimerStart time_reduce;


    word_pairs.ReduceByKey(
	[](const Pair& p) { return p.first; }, // lambda_1: extract key = word string
        [](const Pair& a, const Pair& b) {     // lambda_2: ...
	  return Pair(a.first, a.second + b.second); //  ... update value = word counter
        })
    .Map([&ctx](const Pair& p) {                // lambda: input = context ctx
	size_t wID = ctx.local_worker_id();     //  ...    determine ID of worker ...
                                                //  ... convert ID + Pair to string
        return "worker_" + std::to_string(wID) + ": " +	p.first + ": " + std::to_string(p.second);
        })
    .WriteLines(out_file)
    ;

    time_reduce.Seconds();
    LOG1 << "time_reduce[sec]= " << time_reduce  ;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " <filePath_input> <filePath_output> " << std::endl;
        return -1;
    }

    common::StatsTimerStart timer_prepare;

    // access input data stored on disc or FAM
    int in_fd = open(argv[1], O_RDONLY);

    // length of file
    size_t file_size = lseek(in_fd, 0, SEEK_END);

    // MAP_PRIVATE: mapping needs reservation of memory
    //              => size of file limited to RAM (physical memory) + SWAP (virtual memory)
    // MAP_SHARED: changes "in-memory" are written back to file AND kernel can free
    //             up memory - if needed - by writing back to file
    const char* srcAddr;
    srcAddr = static_cast<char*>( mmap(NULL, file_size, PROT_READ, MAP_SHARED, in_fd, 0L ) );
    if (srcAddr == MAP_FAILED){
      std::cout << "--- Error: mmap failed  " << std::endl;
      return -1 ;
    }

    timer_prepare.Seconds();
    std::cout << "[main] time_prepare[sec]= " << timer_prepare << std::endl;

    return thrill::Run(  // Launch Thrill program: the lambda function is executed on each worker.
		       [&](thrill::Context& ctx) { WordCount(ctx,
			   srcAddr /* call memory mapped data by address => fast */,
		           file_size, argv[2]); });
}
