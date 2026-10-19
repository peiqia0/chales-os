DEFAULT_HOST!=../default-host.sh
HOST?=DEFAULT_HOST
HOSTARCH!=../target-triplet-to-arch.sh $(HOST)

CFLAGS?=-O2 -g
CPPFLAGS?=
LDFLAGS?=
LIBS?=

DESTDIR?=
PREFIX?=/usr/local
EXEC_PREFIX?=$(PREFIX)
INCLUDEDIR?=$(PREFIX)/include
LIBDIR?=$(EXEC_PREFIX)/lib

CFLAGS:=$(CFLAGS) -ffreestanding -fno-builtin -fno-tree-loop-distribute-patterns -Wall -Wextra
CPPFLAGS:=$(CPPFLAGS) -D__is_libc -Iinclude
LIBK_CFLAGS:=$(CFLAGS)
LIBK_CPPFLAGS:=$(CPPFLAGS) -D__is_libk

ARCHDIR=arch/$(HOSTARCH)

include $(ARCHDIR)/make.config

CFLAGS:=$(CFLAGS) $(ARCH_CFLAGS)
CPPFLAGS:=$(CPPFLAGS) $(ARCH_CPPFLAGS)
LIBK_CFLAGS:=$(LIBK_CFLAGS) $(KERNEL_ARCH_CFLAGS)
LIBK_CPPFLAGS:=$(LIBK_CPPFLAGS) $(KERNEL_ARCH_CPPFLAGS)

FREEOBJS=\
$(ARCH_FREEOBJS) \
stdio/printk.o \
stdio/putchar.o \
stdio/puts.o \
stdio/snprintk.o \
stdlib/abort.o \
stdlib/kmalloc.o \
string/memcmp.o \
string/memcpy.o \
string/memmove.o \
string/memset.o \
string/strlen.o \
string/strcpy.o \
string/strcat.o \
string/strcmp.o \
string/strncmp.o \
string/strrchr.o \

LIBK_OBJS=$(FREEOBJS:.o=.libk.o)

# libk.a is the kernel's own freestanding library. User programs use newlib;
# libchalessys.a is the syscall layer newlib needs underneath it.
BINARIES=libk.a libchalessys.a

.PHONY: all clean install install-headers install-libs
.SUFFIXES: .o .libk.o .c .S

all: $(BINARIES)

libk.a: $(LIBK_OBJS)
	$(AR) rcs $@ $(LIBK_OBJS)

.c.o:
	$(CC) -MD -c $< -o $@ -std=gnu11 $(CFLAGS) $(CPPFLAGS)

.c.S:
	$(CC) -MD -c $< -o $@ $(CFLAGS) $(CPPFLAGS)

.c.libk.o:
	$(CC) -MD -c $< -o $@ -std=gnu11 $(LIBK_CFLAGS) $(LIBK_CPPFLAGS)

.S.libk.o:
	$(CC) -MD -c $< -o $@ $(LIBK_CFLAGS) $(LIBK_CPPFLAGS)

clean:
	rm -f $(BINARIES) *.a newlib/*.o newlib/*.d
	rm -f $(LIBK_OBJS) *.o */*.o */*/*.o
	rm -f $(LIBK_OBJS:.o=.d) *.d */*.d */*/*.d

install: install-headers install-libs

install-headers:
	mkdir -p $(DESTDIR)$(INCLUDEDIR)
	cp -R --preserve=timestamps include/. $(DESTDIR)$(INCLUDEDIR)/.

install-libs: $(BINARIES)
	mkdir -p $(DESTDIR)$(LIBDIR)
	cp $(BINARIES) $(DESTDIR)$(LIBDIR)

-include $(LIBK_OBJS:.o=.d)

# ---- newlib support ---------------------------------------------------------
# The syscall layer newlib needs (_write, _sbrk, _open, ...). It is compiled
# against newlib's headers, so it is not part of the default build. Point
# NEWLIB_PREFIX at a newlib install (it contains $(HOST)/include and
# $(HOST)/lib):
#   make libchalessys.a NEWLIB_PREFIX=$HOME/opt/newlib
NEWLIB_PREFIX?=
ifeq ($(NEWLIB_PREFIX),)
$(error NEWLIB_PREFIX is not set: point it at a newlib install containing $(HOST)/include and $(HOST)/lib)
endif
NEWLIB_INCLUDE=$(NEWLIB_PREFIX)/$(HOST)/include
NEWLIB_OBJS=newlib/newlib_syscalls.o

# -idirafter: newlib's own headers win; only sys/chales_abi.h comes from include/.
newlib/%.o: newlib/%.c
	$(CC) -MD -c $< -o $@ -std=gnu11 $(CFLAGS) -D__is_libc -nostdinc -I$(NEWLIB_INCLUDE) -idirafter include -isystem $(shell $(CC) -print-file-name=include)

libchalessys.a: $(NEWLIB_OBJS)
	$(AR) rcs $@ $(NEWLIB_OBJS)

install-newlib-libs: libchalessys.a
	mkdir -p $(DESTDIR)$(LIBDIR)
	cp libchalessys.a $(DESTDIR)$(LIBDIR)

-include $(NEWLIB_OBJS:.o=.d)
