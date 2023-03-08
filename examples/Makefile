CC=cc2600
CFLAGS=-I../headers

%.a: %.c
	$(CC) -o $@ $< $(CFLAGS)

%.bin: %.a
	dasm $< -f3 -v4 -o$@

all: example_sprite.bin test_superchip.bin test_dpc.bin test_dpcplus.bin test_3e.bin

clean:
	rm *.bin


