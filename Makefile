CC = g++
FLAGS = -g -O0 -Wall
LIBS = -lsfml-system -lsfml-graphics -lsfml-window
SRC = $(shell find src -name *.cpp)
OBJ = $(patsubst %.cpp,%.o,$(SRC))
TARGET = bin/main.bin

all: clean $(OBJ)
	$(CC) $(FLAGS) $(OBJ) -o $(TARGET) $(LIBS)

%.o: %.cpp
	$(CC) -c $(FLAGS) $^ -o $@

clean:
	rm -rf $(OBJ) $(TARGET)

run_main: all
	$(TARGET)