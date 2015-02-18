CC=gcc
CFLAGS=-c -Wall
LDFLAGS=
SOURCES=bit-pack.c ega-codebook.c lzrw.c native-test.c 
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=native-test

all: $(SOURCES) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf *.o
