CXXFLAGS = -std=c++11
LDFLAGS = -lcurl

example: example.o analytics.o
	$(CXX) -o $@ $^ $(LDFLAGS)

analytics.o: analytics.cpp

clean:
	rm -rf example example.o analytics.o

valgrind: example
	valgrind --leak-check=full --show-reachable=yes ./$<

.PHONY: clean valgrind
