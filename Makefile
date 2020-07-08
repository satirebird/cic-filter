CC=gcc 
CFLAGS=-c -g -Wall
LDFLAGS=
SOURCES=cic_filter.c test/cic_test.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=cic_test
INCLUDES=-I . -I test
LIBS=-lcunit

all: $(SOURCES) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) $(LIBS) -o $@

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@

clean:
	rm -rf *.o test/*.o $(EXECUTABLE)

