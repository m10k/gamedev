OBJECTS = main.o
OUTPUT = nimotsusan
CFLAGS += $(shell sdl2-config --cflags)
LIBS += $(shell sdl2-config --libs) -lSDL2_ttf

all: $(OUTPUT)

$(OUTPUT): $(OBJECTS)
	$(CC) -Wall -O2 -o $@ $^ $(LIBS)

clean:
	rm -rf $(OBJECTS) $(OUTPUT)

.PHONY: clean
