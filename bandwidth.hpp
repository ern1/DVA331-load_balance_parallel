#ifndef __BANDWIDTH_HPP__
#define __BANDWIDTH_HPP__

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "perfCounters.hpp"

#define STR_SIZE 1000
#define BW_VAL_THRESHOLD 5

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
inline static double calculate_bandwidth_MB(long long l3_misses, long long prefetch_misses, int cache_line_size)
{
	long long bw_b = (l3_misses + prefetch_misses) * cache_line_size;
	return (double)(bw_b * 1.1920928955078125e-7);
}

// Parition bandwidth between different cores
void partition_bandwidth(ThreadInfo* th, int num_threads)
{
	int cache_line_size = sysconf(_SC_LEVEL3_CACHE_LINESIZE);
	double used_bw, used_ewma_bw[num_threads];
	int new_core_bw[num_threads]; // in percent
	int coefficent = 1;

	/* Calculate bandwidth used by each thread */
	for (int i = 0; i < num_threads; i++){
		used_bw = calculate_bandwidth_MB(th[i].l3_misses, th[i].prefetch_misses, cache_line_size);
		std::cout << "Core: " << i << ", bw: " << used_bw << '\n';

		// Add current used_bw to prev_used_bw and delete oldest
		/*th[i].prev_used_bw.push_back(used_bw);

		if(th[i].prev_used_bw.size >= BW_VAL_THRESHOLD)
			th[i].prev_used_bw.erase(th[i].prev_used_bw.begin());*/
		
		// Calculate EWMA (Moving Áverage BW) for each core
		/*for(int j = th[i].prev_used_bw.size(); j > 0; j--)
		{

			if(j == 5)
				used_ewma_bw[i] += th[i].prev_used_bw[j];
			else
			{
				coefficent * th[i].prev_used_bw[j] + (1 - coefficent) * 
			}
		}*/
		

		// int n = th[i].runs < BW_VAL_THRESHOLD ? h[i].runs : BW_VAL_THRESHOLD;
		// int m = 2 / (1 + n);
		// th[i].ewma_bw = m * used_bw + (1 - m) * th[i].ewma_bw;
	}
	std::cout << '\n';
	
	/* Calculate how to partition bandwidth between different cores */
	

	/* Partition bandwidth */
	//if (num_threads == 4) // Lägg till dessa om vi behöver köra med olika antal trådar.
	//assign_bw(new_core_bw[0], new_core_bw[1], new_core_bw[2], new_core_bw[3]);
}

#endif
