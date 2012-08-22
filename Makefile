TARGET=csslh
LIBS=-lpthread
CFLAGS=-Wall -O2 -g
OBJS=main.o settings.o utils.o handleConnections.o readWrite.o  
CC=gcc

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $+ $(LIBS)
		
%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@
    
clean:
	rm -f $(TARGET) $(OBJS)


