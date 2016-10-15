FLAGS=-w -std=c++0x
LIBS=-lpthread
DIRS=-Iinclude
target=build

all:
	g++ main.cpp src/*.cpp $(DIRS) $(FLAGS) $(LIBS) -o bin/$(target)

server:
	g++ -DMASTER main.cpp src/*.cpp $(DIRS) $(FLAGS) $(LIBS) -o bin/server

client:
	g++ main.cpp src/*.cpp $(DIRS) $(FLAGS) $(LIBS) -o bin/client

run:
	./bin/$(target)
