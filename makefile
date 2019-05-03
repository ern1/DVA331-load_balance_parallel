num_threads = 4
src = "drone.mp4"

clean:
	@rm -f /tmp/test123/*

compile:
	@mkdir -m 777 -p /tmp/test123
	@g++ -std=c++11 main.cpp `pkg-config --cflags --libs opencv` -o /tmp/test123/main -lrt -O0 -fpermissive /usr/local/lib/libpapi.a -pthread

run:
	@/tmp/test123/main $(num_threads) $(src) || echo "exit($$?)"