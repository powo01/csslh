TARGET=csslh
LIBS=-lpthread
CFLAGS=-Wall -Wextra -Werror -O2 -g
OBJS=main.o config.o utils.o handleConnections.o readWrite.o  
CC=gcc
INSTALLDIR ?= /usr/local/bin
DEPEND=.depend

.PHONY: clean

all:  $(DEPEND) $(TARGET)

install: $(TARGET)
	install -s $< $(INSTALLDIR)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LIBS)
		
%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@
    
clean:
	rm -f $(TARGET) *.o $(DEPEND)

$(DEPEND): $(OBJS:%.o=%.c)
	$(CC) -M $(CFLAGS) $^ >$@

-include $(DEPEND)
