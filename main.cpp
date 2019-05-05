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
#include "perfCounters.hpp"
#include "bandwidth.hpp"

int num_threads;
int global_width, global_height;
cv::Mat frame;
cv::Mat* roi;

const cv::Scalar COLORS[4] = {
	cv::Scalar(255, 0, 0),
	cv::Scalar(0, 255, 0),
	cv::Scalar(0, 0, 255),
	cv::Scalar(255, 0, 255)
};

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

	// 
	long long values[2];
	start_PAPI();

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

	//cv::Ptr<cv::ORB> detector = cv::ORB::create(800);
	cv::Ptr<cv::xfeatures2d::SURF> detector = cv::xfeatures2d::SURF::create(2);
	//cv::Ptr<cv::xfeatures2d::SIFT> detector = cv::xfeatures2d::SIFT::create(1200);
	detector->detect(roi[thread_info->core_id], keypoints, cv::Mat());
	cv::drawKeypoints(roi[thread_info->core_id], keypoints, roi[thread_info->core_id], COLORS[thread_info->core_id], cv::DrawMatchesFlags::DEFAULT);

	// End timer
	//thread_info->execution_time = (double)(time_ns() - start_time) * 0.000001;
	thread_info->execution_time = (double)(cv::getTickCount() - start_cycle) / cv::getTickFrequency();

	// Read performance counters
	read_PAPI(values);
	stop_PAPI(values);
	thread_info->l3_misses = values[0];
	thread_info->prefetch_misses = values[1];
	std::cout << "getppid/getpid/pthread_self: " << getppid()  << "/" << getpid() << "/" << pthread_self() << " \n";
}

int main(int argc, char** argv)
{
	//getchar();

	// Check if user is root
	if (getuid()) {
		std::cout << "User is not root" << std::endl;
		return 0;
	}
	
	//set_exclusive_mode(0); // Disable best-effort
	std::cout << std::fixed << std::setprecision(3) << std::left;
	num_threads = atoi(argv[1]);
	void* status;

	init_PAPI();
	cv::VideoCapture cap(argv[2]);
  	cap.set(CV_CAP_PROP_FOURCC, CV_FOURCC('H', '2', '6', '4'));
	//cv::namedWindow("Video", cv::WINDOW_NORMAL);
	//cv::resizeWindow("Video", 1920, 1080);

	// Check if camera opened successfully
	if(!cap.isOpened()) {
		std::cout << "Error opening video stream or file" << std::endl;
		return 0;
	}
	
	// 
	roi = new cv::Mat[num_threads];
	ThreadInfo* thread_info = new ThreadInfo[num_threads];

	while(cv::waitKey(25) != 27) {
		int64 start = cv::getTickCount();

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
		
		for (int i = 0; i < num_threads; i++) {
			thread_info[i].core_id = i;

			if(pthread_create(&threads[i], NULL, feature_thread, (void*)&thread_info[i]) != 0) {
				perror("thread create error");
        		exit(1);
			}
		}
		
		// Join threads
		for (int i = 0; i < num_threads; i++) {
			if(pthread_join(threads[i], &status) != 0) {
				perror("thread join error");
            	exit(1);
			}
		}

		std::vector<cv::Mat> result_mat;
		for (int i = 0; i < num_threads; i++)
			result_mat.push_back(roi[i].clone());
		
		cv::Mat out;
		cv::vconcat(result_mat, out);
		//cv::imshow("Video", out);

		// Print stats
		double fps = cv::getTickFrequency() / (cv::getTickCount() - start);
		print_thread_info(thread_info, num_threads);
		std::cout << "FPS: " << fps << "\n\n";

		partition_bandwidth(thread_info, num_threads);
	}

	cap.release();
  	cv::destroyAllWindows();
	
	return 0;
}
