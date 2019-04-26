#gcc partitionpool.c wrapper.h wrapper.c pearsoncorrelation.c pearsoncorrelation.h -lm -pthread -lrt -o pool /home/jakob/Desktop/papi/src/libpapi.a
#g++ cv_algs.cpp wrapper.h wrapper.c -pthread -o feature -lrt -O0 -fpermissive -std=c++11 `pkg-config --cflags --libs opencv` /home/tommy/Desktop/papi/src/libpapi.a -pthread
#gcc perf_arm_pmu.c wrapper.h wrapper.c -pthread -o matmult -pthread -lrt /home/jakob/Desktop/papi/src/libpapi.a
g++ main.cpp -pthread -o feature -lrt -O0 -fpermissive -std=c++11 `pkg-config --cflags --libs opencv` /usr/local/lib/libpapi.a -pthread