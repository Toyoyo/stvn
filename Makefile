SOURCES_DIR=./src
BUILD_DIR=./build
DIST_DIR=./dist
RSC_DIR=./rsc

CC=m68k-atari-mint-gcc
CFLAGS=-c -O3
LINKFLAGS=-lz -s -Wl,--gc-sections

# VASM PARAMETERS
ASM=vasmm68k_mot
ASMFLAGS=-Faout -quiet -x -m68000 -spaces -showopt

all: prepare dist

prepare:
	mkdir -p $(BUILD_DIR)

clean-compile : clean buffer.o screen.o main.o

sndhisr.o: prepare
	$(ASM) $(ASMFLAGS) $(SOURCES_DIR)/sndhisr.s -o $(BUILD_DIR)/sndhisr.o

sndh.o: prepare
	$(CC) $(CFLAGS) $(SOURCES_DIR)/sndh.c -o $(BUILD_DIR)/sndh.o

stvn.o: prepare
	$(CC) $(CFLAGS) $(SOURCES_DIR)/stvn.c -o $(BUILD_DIR)/stvn.o

line.o: prepare
	$(CC) $(CFLAGS) $(SOURCES_DIR)/line.c -o $(BUILD_DIR)/line.o

main: stvn.o line.o sndh.o sndhisr.o
	$(CC) \
	      $(BUILD_DIR)/*.o \
		  -o $(BUILD_DIR)/stvn.prg $(LINKFLAGS);

dist: main
	mkdir -p $(DIST_DIR)
	cp $(BUILD_DIR)/stvn.prg $(DIST_DIR)
	m68k-atari-mint-strip -s $(DIST_DIR)/stvn.prg
	cp -R $(RSC_DIR)/* $(DIST_DIR)

clean:
	rm -rf $(BUILD_DIR)
	rm -rf $(DIST_DIR)

