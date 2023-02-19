# Here's the stuff you need to build on Windows and Linux.

# You need SDL2 (http://www.libsdl.org) to talk to the OS
#########
# WINDOWS SDL2
#########
# MSYS2:
#   pacman -S mingw-w64-x86_64-SDL2
#######
# LINUX SDL2
#######
#   sudo apt install libsdl2-dev

# You need pkgconf / pkg-config to automate compiler and linker flags:
#########
# WINDOWS pkg-config
#########
# MSYS2:
#   pacman -S pkg-config
# Tag-hopping and omni-complete:
# MSYS2:
#   pacman -S ctags
#######
# LINUX pkgconf
#######
#   sudo apt install pkgconf

run: falling-something
	./falling-something

falling-something: main.c
	gcc $(CFLAGS) -o $@ $< $(LFLAGS)

# pkg-config -h
# --cflags                          print required CFLAGS to stdout
# --libs                            print required linker flags to stdout
CFLAGS = `pkg-config --cflags sdl2`
LFLAGS = `pkg-config --libs sdl2`

.PHONY: tags
tags: main.c
	ctags --c-kinds=+l --exclude=Makefile -R .

.PHONY: lib-tags
lib-tags: main.c
	gcc $(CFLAGS) $< -M > headers-windows.txt
	python.exe parse-lib-tags.py
	rm -f headers-windows.txt
	ctags -f lib-tags --c-kinds=+p -L headers-posix.txt
	rm -f headers-posix.txt

.PHONY: clean
clean:
	rm -f falling-something.exe

what-CFLAGS:
	@echo $(CFLAGS)
what-LFLAGS:
	@echo $(LFLAGS)
where-is-gcc:
	@whereis gcc
