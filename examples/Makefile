CC=cc2600
CFLAGS=-I../headers

%.bin: %.c
	$(CC) -o $@ $< $(CFLAGS)

all: example_sprite.bin test_superchip.bin test_dpc.bin test_dpcplus.bin test_3e.bin test_3eplus.bin armor_ambush_reloaded.bin

clean:
	rm *.bin


