# Declaration of variables
CC = g++
SDL2_DIR = ./SDL2-2.32.2/x86_64-w64-mingw32
SDL2_IMAGE_DIR = ./SDL2_image-2.8.8/x86_64-w64-mingw32
SDL2_TTF_DIR = ./SDL2_ttf-2.24.0/x86_64-w64-mingw32
OUT_DIR = _out

# Check if required directories exist
ifeq ($(wildcard $(SDL2_DIR)),)
$(error SDL2 directory $(SDL2_DIR) not found)
endif
ifeq ($(wildcard $(SDL2_IMAGE_DIR)),)
$(error SDL2_image directory $(SDL2_IMAGE_DIR) not found) 
endif
ifeq ($(wildcard $(SDL2_TTF_DIR)),)
$(error SDL2_ttf directory $(SDL2_TTF_DIR) not found)
endif

CC_FLAGS = -w -g -O0 -I$(SDL2_DIR)/include/SDL2 -I$(SDL2_IMAGE_DIR)/include/SDL2 -I$(SDL2_TTF_DIR)/include/SDL2

# File names
EXEC = $(OUT_DIR)/blockenspiel.exe
SOURCES = $(wildcard *.cpp)
OBJECTS = $(SOURCES:.cpp=.o)

# Main target
$(EXEC): $(OBJECTS)
	mkdir -p $(OUT_DIR)
	$(CC) $(OBJECTS) -o $(EXEC) -static-libgcc -static-libstdc++ -L$(SDL2_DIR)/lib -L$(SDL2_IMAGE_DIR)/lib -L$(SDL2_TTF_DIR)/lib -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf -mconsole
	cp $(SDL2_DIR)/bin/SDL2.dll $(OUT_DIR)/
	cp $(SDL2_IMAGE_DIR)/bin/SDL2_image.dll $(OUT_DIR)/
	cp $(SDL2_TTF_DIR)/bin/SDL2_ttf.dll $(OUT_DIR)/
	cp libwinpthread-1.dll $(OUT_DIR)/
	cp map.txt font.ttf texmap.gif $(OUT_DIR)/

# To obtain object files
%.o: %.cpp
	$(CC) -c $(CC_FLAGS) $< -o $@

# To remove generated files
clean:
	rm -rf $(OUT_DIR)
	rm -f $(OBJECTS)
