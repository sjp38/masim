.PHONY: clean help

APPS	:= miw
MIW	:= miw

CC	:= gcc
CFLAGS	:= -g -I$(IDIR) -O3 -Wall -Werror -std=gnu99

OBJ_MIW	:= miw.o

all: $(APPS)

$(ODIR)/%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(MIW): $(OBJ_MIW)
	$(CC) -o $@ $^ $(LIBS)

clean:
	rm -f $(ODIR)/*.o $(APPS)

help:
	@echo "Usage: make <target>"
	@echo ""
	@echo "targets: $(APPS)"
	@echo ""
