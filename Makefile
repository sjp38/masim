.PHONY: clean help

APPS	:= masim
MASIM	:= masim

CC	:= gcc
CFLAGS	:= -g -I$(IDIR) -O3 -Wall -Werror -std=gnu99

OBJ_MSM	:= masim.o misc.o

all: $(APPS)

%.o: %.c masim.h
	$(CC) -c -o $@ $< $(CFLAGS)

%.s: %.c
	$(CC) -S -o $@ $< $(CFLAGS)

$(MASIM): $(OBJ_MSM)
	$(CC) -o $@ $^ $(LIBS)

clean:
	rm -f *.o *.s $(APPS)

help:
	@echo "Usage: make <target>"
	@echo ""
	@echo "targets: $(APPS)"
	@echo ""
