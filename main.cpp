#include <opencv2/opencv.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/xfeatures2d/nonfree.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgcodecs/imgcodecs.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/objdetect/objdetect.hpp>
#include <stdio.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <iomanip>
#include <pthread.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <math.h>
#include <sys/time.h>
#include <chrono>
#include <papi.h>
#include <pthread.h>
//#include "perfCounters.hpp"
#include "bandwidth.hpp"

#define USE_MEMGUARD 1

int num_threads;
int global_width, global_height;
cv::Mat frame;
cv::Mat* roi;

int get_avg_bw_done = 0;
double avg_bw[4] = {0};

const cv::Scalar COLORS[4] = {
	cv::Scalar(255, 0, 0),
	cv::Scalar(0, 255, 0),
	cv::Scalar(0, 0, 255),
	cv::Scalar(255, 0, 255)
};

void get_avg_bw_usage()
{
	std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now();
	std::fill(avg_bw, avg_bw + num_threads, 0);
	double res[num_threads] = {0};
	//int cnt = 0;

	while(get_avg_bw_done < 4)
	{
		get_bw_from_memguard(res);
		for(int i = 0; i < num_threads; i++)
		{
			//avg_bw[i] += res[i];
			//cnt++;	
			
			if(res[i] > avg_bw[i])
				avg_bw[i] = res[i];

			// if(std::chrono::steady_clock::now() - start_time > std::chrono::milliseconds(10))
			// {
			// 	break;
			// }
		}
	}

	// for(int i = 0; i < num_threads; i++)
	// 		avg_bw[i] /= cnt;

	get_avg_bw_done = 0;
}

int stick_this_thread_to_core(int core_id)
{
  int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
  if (core_id < 0 || core_id >= num_cores)
  {
    printf("Core out of index");
    return 0;
  }

  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(core_id, &cpuset);

  pthread_t current_thread = pthread_self();    
  return pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
}

void* feature_thread(void* threadArg)
{
	ThreadInfo* thread_info = (ThreadInfo*)threadArg;
	stick_this_thread_to_core(thread_info->core_id);

	// Start timer
	//long long unsigned start_time = time_ns();
	long long start_cycle = cv::getTickCount();

	// ROI
	int local_x = 0, local_y;
	if (thread_info->core_id == 0)
		local_y = 0;
	else
		local_y = (int)global_height / num_threads * thread_info->core_id - 1;
	int local_width = global_width;
	int local_height = (int)global_height / num_threads;
	
	cv::Rect roi_rect = cv::Rect(local_x, local_y, local_width, local_height);
	roi[thread_info->core_id] = frame(roi_rect);
	std::vector<cv::KeyPoint> keypoints;

	//cv::Ptr<cv::ORB> detector = cv::ORB::create(10);
	cv::Ptr<cv::xfeatures2d::SURF> detector = cv::xfeatures2d::SURF::create(5);
	//cv::Ptr<cv::xfeatures2d::SIFT> detector = cv::xfeatures2d::SIFT::create(10);
	detector->detect(roi[thread_info->core_id], keypoints, cv::Mat());
	cv::drawKeypoints(roi[thread_info->core_id], keypoints, roi[thread_info->core_id], COLORS[thread_info->core_id], cv::DrawMatchesFlags::DEFAULT);
	get_avg_bw_done++;
	// End timer
	//thread_info->execution_time = (double)(time_ns() - start_time) * 0.000001;
	thread_info->execution_time = (double)(cv::getTickCount() - start_cycle) / cv::getTickFrequency();
	thread_info->tot_exec_time += thread_info->execution_time;
	
	
#if USE_MEMGUARD
	/* 	TODO: 
	**	- Change calculate_bandwidth_MBs to not make use of prefetch cache misses.
	**	- Only use this if we use perf events to get number of cache misses. 
	*/
	// Read performance counters and calculate used bandwidth
	//thread_info->used_bw = calculate_bandwidth_MBs(?, ?, thread_info->execution_time) / max_bw;
#endif
}

int main(int argc, char** argv)
{
	// Check if user is root
	if (getuid()) {
		std::cout << "User is not root" << std::endl;
		return 0;
	}

	std::cout << std::fixed << std::setprecision(3) << std::left;
	num_threads = atoi(argv[1]);
	void* status;

#if USE_MEMGUARD
	measure_max_bw();		// Measure and set max_bw
	set_exclusive_mode(0); 	// Disable best-effort
#endif

	cv::VideoCapture cap(argv[2]);
  	cap.set(CV_CAP_PROP_FOURCC, CV_FOURCC('H', '2', '6', '4'));

	// Check if camera opened successfully
	if(!cap.isOpened()) {
		std::cout << "Error opening video stream or file" << std::endl;
		return 0;
	}
	
	// 
	roi = new cv::Mat[num_threads];
	ThreadInfo* thread_info = new ThreadInfo[num_threads];
	double total_tick_count = 0;
	int num_frames = 0;

	while((cv::waitKey(25) != 27) && (num_frames < 100)) {
		cap >> frame;
		if(frame.empty())
			break;
		
		global_width = frame.size().width;
		global_height = frame.size().height;
		int size = frame.elemSize();
		int cols = frame.cols;
		int rows = frame.rows;
		int total_size = size * cols * rows;
		
		// Create threads
		pthread_t threads[num_threads];
  		pthread_attr_t attr;

		int64 start = cv::getTickCount();
		
		for (int i = 0; i < num_threads; i++) {
			thread_info[i].core_id = i;

			if(pthread_create(&threads[i], NULL, feature_thread, (void*)&thread_info[i]) != 0) {
				perror("thread create error");
        		exit(1);
			}
		}

		//get_avg_bw_usage();

		// Join threads
		for (int i = 0; i < num_threads; i++) {
			if(pthread_join(threads[i], &status) != 0) {
				perror("thread join error");
            	exit(1);
			}
		}

#if USE_MEMGUARD
		double used_bw[4];
		get_bw_from_memguard(used_bw);
		for(int i = 0; i < num_threads; i++)
			thread_info[i].used_bw = used_bw[i] / max_bw;
#endif

		// Print stats
		double ticks = cv::getTickCount() - start;
		double fps = cv::getTickFrequency() / ticks;
		print_thread_info(thread_info, num_threads);
		std::cout << "FPS: " << fps << "\n\n";
		for(int i = 0; i < num_threads; i++)
			std::cout << "AVG_BW: " << avg_bw[i] << "\n";

		total_tick_count += ticks;
		num_frames++;

		// create image
		std::vector<cv::Mat> result_mat;
		for (int i = 0; i < num_threads; i++)
			result_mat.push_back(roi[i].clone());
		
		cv::Mat out;
		cv::vconcat(result_mat, out);
		//cv::imshow("Video", out);


#if USE_MEMGUARD
		partition_bandwidth(thread_info, num_threads);
#endif
	}

	cap.release();
  	cv::destroyAllWindows();

	// double sum_exec_time = 0;
	// for(int i = 0; i < num_threads; i++)
	// 	sum_exec_time += thread_info[i].tot_exec_time;

	// std::cout << "Sum of execution times: " << sum_exec_time << std::endl;
	std::cout << "Execution time: " << (total_tick_count / cv::getTickFrequency()) << std::endl;
	return 0;
}
