CC     = gcc
CFLAGS = -Wall
TARGET = mycomputer
OBJS   = main.o compiler.o processor.o memory.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

main.o: main.c compiler.h processor.h memory.h
	$(CC) $(CFLAGS) -c main.c

compiler.o: compiler.c compiler.h
	$(CC) $(CFLAGS) -c compiler.c

processor.o: processor.c processor.h memory.h
	$(CC) $(CFLAGS) -c processor.c

memory.o: memory.c memory.h
	$(CC) $(CFLAGS) -c memory.c

clean:
	rm -f $(OBJS) $(TARGET) program.byte
