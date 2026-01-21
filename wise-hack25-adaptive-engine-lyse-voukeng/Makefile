CC = g++
CFLAGS = -O2 -std=c++17
LDFLAGS = -ldl

all:
	$(CC) $(CFLAGS) src/monitor.cpp src/patcher.cpp src/main.cpp -o adaptive_engine $(LDFLAGS)

clean:
	rm -f adaptive_engine
