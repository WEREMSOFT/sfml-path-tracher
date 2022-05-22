CC = g++
FLAGS = -g -O0 -Wall
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

run_main: clean all
	$(TARGET)

set_release:
	$(eval FLAGS := -g -O4 -Wall)

run_release: clean set_release all
	$(TARGET)

copy_assets:
	cp -rf resources bin