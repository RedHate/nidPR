TARGET = nidPR
OBJS =  mips/mips.o float/float.o snprintf/snprintf.o pspdebugkb/pspdebugkb.o exports/exports.o  main/crt0_prx.o 
#

# Define to build this as a prx (instead of a static elf)
BUILD_PRX=1
PSP_FW_VERSION=150
# Define the name of our custom exports (minus the .exp extension)
PRX_EXPORTS=exports/exports.exp

USE_KERNEL_LIBS = 1
USE_KERNEL_LIBC = 1

INCDIR = 
CFLAGS = -O2 -G0 -msingle-float -g
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBDIR =
LIBS =  -lpspdebug -lpspge_driver
LDFLAGS = -nostdlib -nodefaultlibs -g

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak

copy:
	make clean
	make
	sudo mount /dev/sda1 mount
	sudo cp  $(TARGET).prx mount/seplugins/$(TARGET).prx
	sudo umount mount
