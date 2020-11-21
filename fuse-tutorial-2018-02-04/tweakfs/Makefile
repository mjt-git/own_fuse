all: tweakfs

clean:
	rm -f tweakfs

tweakfs: bbfs.c log.c
	gcc -Wall $^ `pkg-config fuse --cflags --libs` -o $@

