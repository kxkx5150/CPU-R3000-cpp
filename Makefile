OS         = `uname -s -r -m|sed \"s/ /-/g\"|tr \"[A-Z]\" \"[a-z]\"|tr \"/()\" \"___\"`
BUILDDIR   = .
CCC        = g++
CC         = gcc
GASM       = g++

INCLUDES   += -I./src/core -I./src/pcsxcore
OPT        = -O0
CCFLAGS    = -std=c++11 -g -Wno-unused-result $(OPT) -o pcsx -D USESDLSOUND
CFLAGS     = -std=gnu11 -g -Wpointer-sign -Wno-unused-result $(OPT) -o pcsx -D USESDLSOUND

OBJECTS    =  src/core/main.o src/core/pcsx.o src/bios/bios.o src/core/cdrom.o src/core/receiver.o src/core/counters.o src/core/dma.o src/core/sio.o \
src/core/hw.o src/core/mdec.o src/core/mem.o src/core/plugins.o src/core/r3000a.o \
src/core/gte.o src/utils/common.o src/core/cdriso.o src/plugins/dfxvideo/cfg.o \
src/plugins/dfxvideo/fps.o src/plugins/dfxvideo/prim.o src/plugins/dfxvideo/zn.o src/plugins/dfxvideo/draw.o src/plugins/dfxvideo/gpu.o \
src/plugins/dfxvideo/soft_modified.o src/plugins/dfsound/sdl.o src/plugins/dfsound/spu.o src/plugins/dfsound/cfg.o src/plugins/dfsound/dma.o \
src/plugins/dfsound/registers.o src/plugins/sdlinput/cfg.o src/plugins/sdlinput/pad.o src/plugins/sdlinput/xkb.o src/plugins/sdlinput/sdljoy.o src/plugins/sdlinput/analog.o

.SUFFIXES: .o .cpp .c .cc .h .m .i .s .obj

pcsx: $(OBJECTS)
	$(CCC) $(LDFLAGS) $(INCLUDES) -o $@ $(OBJECTS) -lm -lSDL2

.cpp.o:
	$(CCC) $(INCLUDES) -c $(CCFLAGS) $*.cpp -o $@

.c.o:
	$(CC) $(INCLUDES) -c $(CFLAGS) $*.c -o $@

.cpp.S:
	$(GASM) $(INCLUDES) -S $(CCFLAGS) $*.cpp -o $@

.cpp.i:
	$(GASM) $(INCLUDES) -E $(CCFLAGS) $*.cpp -o $@

.S.o:
	$(GASM) $(INCLUDES) -c $(CCFLAGS) $*.S -o $@

.S.i:
	$(GASM) $(INCLUDES) -c -E $(CCFLAGS) $*.S -o $@

.s.o:
	@echo Compiling $*.s
	sh-elf-as -little $*.s -o $@

.obj.o:
	cp $*.obj $*.o

clean:
	rm -f $(OBJECTS) $(ROMCHIPS) snes
