.PHONY: clean help

APPS	:= masim
MASIM	:= masim

CC	:= gcc
CFLAGS	:= -g -I$(IDIR) -O3 -Wall -Werror -std=gnu99

OBJ_MSM	:= masim.o

all: $(APPS)

%.o: %.c config.h masim.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(MASIM): $(OBJ_MSM)
	$(CC) -o $@ $^ $(LIBS)

clean:
	rm -f *.o $(APPS)

help:
	@echo "Usage: make <target>"
	@echo ""
	@echo "targets: $(APPS)"
	@echo ""
