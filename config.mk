PREFIX = /usr/local
CFLAGS = -std=c99 -pedantic -Wall -Werror -D_GNU_SOURCE -O2 `pkg-config --cflags libudev`
LDFLAGS = `pkg-config --libs libudev`
