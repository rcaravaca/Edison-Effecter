OBJS = Effecter.o 
CC = g++
DEBUG = -g
CFLAGS = -Wall -c $(DEBUG) -pedantic
LFLAGS = -Wall $(DEBUG) -pedantic -lasound -lmraa -pthread
TARGET = Effecter

$(TARGET): $(OBJS)
	$(CC) $(LFLAGS) -o $(TARGET) $(OBJS)
Effecter.o: Effecter.cpp
	$(CC) $(CFLAGS) Effecter.cpp
clean:
	rm -f *.o $(TARGET)
exec: 
	./Effecter "hw:2,0"

