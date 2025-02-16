CXX := g++
CXXFLAGS := -I/home/declan/RPI/zwo/dependencies/zwo-asi-sdk/1.36/linux_sdk/include \
            -Wall -Wextra -std=c++14
LDFLAGS := -L/home/declan/RPI/zwo/dependencies/zwo-asi-sdk/1.36/linux_sdk/lib \
           -lASICamera2

TARGET := capture-exposure
SRCS := capture-exposure.cpp
OBJS := $(SRCS:.cpp=.o)

.PHONY: all clean clean-img

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

clean-img:
	rm -f *.bin *.fits *.png