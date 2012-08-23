TARGET=csslh
LIBS=-lpthread
CFLAGS=-Wall -O2 -g
OBJS=main.o settings.o utils.o handleConnections.o readWrite.o  
CC=gcc
INSTALLDIR ?= /usr/local/bin
DEPEND=.depend

all:  $(DEPEND) $(TARGET)

install: $(TARGET)
	install -s $< $(INSTALLDIR)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LIBS)
		
%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@
    
clean:
	rm -f $(TARGET) $(OBJS) $(DEPEND)

$(DEPEND):
	$(CC) -M $(CFLAGS) *.c > $@


-include $(DEPEND)
