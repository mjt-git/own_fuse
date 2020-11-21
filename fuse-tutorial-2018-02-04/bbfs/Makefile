all: bbfs

clean:
	rm -f bbfs

bbfs: bbfs.c log.c
	gcc -Wall $^ `pkg-config fuse --cflags --libs` -o $@

