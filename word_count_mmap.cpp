/*******************************************************************************
 * Hermann Hessling (HTW Berlin), Dec. 2019, based on
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
#include <iostream>     // for std::cout, std::endl
// #include <fstream>   // for ifstream
#include <boost/iostreams/device/mapped_file.hpp>

#include <stdlib.h>     // for atoi
#include <sys/mman.h>   // for mmap   mmap64: library libc  (use the -l c option [size_t len; off64_t off[]
#include <sys/types.h>  // for mmap64 ?
#include <fcntl.h>      // for open



void WordCount(thrill::Context& ctx,
               std::vector<std::string>  v_input, std::string out_file) {
    using Pair = std::pair<std::string, size_t>;
    
common::StatsTimerStart timer_all_wc;
    common::StatsTimerStart timer_distribute;

    // distribute components of vector to workers (evenenly)
    auto word_line  = EqualToDIA(ctx, v_input);

    timer_distribute.Seconds();
    LOG1 << "time_distribute[sec]= "    << timer_distribute    ;
    common::StatsTimerStart timer_split;

    auto word_pairs = word_line.template FlatMap<Pair>(
           // lambda: split line into single words and emit the pair (word, 1)
           [](const std::string& line, auto emit) {
               tlx::split_view(' ', line, [&](tlx::string_view sv) {
                                emit(Pair(sv.to_string(), 1));
           });
        });

    timer_split.Seconds();
    LOG1 << "time_split[sec]= "    << timer_split    ;
    common::StatsTimerStart timer_reduce;

    word_pairs.ReduceByKey(
	[](const Pair& p) { return p.first; }, // lambda_1: extract key = word string
        [](const Pair& a, const Pair& b) {     // lambda_2: ...
	  return Pair(a.first, a.second + b.second); //  ... update word counter
        })
    .Map([&ctx](const Pair& p) {                // lambda: input = context ctx
	size_t wID = ctx.local_worker_id();     //  ...    determine ID of worker ...
                                                // ... convert ID + Pair to string
  	return "worker_" + std::to_string(wID) + ": " +	p.first + ": " + std::to_string(p.second);
        })
    .WriteLines(out_file);

    timer_reduce.Seconds();
    LOG1 << "time_reduce[sec]= " << timer_reduce  ;
    timer_all_wc.Seconds();
    LOG1 << "timer_all_wc[sec]= " << timer_all_wc  ;
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cout << "Usage: " << argv[0] << " <filePath_input> <filePath_output> <nWorker> " << std::endl;
        return -1;
    }
    common::StatsTimerStart timer_all_main;
    common::StatsTimerStart timer_prepare;

    size_t nWorker    = (size_t) atoi(argv[3]);
    //size_t line_Bytes = (size_t) atoi(argv[4]);

    // access input data stored on disc or FAM
    int in_fd = open(argv[1], O_RDONLY);

    // length of file
    size_t file_size = lseek(in_fd, 0, SEEK_END);
    lseek(in_fd, 0, SEEK_SET); // Return cursor to begin of file

    // MAP_PRIVATE: mapping needs reservation of memory => size of file limited to RAM (physical memory) + SWAP (virtual memory)
    // MAP_SHARED: changes "in-memory" are written back to file  AND kernel can free up memory - if needed - by writing back to file
    const char* srcAddr = static_cast<char*>( mmap(NULL, file_size, PROT_READ, MAP_SHARED, in_fd, 0L ) );
    if (srcAddr == MAP_FAILED){
      std::cout << "--- Error: mmap failed  " << std::endl;
      return -1 ;
    }


    // copy memory-mapped file page-wise into a vector, where size of page = file_size / nWorker
    size_t delta_file_size = file_size / nWorker ;

    std::vector<std::string> inVector; // = empty container for strings
    inVector.reserve(file_size);       // to speed up push_back method, which is used below

    char* tmp_page = (char *) malloc(delta_file_size + 0); // allocate memory on heap
                                                  // + 1 ?  => magic byte -> no "Segmentation fault: 11" for large input files
    if ( !tmp_page || tmp_page == NULL )
      std::cout << "Error: tmp_page - memory allocation failed. " <<  std::endl;

    for( size_t n = 0; n < nWorker; n++) {
      for( size_t i = 0; i < delta_file_size; i++){
        *(tmp_page+i) = *(srcAddr + n*delta_file_size + i);
      };
      inVector.push_back(tmp_page); // = add string tmp_page at the ende of inVector
    };

    // If file_size % nWorker != 0: do not forget the remaining chars at the end of the input file
    size_t remainder_size = file_size - nWorker*delta_file_size ;
    char*  tmp_page_remain = (char *) malloc(remainder_size + 0); // allocate memory on heap
    if ( !tmp_page_remain || tmp_page_remain == NULL )
      std::cout << "Error: tmp_page_remain - memory allocation failed. " <<  std::endl;
    if( remainder_size > 0){
      for( size_t i = 0; i < remainder_size ; i++){
        *(tmp_page_remain+i) = *(srcAddr + nWorker*delta_file_size + i);
      };
      inVector.push_back(tmp_page_remain); // = add string tmp_page_remain at the ende of inVector
    }

    /*   // check inVector:
    for_each(inVector.begin(), inVector.end(), [] (std::string& s){ std::cout << s << " "; });
    std::cout << "[main] inVector: size = " << inVector.size()
	      << ",  capacity = " << inVector.capacity()
	      << ",  max_size = " << inVector.max_size()  << std::endl;
    */

    timer_prepare.Seconds();
    std::cout << "[main] time_prepare[sec]= " << timer_prepare << std::endl;

    // garbage collection
    free( tmp_page );
    free( tmp_page_remain );
    munmap( (void *)srcAddr, file_size);
    close(in_fd);

    return thrill::Run(  // Launch Thrill program: the lambda function is executed
                         // on each worker.
		       [&](thrill::Context& ctx) { WordCount(ctx, inVector, argv[2]); } );

}
