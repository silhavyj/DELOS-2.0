NASM        = nasm
NASM_PARAMS = -f elf32 -O0
CXX         = g++
LD_PARAMS   = -melf_i386
CXX_PARAMS  = -m32                    \
              -nostdlib               \
              -nostdinc               \
              -fno-builtin            \
              -fno-stack-protector    \
              -fno-rtti               \
              -fno-exceptions         \
              -Wall                   \
              -Wextra                 \
              -I../stdlib/include

SRC_FOLDER_STDLIB = ../stdlib

CXX_SRC_STDLIB := $(shell find $(SRC_FOLDER_STDLIB) -name '*.cpp')
ASM_SRC_STDLIB := $(shell find $(SRC_FOLDER_STDLIB) -name '*.asm')

CXX_SRC_FILES  := $(shell ls *.cpp)
ASM_SRC_FILES  := $(shell ls *.asm)

OBJ_FILES      = $(CXX_SRC_FILES:.cpp=.o)      \
                 $(CXX_SRC_STDLIB:.cpp=.o)     \
                 $(ASM_SRC_STDLIB:.asm=.o)      \
                 $(ASM_SRC_FILES:.asm=.o)

BIN_FILES      = $(CXX_SRC_FILES:.cpp=.bin)    \
                 $(ASM_SRC_FILES:.asm=.bin)

HEADER_FILES   = $(CXX_SRC_FILES:.cpp=.bin.h)  \
                 $(ASM_SRC_FILES:.asm=.bin.h)

all:
	./compile.sh

$(HEADER_FILES): $(BIN_FILES)
	xxd -i $< $@

$(BIN_FILES): link.ld $(OBJ_FILES)
	ld $(LD_PARAMS) -T $< -o $@ $(OBJ_FILES)

%.o: %.asm
	$(NASM) $(NASM_PARAMS) $^

%.o: %.cpp
	$(CXX) $(CXX_PARAMS) -c -o $@ $<

.PHONY clean:
	rm $(OBJ_FILES)    || true
	rm $(BIN_FILES)    || true
	rm $(HEADER_FILES) || true
	rm programs/*.h    || true
	./cleanup.sh
