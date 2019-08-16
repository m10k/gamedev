OBJECTS = main.o engine.o gfx.o game.o anim.o ai.o
OUTPUT = bakudan
CFLAGS += $(shell sdl2-config --cflags)
LIBS += $(shell sdl2-config --libs) -lSDL2_ttf -lSDL2_image

all: $(OUTPUT)

$(OUTPUT): $(OBJECTS)
	$(CC) -Wall -O2 -o $@ $^ $(LIBS)

clean:
	rm -rf $(OBJECTS) $(OUTPUT)

.PHONY: clean
