CXXFLAGS =
LDFLAGS = -lcurl

example: example.o analytics.o
	$(CXX) -o $@ $^ $(LDFLAGS)

analytics.o: analytics.cpp

clean:
	rm -rf example example.o

.PHONY: clean
