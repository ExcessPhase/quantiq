all: reverse.exe
CXXFLAGS=-std=c++17 -O3 -DNDEBUG -flto=auto -MMD -MP

OBJECTS=reverse.o io_data.o io_uring_queue_init.o

DEPS = $(OBJECTS:.o=.d)

clean:
	rm -f reverse.exe $(OBJECTS) $(DEPS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

reverse.exe:$(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJECTS) -luring

-include $(DEPS)
