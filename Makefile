CC := gcc
CFLAGS := -fPIC
SHARED_FLAGS := -shared

all: memalloc.so

memalloc.so: memalloc.c
	$(CC) $(CFLAGS) $(SHARED_FLAGS) -o $@ $<

clean:
	rm -f memalloc.so