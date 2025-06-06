MAIN_SRC = main
ASSEMBLY_SRC = lib
TARGET_EXEC = main

CC = gcc
AS = as

CFLAGS = -Wall -Wextra -O2 -I.

LDFLAGS = -lm

MAIN_OBJ = $(MAIN_SRC).o
ASSEMBLY_OBJ = $(ASSEMBLY_SRC).o
OBJS = $(ASSEMBLY_OBJ) $(MAIN_OBJ)

all: $(TARGET_EXEC)

# Rule to link the executable from object files
$(TARGET_EXEC): $(OBJS)
	$(CC) -o $(TARGET_EXEC) $(OBJS) $(LDFLAGS)
	@echo "Linking complete. Executable '$(TARGET_EXEC)' created."

$(MAIN_OBJ): $(MAIN_SRC).c hps_0.h stb_image/stb_image.h stb_image/stb_image_write.h
	$(CC) $(CFLAGS) -c -o $(MAIN_OBJ) $(MAIN_SRC).c
	@echo "Compiled $(MAIN_SRC).c -> $(MAIN_OBJ)"

# Rule to assemble the Assembly source file into an object file
$(ASSEMBLY_OBJ): $(ASSEMBLY_SRC).s
	$(AS) -o $(ASSEMBLY_OBJ) $(ASSEMBLY_SRC).s
	@echo "Assembled $(ASSEMBLY_SRC).s -> $(ASSEMBLY_OBJ)"

# Target to run the compiled executable
run: $(TARGET_EXEC)
	./$(TARGET_EXEC)

# Target to clean up generated files (object files and executable)
clean:
	rm -f $(OBJS) $(TARGET_EXEC)
	@echo "Cleaned up object files and executable."

# Target to build for debugging and run gdb
# Temporarily adds the -g flag (debugging symbols) to CFLAGS for this build.
debug: CFLAGS += -g
debug: $(TARGET_EXEC)
	gdb $(TARGET_EXEC)

# Declare targets that are not actual files
.PHONY: all run clean debug