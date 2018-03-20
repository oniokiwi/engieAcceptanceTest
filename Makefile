#IDIR =../../../../libmodbus/src
IDIR=../libmodbus/src
LDIR=/usr/local/lib


TARGET=engieAccepTest
CC=gcc
#CFLAGS=-I$(IDIR) -L$(LDIR) -g -std=gnu99
CFLAGS= -g -I/usr/local/include -I/usr/include/json-c/ -L/usr/local/lib

.PHONY: default all clean check cron

default: $(TARGET)
all: default

SRC_C=engie.c \
      main.c


HDR=typedef.h \
    engie.h 
    

LIBS=-lpthread -lmodbus

#DEPS = $(patsubst %,$(IDIR)/%,$(HDR))
OBJ=$(patsubst %.c,%.o,$(SRC_C))

%.o: %.c $(HDR)
	$(CC) -c -o $@ $< $(CFLAGS)

$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)
	
check:
	@echo '#############################'
	@echo ' SRC_C  = $(SRC_C)           '
	@echo ' OBJ    = $(OBJ)             '
	@echo ' HDR    = $(HDR)             '
	@echo '#############################'
	

clean:
	rm -f *.o $(TARGET) 
