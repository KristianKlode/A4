.PHONY: all clean

all:
	make -C kudos
	make -C userland

clean:
	make -C docs clean
	make -C kudos clean
	make -C userland clean
	rm -f store.file
