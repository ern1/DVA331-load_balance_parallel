#ifndef __BANDWIDTH_HPP__
#define __BANDWIDTH_HPP__

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <fstream>

#define STR_SIZE 1000
#define BW_VAL_THRESHOLD 5

int cache_line_size = sysconf(_SC_LEVEL3_CACHE_LINESIZE);
int max_bw;

struct ThreadInfo {
	int core_id;
	double execution_time; 		// in seconds
	double bw_percentage; 		// in MB/s
	std::vector<double> prev_used_bw;
};

inline void print_thread_info(const ThreadInfo* th, int size){
  std::cout << std::setw(10) << "Core id" << std::setw(22) << "Execution time (ms)" << "Bandwidth (%)" <<'\n';

  for (int i = 0; i < size; i++)
    std::cout << std::setw(10) << th[i].core_id << std::setw(22) << th[i].execution_time * 1000 << th[i].bw_percentage << '\n';
}

 

void get_bw_from_memguard(double* bw)
{
	if(system(NULL)) puts ("Ok");
		else exit (EXIT_FAILURE);

	int status = system("tail -1 /sys/kernel/debug/memguard/usage > usage_bw");
	if(status < 0)
		printf("%s\n",strerror(errno));

	std::ifstream f;
	int value, i = 0;

	f.open("usage_bw");
	while(f >> value){
		bw[i++] = value;
	}

}
// use mode = 0 to disable best-effort
void set_exclusive_mode(int mode)
{
	if (system(NULL)) puts ("Ok");
		else exit (EXIT_FAILURE);

	char script_str[STR_SIZE] = {0};
	snprintf(script_str, sizeof(script_str), "%s %d %s",
		"echo exclusive", mode, "> /sys/kernel/debug/memguard/control");

	int status = system(script_str);
	if (status < 0)
		printf("%s\n", strerror(errno));
}

// Assign bandwidth using percentages
void assign_bw(int core1_bw, int core2_bw, int core3_bw, int core4_bw)
{
	if (system(NULL)) puts ("Ok");
		else exit (EXIT_FAILURE);

	char script_str[STR_SIZE] = {0};
	snprintf(script_str, sizeof(script_str), "%s %d %d %d %d %s",
		"echo", core1_bw, core2_bw, core3_bw, core4_bw, "> /sys/kernel/debug/memguard/limit");

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
	snprintf(script_str, sizeof(script_str), "%s %d %d %d %d %s",
		"echo mb", core1_bw, core2_bw, core3_bw, core4_bw, "> /sys/kernel/debug/memguard/limit");

	int status = system(script_str);
	if (status < 0)
		printf("%s\n", strerror(errno));
}

// Set total bandwidth that can be partitionen between cores
// Att sätta maxbw verkar inte fungera, så skit i denna.
void set_max_bw(int max_bw)
{
	if (system(NULL)) puts ("Ok");
		else exit (EXIT_FAILURE);

	char script_str[STR_SIZE] = {0};
	snprintf(script_str, sizeof(script_str), "%s %d %s",
		"echo maxbw", max_bw, "> /sys/kernel/debug/memguard/control");

	int status = system(script_str);
	if (status < 0)
		printf("%s\n", strerror(errno));
}

// Calculate used bandwidth by using performance counters
inline static double calculate_bandwidth_MBs(long long l3_misses, long long prefetch_misses, int cache_line_size, double execution_time)
{
	long long bw_b = (l3_misses + prefetch_misses) * cache_line_size;
	return (double)(bw_b * 1.1920928955078125e-7) / execution_time;
}

// Parition bandwidth between different cores
void partition_bandwidth(ThreadInfo* th, int num_threads)
{
	double used_wma_bw[num_threads] = {0};
	int new_core_bw[num_threads]; // in percent
	
	double used_bw[num_threads];
	get_bw_from_memguard(used_bw);

	/* Calculate bandwidth used by each thread */
	for (int i = 0; i < num_threads; i++){
		std::cout << "Core: " << i << ", bw: " << used_bw[i] << '\n';

		//Add current used_bw to prev_used_bw and delete oldest
		th[i].prev_used_bw.push_back(used_bw[i]);

		if(th[i].prev_used_bw.size() >= BW_VAL_THRESHOLD)
			th[i].prev_used_bw.erase(th[i].prev_used_bw.begin());
		
		// //Calculate WMA (Weighted Moving Average BW) for each core
		int tot_weight = 0;
		for(int j = th[i].prev_used_bw.size() - 1; j >= 0; j--)
		{
			// kan testa med att ha en array med vikter sen
			used_wma_bw[i] += th[i].prev_used_bw[j] * (j + 1);
			tot_weight += (j + 1);
		}
		used_wma_bw[i] /= tot_weight;
		std::cout << "wma bw: " << used_wma_bw[i] << "\n";

		// int n = th[i].runs < BW_VAL_THRESHOLD ? h[i].runs : BW_VAL_THRESHOLD;
		// int m = 2 / (1 + n);
		// th[i].ewma_bw = m * used_bw + (1 - m) * th[i].ewma_bw;
	}
	std::cout << '\n';
	
	/* Calculate how to partition bandwidth between different cores */
	// (As a percentage of maxbw?)
	
	
	
	/* Partition bandwidth */
	//assign_bw(new_core_bw[0], new_core_bw[1], new_core_bw[2], new_core_bw[3]);
}

#endif