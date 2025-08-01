CXX = g++
CXXFLAGS = -std=c++17 -O3 -Wall -Wextra
TARGET = reconstruction_orderbook
SRCS = main.cpp orderbook.cpp
OBJS = $(SRCS:.cpp=.o)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
