CC = g++
FLAGS = -g -O4 -Wall
LIBS = -lsfml-system -lsfml-graphics -lsfml-window -lpthread
SRC = $(shell find src -name *.cpp)
OBJ = $(patsubst %.cpp,%.o,$(SRC))
TARGET = bin/main.bin

all: clean copy_assets $(OBJ)
	$(CC) $(FLAGS) $(OBJ) -o $(TARGET) $(LIBS)

%.o: %.cpp
	$(CC) -c $(FLAGS) $^ -o $@

clean:
	rm -rf $(OBJ) $(TARGET)

run_main: all
	$(TARGET)

copy_assets:
	cp -rf resources bin