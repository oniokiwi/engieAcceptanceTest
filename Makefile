#IDIR =../../../../libmodbus/src
IDIR=../libmodbus/src
LDIR=/usr/local/lib


TARGET=bluekolding
CC=gcc
#CFLAGS=-I$(IDIR) -L$(LDIR) -g -std=gnu99
CFLAGS= -g -I/usr/local/include -L/usr/local/lib

.PHONY: default all clean check cron

default: $(TARGET)
all: default

SRC_C=bluekolding.c \
      main.c


HDR=typedef.h \
    bluekolding.h 
    

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

cronjobstart:
	crontab -u ${USER} cronjob.txt

cronjobstop:
	crontab -u ${USER} -r	

clean:
	rm -f *.o $(TARGET) 
