//
// Data Prefetching Championship Simulator 2
// Seth Pugsley, seth.h.pugsley@intel.com
//

/*

  This file does NOT implement any prefetcher, and is just an outline

 */

#include <stdio.h>
#include "../inc/prefetcher.h"
#define PREFETCH_DEGREE 3
#define NUM_ENTRIES 1024
#define NUM_WAYS 1

//287302 -> MSHR threshold 8
//284244 -> NEXT LINE
//287414 -> MSHR threshold 10

typedef struct prefetcher_field {

  unsigned long long int ip_tag;

  short stride;

  unsigned long long int addr;

  int lru;

} pf_entry_t;

pf_entry_t pf_table[NUM_ENTRIES][NUM_WAYS];

void l2_prefetcher_initialize(int cpu_num)
{
  printf("No Prefetching\n");
  // you can inspect these knob values from your code to see which configuration you're runnig in
  printf("Knobs visible from prefetcher: %d %d %d\n", knob_scramble_loads, knob_small_llc, knob_low_bandwidth);

  for(int i = 0; i < NUM_ENTRIES; i++)
    for(int w = 0; w < NUM_WAYS; w++)
    {
      pf_table[i][w].ip_tag = 0;
      pf_table[i][w].stride = 0;
      pf_table[i][w].addr = 0;
      pf_table[i][w].lru = -1;
    }
}

void l2_prefetcher_operate(int cpu_num, unsigned long long int addr, unsigned long long int ip, int cache_hit)
{

  /*unsigned long long int cl_address = addr>>6;
  unsigned long long int page = cl_address>>6;
  int page_offset = cl_address&63;*/

  unsigned char index_set = ip&63;
  unsigned long ip_tag = ip>>6;
  unsigned long long int pf_address;
  // uncomment this line to see all the information available to make prefetch decisions
  //printf("(0x%llx 0x%llx %d %d %d) ", addr, ip, cache_hit, get_l2_read_queue_occupancy(0), get_l2_mshr_occupancy(0));

  /* LRU policy */
  int lru_entry = pf_table[index_set][0].lru;
  for(int w = 1; w < NUM_WAYS; w++)
    if(pf_table[index_set][w].lru < lru_entry)
    {
        lru_entry = w;
    }

  /* Select way */
  int way = -1;
  for(int w = 0; w < NUM_WAYS; w++)
    if(pf_table[index_set][w].ip_tag == ip_tag)
    {
        way = w;
        break;
    }

  /* Replace entry */
  if (way < 0)
  {
    pf_table[index_set][lru_entry].ip_tag = ip_tag;
    pf_table[index_set][lru_entry].stride = 0;
    pf_table[index_set][lru_entry].addr = addr;
    pf_table[index_set][lru_entry].lru = get_current_cycle(0);
    way = 0;
    //return;
  }
  printf("Index: %d  - Way: %d\n", index_set, way);
  short stride;
  if (addr > pf_table[index_set][way].addr)
    stride = addr - pf_table[index_set][way].addr;
  else
    stride = -1*(pf_table[index_set][way].addr - addr);

  /* Existed but not initialized */
  unsigned char do_prefetch = 0;

  pf_table[index_set][way].ip_tag = ip_tag;
  pf_table[index_set][way].addr = addr;
  pf_table[index_set][way].lru = get_current_cycle(0);

  // Check if stride is initialized
  // Check if prefetch based on stride pattern. Only if seen twice
  if(pf_table[index_set][way].stride != 0)
    if (stride == pf_table[index_set][way].stride)
      do_prefetch = 1;

  pf_table[index_set][way].stride = stride;
  if (do_prefetch == 0){
    return;
  }
  for (int p = 1; p <= PREFETCH_DEGREE; p++)
  {
    printf("Adios\n");
    pf_address = ((addr>>6)+(stride*p))<<6;

    printf("Prefetch cycle %llu addr: %llu pf: %llu", get_current_cycle(0), addr, pf_address);
  	if(get_l2_mshr_occupancy(0) > 8)
  	{
  		// conservatively prefetch into the LLC, because MSHRs are scarce
  		l2_prefetch_line(0, addr, pf_address, FILL_LLC);
  	}
  	 else
     {
  		// MSHRs not too busy, so prefetch into L2
  		l2_prefetch_line(0, addr, pf_address, FILL_L2);
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
