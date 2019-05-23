OBJECTS = main.o
OUTPUT = nimotsukun

all: $(OUTPUT)

$(OUTPUT): $(OBJECTS)
	$(CC) -Wall -O2 -o $@ $^

clean:
	rm -rf $(OBJECTS) $(OUTPUT)

.PHONY: clean
