CXX ?= g++
CXXFLAGS ?= -Wall -Wextra -O2 -std=c++11
TARGET = temp_monitor
SRC = temp_monitor.cpp

all: $(TARGET)

$(TARGET): $(SRC) ../driver/tempsensor_ioctl.h
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

.PHONY: all clean
