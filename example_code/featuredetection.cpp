#include "opencv2/opencv.hpp"
#include <stdio.h>
#include <opencv2/legacy/legacy.hpp>
#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/nonfree/nonfree.hpp>
#include <opencv2/nonfree/features2d.hpp>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <chrono>
#include <iomanip>
#include <pthread.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include "System.hpp"
 
using namespace std;
using namespace cv;

int amountOfCores = 4;
char* source_window = "Source image";
char* corners_window = "Corners detected";
int imgheight;
int imgwidth;
vector<KeyPoint> * keypoints;
cv::Mat * roi;
vector<double> * executionTimes;
vector<double> medianExecTimes;
char * execAlgs [12];
int testcases = 100;
Mat src, src_gray, out;
int programrunning = 0;
int algrunning = 0;
vector<int> threadIdVec;
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
int frame = 0;
int gatherPerformance = 1;

struct threadInfo {
    char * algorithm;
    int currentCore;
};


int stick_this_thread_to_core(int core_id) {
   int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
   if (core_id < 0 || core_id >= num_cores)
      return EINVAL;

   cpu_set_t cpuset;
   CPU_ZERO(&cpuset);
   CPU_SET(core_id, &cpuset);

   pthread_t current_thread = pthread_self();    
   return pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
}

void usePerf(int pid1, int pid2, int pid3, int pid4) {
	std::string filename = name.find(".data") == std::string::npos ? (name + ".data") : name;

	// Launch profiler
	pid_t pid;
	std::stringstream s;
	s << getpid();
	pid = fork();

	if (pid == 0) {
	auto fd=open("/dev/null",O_RDWR);
	dup2(fd,1);
	dup2(fd,2);
	exit(execl("/usr/bin/perf","perf","stat","-o",filename.c_str(),"-p",s.str().c_str(),"-e",
			"longest_lat_cache.miss,l1d_pend_miss.pending,l2_rqsts.pf_miss",nullptr));
	}

	// Run body
	while (gatherPerformance){}

	// Kill profiler  
	kill(pid,SIGINT);
	waitpid(pid,nullptr,0);
}

void * measureInstructions(void * args) {
	while (programrunning==1)
	{
		while (algrunning==1 && threadIdVec.size() == amountOfCores)
		{
			//threadIdvec

			usleep(100000);
		}
	}
}

/*void test_fork_exec(int threadId)
{
	pid_t pid;
	int status;
	char temp [14] = itoa(threadId);
	pid = fork();

	switch (pid) {
	case -1:
		perror("fork");
		break;

	case 0:
		execl("exitscript.sh", temp, (char*)0);
		perror("exec");
		std::cout << "ERROR IN CODE ERRORRRRR" ;
		break;

	default:
		printf("Child id: %i\n", pid);
		fflush(NULL);
		if (waitpid(pid, &status, 0) != -1) {
			printf("Child exited with status %i\n", status);
		} 
		else {
			std::cout << "ERROR IN WAIT ERRORRRRR" ;
			perror("waitpid");
		}
		break;
	}
	
	//char * localThreaID = itoa(threadId);
	//std::cout << "HELLO" << std::endl;
}*/


void * featureThread(void * threadArg) {
	pid_t tid;
	tid = syscall(SYS_gettid);
	double timeD = 0.0;
	struct threadInfo * coreInfo = (struct threadInfo*)threadArg;
	std::chrono::time_point<std::chrono::system_clock> coreStart, coreEnd;
	//system("perf stat -e inst_retired.any -p 4537"); 
	
	const int threshold=9;
	int core = (*coreInfo).currentCore;
	char * alg = (*coreInfo).algorithm;
	char perfBuffer [256];
	int ec = 0;
	
	//ec = sprintf (perfBuffer, "sudo perf stat -t %d -e longest_lat_cache.miss", tid);
	ec = sprintf (perfBuffer, "sudo perf stat -p 9728 -e longest_lat_cache.miss");
	//exit(execl("/usr/bin/perf", "stat", "-p", "stat"," -h","datat.data", "-e", "longest_lat_cache.miss"));
	//std::cout << "Executing " << perfBuffer << " and my tid is: " << tid << std::endl;
	//exit(system(perfBuffer));
	//test_fork_exec(tid);
	stick_this_thread_to_core(core);
	pthread_mutex_lock( &mutex1 );
	//threadIdVec.push_back((int)tid);
	pthread_mutex_unlock( &mutex1 );
	algrunning = 1;
	//std::cout << "My tid is: " << tid << std::endl;
	//std::cout << "My name is: " << core << " and i work with: " << alg << std::endl;
	Ptr<FeatureDetector> detector = FeatureDetector::create(alg);
	
	
	for (int i = 0; i < testcases;i++)
	{
		coreStart = std::chrono::system_clock::now();
		//cv::Rect roi_rect = cv::Rect (0,imgheight/amountOfCores*core,imgwidth,imgheight/amountOfCores);
	
		cv::Rect roi_rect = cv::Rect (0,0, imgwidth, imgheight);
  	roi[core] = src_gray(roi_rect);
		System::profile("hejhej", [&]()
		{
			detector->detect(roi[core],keypoints[core]);
		});
	
  	coreEnd = std::chrono::system_clock::now();
  	timeD = std::chrono::duration_cast<std::chrono::microseconds>(coreEnd - coreStart).count();
		executionTimes[core].push_back(timeD);

  	//std::cout  << "Core " << core << " execution time: " << timeD << std::endl;
	}
	algrunning = 0;
	pthread_exit(NULL);
}

void initAlgs() {
	execAlgs [0] = "FAST";
	execAlgs [1] = "HARRIS";
	execAlgs [2] = "SIFT";
	execAlgs [3] = "SURF";
	execAlgs [4] = "ORB";
	execAlgs [5] = "BRISK";
	execAlgs [6] = "MSER";
	execAlgs [7] = "GFTT";
	execAlgs [8] = "Dense";
	execAlgs [9] = "STAR";
	execAlgs [10] = "SimpleBlob";
	execAlgs [11] = "end";
}

int main(int argc, char** argv) {
	//source image
  int thread_ec = 0;
  void * status;
  
  /// Load source image and convert it to gray
  initModule_nonfree();
  //src = imread( argv[1], 1 );
  char * alg = argv[2];
  testcases = atoi(argv[3]);
  initAlgs();
  int coreLimit = 4;
  int countAlgs = atoi(argv[3]);
  char ** selectedAlgs;
  amountOfCores = atoi(argv[4]);
  selectedAlgs = malloc(sizeof(char*)*amountOfCores);

  VideoCapture cap;
  cap.set(CV_CAP_PROP_FOURCC, CV_FOURCC('H', '2', '6', '4'));

  if (strcmp(argv[1], "h")==0 || argc < 5)
  {
		std::cout << "Input picture, lol, testcases, amount of cores, alg, alg, alg "<<std::endl;
		for (int i = 0; i < 11; i++)
		{
			std::cout << i << " - " << execAlgs[i] << std::endl;
		}
		return 0;
  }

  for (int i = 5; i<argc; i++)
  {
		std::cout<<"algorithm at"<< 5-i <<" " << argv[i] << std::endl;
		selectedAlgs[i-5] = argv[i];
  }

  if (!cap.open(argv[1]))
  {
		cout << "didnt open" <<endl;
  }
  
  while (1)
  {
		cap >> src;
		imshow( "Source", src);

		if (src.empty())
			break;

		/*for (int algs = 0; strcmp(execAlgs[algs], "end") != 0; algs++)
		{
		std::cout << execAlgs[algs] << '\t';
		}*/
		//std::cout << std::endl;
		
		/*for (int coreCountIterator = 1; coreCountIterator <= coreLimit; coreCountIterator++)
		{
		amountOfCores = coreCountIterator;
		for (int countAlgs = 0; strcmp(execAlgs[countAlgs], "end") != 0; countAlgs++)
		{*/

  	keypoints = new vector<KeyPoint>[amountOfCores];
  	roi = new cv::Mat[amountOfCores];
  	executionTimes = new vector<double>[amountOfCores];
  	//std::cout << "Algorithm: " << alg << std::endl;
  	//std::cout << "Amount of cores: " << cores << std::endl; 
  	cvtColor( src, src_gray, CV_BGR2GRAY );
  	/// Create a window and a trackbar
  	//namedWindow( source_window, CV_WINDOW_AUTOSIZE );
  	//imshow( source_window, src );

  	imgwidth = src_gray.size().width;
  	imgheight = src_gray.size().height;
  
  	pthread_t threads[24];
  	pthread_attr_t attr;

  	vector<Mat> resMat;
  	struct threadInfo * info;
  	for (int c=0; c<amountOfCores;c++)
  	{
			info = malloc(sizeof(struct threadInfo));
			//(*info).algorithm = execAlgs[countAlgs];
			//std::cout<<"sadasa alg:"<<selectedAlgs[c]<<std::endl;
			(*info).algorithm = selectedAlgs[c];
			//std::cout << "Input alg " << c << " " << selectedAlgs[1] << std::endl;
			(*info).currentCore = c;

			if (strcmp (alg, "lol") == 0)
			{
				//std::cout << "starting a lol thread" << std::endl;
				thread_ec = pthread_create(&threads[c], NULL, featureThread, (void*)info);
			} 
  	}

  	for(int c=0; c<amountOfCores;c++) 
  	{
        	thread_ec = pthread_join(threads[c], &status);
  	}

    //begin = clock();

  	for (int i = 0; i < amountOfCores; i++)
  	{
			std::sort(executionTimes[i].begin(), executionTimes[i].end());
  	}

  	int length = executionTimes[0].size();
  	//std::cout << execAlgs[countAlgs] << '\t';

  	for (int i = 0; i < amountOfCores; i++)
  	{
			if (executionTimes[i].size() % 2 == 1)
			{
				medianExecTimes.push_back(executionTimes[i][(int)length/2]);
			}
			else
			{
				medianExecTimes.push_back((executionTimes[i][(int)(length/2)]+executionTimes[i][(int)(length/2+1)])/2);
			}

			//std::cout << medianExecTimes[i] << '\t';
			std::sort(medianExecTimes.begin(), medianExecTimes.end());
  	}

		double frameMaximumExec = medianExecTimes[medianExecTimes.size()-1];
		//std::cout << std::fixed << std::setprecision(0) << medianExecTimes[medianExecTimes.size()-1] << '\t';
		double FPS = 1000000/frameMaximumExec;
		//std::cout << "FPS is: " << FPS << std::endl;

		for (int i = 0; i < amountOfCores; i++)
		{
			executionTimes[i].clear();
			medianExecTimes.clear();
		}

		//std::cout << std::endl;
		//std::cout << keypoints.size() <<std::endl;

		for (int i = 0; i < amountOfCores; i++)
		{
				resMat.push_back(roi[i].clone());
				drawKeypoints(roi[i], keypoints[i], resMat[i], Scalar(0,0,255), DrawMatchesFlags::DEFAULT);
				//std::cout << "HEJ"<< std::endl;
		}
		//std::cout << "HEJ"<< std::endl;
		
		vconcat(resMat,out);
		/*}
		std::cout << std::endl;
		}*/

		char buffer[256];
		snprintf(buffer, sizeof(buffer), "(%g)", FPS);
		putText(out, buffer, cvPoint(30,30), 
		FONT_HERSHEY_COMPLEX_SMALL, 0.8, cvScalar(0,0,255), 1, CV_AA);

		//namedWindow( corners_window, CV_WINDOW_AUTOSIZE );
		imshow( "Video", out );

		waitKey(1);
  }
  return(0);
}