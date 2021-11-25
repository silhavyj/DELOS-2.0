NASM        = nasm
NASM_PARAMS = -f elf32 -O0
CXX         = g++
LD_PARAMS   = -melf_i386
CXX_PARAMS  = -m32                    \
              -nostdlib               \
              -fno-use-cxa-atexit     \
              -nostdinc               \
              -fno-builtin            \
              -fno-stack-protector    \
              -fno-rtti               \
              -fno-exceptions         \
              -fno-leading-underscore \
              -Wno-write-strings      \
              -Wall                   \
              -Wextra                 \
              -Ikernel/include        \
              -Istdlib/include

KERNEL_BIN = DELOS-2.0.elf
KERNEL_ISO = DELOS-2.0.iso
VM_NAME    = DELOS-2.0

SRC_FOLDER_KERNEL = kernel
SRC_FOLDER_STDLIB = stdlib

CXX_SRC_FILES := $(shell find $(SRC_FOLDER_KERNEL) $(SRC_FOLDER_STDLIB) -name *.cpp)
ASM_SRC_FILES := $(shell find $(SRC_FOLDER_KERNEL) $(SRC_FOLDER_STDLIB) -name *.asm)

OBJ_FILES      = $(CXX_SRC_FILES:.cpp=.o) \
                 $(ASM_SRC_FILES:.asm=.o)

KERNEL_LINKER = $(SRC_FOLDER_KERNEL)/link.ld
USERSPACE     = compile-userspace

all: $(KERNEL_ISO)

%.o: %.asm
	$(NASM) $(NASM_PARAMS) $^

%.o: %.cpp $(USERSPACE)
	$(CXX) $(CXX_PARAMS) -c -o $@ $<

$(KERNEL_BIN): $(KERNEL_LINKER) $(OBJ_FILES)
	ld $(LD_PARAMS) -T $< -o $@ $(OBJ_FILES)

.PHONY build-iso:
$(KERNEL_ISO): $(KERNEL_BIN)
	mkdir -p iso/boot/grub
	cp $(KERNEL_BIN) iso/boot/
	cp kernel/src/boot/menu.lst iso/boot/grub
	cp kernel/src/boot/stage2_eltorito iso/boot/grub
	genisoimage -R                           \
                -b boot/grub/stage2_eltorito \
                -no-emul-boot                \
                -boot-load-size 4            \
                -boot-info-table             \
                -input-charset utf8          \
                -o $(KERNEL_ISO)             \
                iso
	rm -rf iso

.PHONY install-dependencies:
install-dependencies:
	sudo apt-get update
	sudo apt-get install -y nasm            \
                            build-essential \
                            genisoimage

.PHONY run-qemu:
run-qemu: $(KERNEL_ISO)
	qemu-system-x86_64 -boot d -cdrom $(KERNEL_ISO) -m 512

.PHONY run-vbox:
run-vbox: $(KERNEL_ISO)
	(killall VirtualBoxVM && sleep 1) || true
	VirtualBoxVM --startvm "$(VM_NAME)"

.PHONY compile-userspace:
compile-userspace:
	make -C userspace

.PHONY clean:
	cd userspace && make clean
	rm $(KERNEL_BIN) || true
	rm $(KERNEL_ISO) || true
	rm $(OBJ_FILES)  || true