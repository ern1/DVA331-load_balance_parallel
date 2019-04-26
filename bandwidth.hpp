#ifndef __BANDWIDTH_HPP__
#define __BANDWIDTH_HPP__

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "perfCounters.hpp"

#define STR_SIZE 1000

// Ska vi bara göra assign-funktioner för 4 kärnor? Eller fixa andra för t.ex. 2 kärnor?

// 
void use_best_effort(bool use)
{
	
}

// Assign bandwidth using percentages
void assign_bw(int core1_bw, int core2_bw, int core3_bw, int core4_bw)
{
	if (system(NULL)) puts ("Ok");
		else exit (EXIT_FAILURE);

	char script_str[STR_SIZE] = {0};
	//snprintf(script_str, sizeof(script_str), "%s %d %d %d %d %s", ?????);

	int status = system(script_str);
	if (status < 0)
		printf("%s\n", strerror(errno));
}

// Assign bandwidth by specifying bandwidth in MB
void assign_bw_MB(int core1_bw, int core2_bw, int core3_bw, int core4_bw)
{
	if (system(NULL)) puts ("Ok");
		else exit (EXIT_FAILURE);

	char script_str[STR_SIZE] = {0};
	//snprintf(script_str, sizeof(script_str), "%s %d %d %d %d %s", ?????);

	int status = system(script_str);
	if (status < 0)
		printf("%s\n", strerror(errno));
}

// Set total bandwidth that can be partitionen between cores
void set_max_bw(int max_bw)
{
	if (system(NULL)) puts ("Ok");
		else exit (EXIT_FAILURE);

	char script_str[STR_SIZE] = {0};
	snprintf(script_str, sizeof(script_str), "%s %d %s", "echo maxbw ", max_bw, " > /sys/kernel/debug/memguard/control");

	int status = system(script_str);
	if (status < 0)
		printf("%s\n", strerror(errno));
}

// Calculate used bandwidth by using performance counters
unsigned calculate_bandwidth(unsigned l3_misses, unsigned prefetch_misses, unsigned cache_line_size)
{
	// Finns det bättre perf event att använda sig av här?
	return (l3_misses + prefetch_misses) * cache_line_size;
}

// Parition bandwidth between different cores
void partition_bandwidth(const ThreadInfo* th, int num_threads)
{
	/* Vad denna funktion ska göra (preliminärt):
	** 1. Börja med att räkna ut bandbredd för varje kärna (calculate_bandwidth)
	** 2. Räkna ut procentuella delen för varje kärna. (Blir det verkligen så enkelt?)
	** 3. Funktionsanrop till assign_bw
	*/

	int cache_line_size = sysconf(_SC_LEVEL3_CACHE_LINESIZE);
	int used_bw[num_threads], new_core_bw[num_threads];

	// Calculate bandwidth used by each thread
	for (int i = 0; i < num_threads; i++){
		used_bw[i] = calculate_bandwidth(th->l3_misses, th->prefetch_misses, cache_line_size);
	}

	// Calculate how to partition bandwidth between different cores
	// ...

	// Partition bandwidth
	//if (num_threads == 4) // Lägg till dessa om vi behöver köra med olika antal trådar.
	assign_bw(new_core_bw[0], new_core_bw[1], new_core_bw[2], new_core_bw[3]);
}

#endif
