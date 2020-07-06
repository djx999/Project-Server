CC=gcc
CFLAGS=-g -Wall -c -I$(INCLUDE) -I/usr/include/mysql
CPPFLAGS=-E -D
TARGET=app
INCLUDE=../include
LDFLAGS=-L -lpthread -lmysqlclient
  
FILENAME = $(wildcard *.c)
DESFILE = $(patsubst %.c,%.o,$(FILENAME))

$(TARGET):$(DESFILE)
  $(CC) $(DESFILE) $(LDFLAGS) $(CFLAGS) -o  $@ 
$(DESFILE):$(FILENAME)
  $(CC) -c $<
