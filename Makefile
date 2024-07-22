CC = g++
CCFLAGS =  -std=c++17 -O0 -lm #-Wall -Wextra

.PHONY: build clean

build: server subscriber

server: server.cpp
	$(CC) -o $@ $^ $(CCFLAGS)

subscriber: subscriber.cpp
	$(CC) -o $@ $^ $(CCFLAGS)

run:
	./server 12345
client:
	./subscriber 1 127.0.0.1 12345

clean:
	@rm -f server subscriber
	@lsof -i :12345 -t | xargs kill -9 >/dev/null 2>&1 || true

free_port: # cuz clean is not enough for some reason?
	@lsof -i :12345 -t | xargs kill -9