#include <cstdint>
#include <vector>
#include <iostream>
#include <thread>

#include "argagg/argagg.hpp"
#include "fir_coeff.h"

// 64-bit LFSR with maximal-length sequence
// polynomial x^64 + x^63 + x^61 + x^60 + 1
// at 100 MHz sample rate, repeats in 2924.7 years
class LFSR64
{
public:
  LFSR64():
    sr(1)
  {}
  
  void fill_buffer(std::vector<uint64_t> &buffer)
  {
    uint64_t *bp = buffer.data();
    unsigned len = buffer.size();
    for (unsigned k = 0; k < len; k++){
      bool b = (((sr & (1UL<<63))>>63) ^ 
                ((sr & (1UL<<62))>>62) ^
                ((sr & (1UL<<60))>>60) ^ 
                ((sr & (1UL<<59))>>59));
      sr = (sr << 1) | b;
      *bp++ = sr;
    }
  }

private:
  uint64_t sr;
};

// fast filtering of LFSR output
// apply a 64-tap FIR filter to saved states of 64-bit LFSR
// using table lookup for parallel multiply-accumulate
// in multiple threads
class Filter
{
public:
  Filter(std::vector<int32_t> &coeff, int shift):
    tables(8*256),
    shift(shift)
  {
    create_tables(coeff);
  }

  // use n_threads threads to filter shift-register state into
  //   flat-spectrum 8-bit signed output samples
  void filter(std::vector<uint64_t> &in, std::vector<int8_t> &out,
              int n_threads)
  {
    std::vector<std::thread> workers;
    
    unsigned len = in.size();
    int worker_len = len / n_threads;
    for (int i=0; i<n_threads-1; ++i){
      workers.push_back(std::thread(&Filter::filter_worker,
                                    this,
                                    worker_len,
                                    in.data() + i*worker_len,
                                    out.data() + i*worker_len));
    }
    // last worker may handle a different length
    workers.push_back(std::thread(&Filter::filter_worker,
                                  this,
                                  len - (n_threads-1)*worker_len,
                                  in.data() + (n_threads-1)*worker_len,
                                  out.data() + (n_threads-1)*worker_len));
    
    for (int i=0; i<n_threads; ++i){
      workers[i].join();
    }
  }

private:
  std::vector<int32_t> tables;
  int shift;

  // create 8-bit lookup tables to return 8 multiply-accumulates for
  //   each byte position in shift register word
  void create_tables(std::vector<int32_t> &coeff)
  {
    const int n_bits = 64;
    int n_tables = n_bits / 8;
    for (int i=0; i<n_tables; ++i){
      for (int j=0; j<256; ++j){
        int32_t sum = 0;
        for (int k=0; k<8; ++k){
          sum += (j & (1<<k)) ? +coeff[i*8 + k] : -coeff[i*8 + k];
        }
        tables[i*256+j] = sum;
      }
    }
  }

  // use table lookup to calculate FIR filtered output for 64-bit
  //  shift register states
  void filter_worker(int len, uint64_t* in, int8_t* out)
  {
    for (int i=0; i<len; ++i){
      uint64_t word = *in++;
      int32_t sum = 0;
      uint8_t* bytes = reinterpret_cast<uint8_t*>(&word);
      
      for (int j=0; j<8; ++j){
        sum += tables[j*256 + *bytes++];
      }
      
      *out++ = sum >> shift;
    }
  }

};

int main(int argc, char **argv)
{
  argagg::parser argparser {{
      {"help", {"-h", "--help"},
          "shows this help message", 0},
      {"threads", {"-t", "--threads"},
          "number of threads to use (default = 4)", 1},
      {"buffer", {"-b", "--buffer"},
          "buffer size log 2 (size = 2^N), (default = 17)", 1}
    }};

  argagg::parser_results args;
  try {
    args = argparser.parse(argc, argv);
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  if (args["help"]){
    argagg::fmt_ostream fmt(std::cerr);
    fmt << "noisegen: generate white noise pre-equalized " <<
      "for zero-order-hold DAC (8-bit signed values)\n\n" <<
      "Usage: noisegen [options]" << 
      std::endl << argparser;
    return EXIT_SUCCESS;
  }

  int n_threads = 4;
  if (args["threads"]){
    n_threads = args["threads"];
  }

  int buflog2 = 17;
  if (args["buffer"]){
    buflog2 = args["buffer"];
  }

  LFSR64 lfsr;
  Filter filter(coeff, fir_shift);
  
  std::cout.setf(std::ios::unitbuf);

  const int buf_size = 1UL<<buflog2;
  std::vector<uint64_t> sr_state(buf_size);
  std::vector<int8_t> filtered(buf_size);

  while(1){
    lfsr.fill_buffer(sr_state);
    filter.filter(sr_state, filtered, n_threads);
    std::cout.write(reinterpret_cast<char*>(filtered.data()), buf_size);
  }

  return EXIT_SUCCESS;
}
