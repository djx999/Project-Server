#Makefile管理工具
CC=gcc
CFLAGS=-g -Wall -I$(INCLUDE) -I/usr/include/mysql
CPPFLAGS=-E -D
TARGET=app
INCLUDE=../include
LDFLAGS=-L -lpthread -lmysqlclient

FILENAME = $(wildcard *.c)
 DESFILE = $(patsubst %.c,%.o,$(FILENAME))
 
  $(TARGET):$(DESFILE)
      $(CC) $(DESFILE) $(CFLAGS) $(LDFLAGS) -o  $@
  
 $(DESFILE):$(FILENAME)
      $(CC) -c $<
