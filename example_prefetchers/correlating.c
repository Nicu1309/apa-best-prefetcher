//
// Data Prefetching Championship Simulator 2
// Seth Pugsley, seth.h.pugsley@intel.com
//

/*

  This file does NOT implement any prefetcher, and is just an outline

 */

#include <stdio.h>
#include "../inc/prefetcher.h"

#define NUM_ENTRIES_GHB 256
#define NUM_ENTRIES_IT 128
#define PREFETCH_DEGREE 2  // We prefetch 2 + 1. PREFETCH_DEGREE indicates how many accesses we are doing recursively

typedef struct index_entry{

  unsigned long long int ip;
  unsigned int pointer;

} index_entry_t;

typedef struct ghb_entry {
	unsigned long long int miss_address;
        int stride;
        unsigned int prev_entry;
} ghb_entry_t;

index_entry_t index_table[NUM_ENTRIES_IT];
ghb_entry_t global_history_buffer[NUM_ENTRIES_GHB];

unsigned int it_head = 0, ghb_head = 0;

void l2_prefetcher_initialize(int cpu_num)
{
  printf("Stride-Correlating prefetcher\n");
  // you can inspect these knob values from your code to see which configuration you're runnig in
  printf("Knobs visible from prefetcher: %d %d %d\n", knob_scramble_loads, knob_small_llc, knob_low_bandwidth);

  // Init stride buffer
  for(int i = 0; i < NUM_ENTRIES_IT; i++){
    index_table[i].ip = 0;
    index_table[i].pointer = 0;
  }

  // Init GHB 
  for(int i = 0; i < NUM_ENTRIES_GHB; i++){
    global_history_buffer[i].miss_address = 0;
    global_history_buffer[i].stride = 0;
    global_history_buffer[i].prev_entry = 0;
  }

}

void l2_prefetcher_operate(int cpu_num, unsigned long long int addr, unsigned long long int ip, int cache_hit)
{
  // uncomment this line to see all the information available to make prefetch decisions
  //printf("(0x%llx 0x%llx %d %d %d) ", addr, ip, cache_hit, get_l2_read_queue_occupancy(0), get_l2_mshr_occupancy(0));

  // Find ip if exists
  int entry = -1;
  for(int i = 0; i < NUM_ENTRIES_GHB; i++){
    if( index_table[i].ip == ip ){
      entry = i;
    }
  }
  //printf("GHB %d  IT %d \n", ghb_head, it_head); 
  if( entry < 0 ){
    
    global_history_buffer[ghb_head].miss_address = addr;  
    global_history_buffer[ghb_head].stride = 0;  
    global_history_buffer[ghb_head].prev_entry = 0;  
    index_table[it_head].ip = ip;
    index_table[it_head].pointer = ghb_head;
    // Module operation does not work if NUM_ENTRIES_IT or NUM_ENTRIES_GHB is not a power of 2.
    it_head = (it_head + 1) & (NUM_ENTRIES_IT - 1);
    ghb_head = (ghb_head + 1) & (NUM_ENTRIES_GHB - 1);
    return;

  } else {
    
    unsigned int ghb_entry = index_table[entry].pointer;
    unsigned long long int prev_addr = global_history_buffer[ghb_entry].miss_address;
   
    // Calculate new stride 
    int stride = 0;
    if ( addr > prev_addr ) {
      stride = addr - prev_addr;
    } else {
      stride = prev_addr - addr;
      stride *= -1;
    }
    
    if (stride != 0){
      // Update GHB with new stride and point to prev entry
      global_history_buffer[ghb_head].miss_address = addr;  
      global_history_buffer[ghb_head].stride = stride;  
      global_history_buffer[ghb_head].prev_entry = ghb_entry;
      index_table[entry].pointer = ghb_head;
      ghb_head = (ghb_head + 1) & (NUM_ENTRIES_GHB - 1);
      
      unsigned long long int pf_address = addr + stride;
      // only issue a prefetch if the prefetch address is in the same 4 KB page
      // as the current demand access address
      if((pf_address>>12) != (addr>>12))
    	    return;
      // check the MSHR occupancy to decide if we're going to prefetch to the L2 or LLC
      if(get_l2_mshr_occupancy(0) < 8) {
    	  l2_prefetch_line(0, addr, pf_address, FILL_L2);
      } else {
          l2_prefetch_line(0, addr, pf_address, FILL_LLC);
      }

    //  
      int pf_entry = global_history_buffer[index_table[entry].pointer].prev_entry;
      for(int i = 0; i < PREFETCH_DEGREE; i++) {
          pf_address = global_history_buffer[pf_entry+i].miss_address + stride; 
    	  // only issue a prefetch if the prefetch address is in the same 4 KB page
    	  // as the current demand access address
    	  if((pf_address>>12) != (addr>>12))
    	    {
    	      break;
    	    }
            //printf("Prefetch cycle %llu addr: %llu pf: %llu \n", get_current_cycle(0), addr, pf_address);
    	  // check the MSHR occupancy to decide if we're going to prefetch to the L2 or LLC
    	  if(get_l2_mshr_occupancy(0) < 8)
    	    {
    	      l2_prefetch_line(0, addr, pf_address, FILL_L2);
    	    }
    	  else
    	    {
    	      l2_prefetch_line(0, addr, pf_address, FILL_LLC);
    	    }

    	}
    }
  }

}

void l2_cache_fill(int cpu_num, unsigned long long int addr, int set, int way, int prefetch, unsigned long long int evicted_addr)
{
  // uncomment this line to see the information available to you when there is a cache fill event
  //printf("0x%llx %d %d %d 0x%llx\n", addr, set, way, prefetch, evicted_addr);
}

void l2_prefetcher_heartbeat_stats(int cpu_num)
{
  printf("Prefetcher heartbeat stats\n");
}

void l2_prefetcher_warmup_stats(int cpu_num)
{
  printf("Prefetcher warmup complete stats\n\n");
}

void l2_prefetcher_final_stats(int cpu_num)
{
  printf("Prefetcher final stats\n");
}
