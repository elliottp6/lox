# parameters for "make" (must come first in file!)
# this parameter enables parallel compilation based on the # of processors
MAKEFLAGS+=j$(shell nproc)

# output folders
DEBUG_FOLDER = bin/debug
RELEASE_FOLDER = bin/release

# exectuable
EXE = main
DEBUG_EXE = $(DEBUG_FOLDER)/$(EXE)
RELEASE_EXE = $(RELEASE_FOLDER)/$(EXE)

# objects
SOURCES = $(shell find . -name "*.c" ! -wholename "*/trash/*") # all *.c files, except those in the "trash" folder
OBJECTS = $(SOURCES:%.c=%.o)
DEBUG_OBJECTS = $(addprefix $(DEBUG_FOLDER)/, $(OBJECTS))
RELEASE_OBJECTS = $(addprefix $(RELEASE_FOLDER)/, $(OBJECTS))

# compilation flags
LIBS =
DEBUG_FLAGS = -Wall -Wextra -Werror -DDEBUG -g
RELEASE_FLAGS = -Wall -Wextra -Werror -DNDEBUG -Ofast -flto

# top-level targets (note that these CANNOT be the same name as any of the C files)
debug_build: $(DEBUG_EXE)
release_build: $(RELEASE_EXE)

# link
$(DEBUG_EXE): $(DEBUG_OBJECTS)
	gcc -o $@ $^ $(DEBUG_FLAGS) $(LIBS)
$(RELEASE_EXE): $(RELEASE_OBJECTS)
	gcc -o $@ $^ $(RELEASE_FLAGS) $(LIBS)

# compile
$(DEBUG_FOLDER)/%.o: %.c
	mkdir -p $(dir $@) && gcc -o $@ $^ -c $(DEBUG_FLAGS)
$(RELEASE_FOLDER)/%.o: %.c
	mkdir -p $(dir $@) && gcc -o $@ $^ -c $(RELEASE_FLAGS)

# clean
clean:
	rm -f $(DEBUG_EXE) $(RELEASE_EXE) $(DEBUG_OBJECTS) $(RELEASE_OBJECTS)
