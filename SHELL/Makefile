CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
TARGET = myshell

.PHONY: all clean

all: $(TARGET)

$(TARGET): myshell.cpp
	$(CXX) $(CXXFLAGS) -o $(TARGET) myshell.cpp

clean:
	rm -f $(TARGET) $(TARGET).exe
