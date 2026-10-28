HOST?=$(shell ../default-host.sh)
AS=$(HOST)-as
LD=$(HOST)-ld

STAGE2_MAX=7680   # 15 sectors, see layout.inc

all: stage1.bin stage2.bin

%.o: %.s layout.inc
	$(AS) -I. -o $@ $<

stage1.bin: stage1.o
	$(LD) -Ttext 0x7C00 --oformat binary -o $@ $<
	@test `wc -c < $@` -eq 512 || { echo "stage1 must be exactly 512 bytes"; rm -f $@; exit 1; }

stage2.bin: stage2.o
	$(LD) -Ttext 0x7E00 --oformat binary -o $@ $<
	@test `wc -c < $@` -le $(STAGE2_MAX) || { echo "stage2 is larger than $(STAGE2_MAX) bytes"; rm -f $@; exit 1; }

clean:
	rm -f *.o *.bin

.PHONY: all clean
