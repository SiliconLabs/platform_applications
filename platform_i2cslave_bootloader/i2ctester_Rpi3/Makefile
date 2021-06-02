SOURCEDIR = src
HEADERDIR = include
CFILES    = $(wildcard $(SOURCEDIR)/*.c)
BINARY    = i2ctester
CC      = gcc
CFLAGS  = -Wall
LDFLAGS =


all: $(BINARY)

$(BINARY): $(CFILES) 
	$(CC) $(CFLAGS) $(LDFLAGS) -I$(HEADERDIR) -I$(SOURCEDIR) $(CFILES) -o $(BINARY)

.PHONY: all clean
clean: 
	-rm -f $(BINARY)
