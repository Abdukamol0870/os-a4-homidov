# TURBO JANG! - Makefile
#
#   make            o'yinni yig'ish (CMake orqali, raylib avtomatik yuklab olinadi)
#   make run        yig'ish va ishga tushirish
#   make test       testlar (faqat gcc kerak, raylib shart emas)
#   make windows    Linux'da Windows uchun .exe (MinGW-w64)
#   make clean      yig'ilgan fayllarni o'chirish

CC      ?= gcc
CFLAGS  ?= -O2 -Wall -Wextra -std=c99
BUILD   ?= build

ifeq ($(OS),Windows_NT)
    EXE     := .exe
    NETLIBS := -lws2_32 -liphlpapi
else
    EXE     :=
    NETLIBS := -lpthread
endif

.PHONY: all run test windows clean

all:
	cmake -B $(BUILD) -DCMAKE_BUILD_TYPE=Release
	cmake --build $(BUILD) --target turbojang --config Release

run: all
	cd $(BUILD) && ./turbojang$(EXE)

# Testlar grafikasiz ishlaydi: tarmoq + to'liq poyga va 20 ta parkovka darajasi
net_test$(EXE): tests/net_test.c src/game.c src/track.c src/net.c src/plat.c src/*.h
	$(CC) $(CFLAGS) -D_DEFAULT_SOURCE -Isrc tests/net_test.c src/game.c src/track.c src/net.c src/plat.c -lm $(NETLIBS) -o $@

parking_test$(EXE): tests/parking_test.c src/parking.c src/parking.h
	$(CC) $(CFLAGS) -Isrc tests/parking_test.c src/parking.c -lm -o $@

test: net_test$(EXE) parking_test$(EXE)
	./net_test$(EXE)
	./parking_test$(EXE)

windows:
	cmake -B build-win -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64.cmake -DCMAKE_BUILD_TYPE=Release
	cmake --build build-win --target turbojang

clean:
	rm -rf $(BUILD) build-win net_test$(EXE) parking_test$(EXE)
