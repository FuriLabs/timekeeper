CC = gcc

CFLAGS = `pkg-config --cflags gio-2.0`
LDFLAGS = `pkg-config --libs gio-2.0`

SOURCE = timekeeper.c

TARGET = timekeeper

PREFIX ?= /usr

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(SOURCE) -o $(TARGET) $(CFLAGS) $(LDFLAGS)

install: $(TARGET)
	install -d $(DESTDIR)$(PREFIX)/sbin
	install -m 0755 $(TARGET) $(DESTDIR)$(PREFIX)/sbin

clean:
	rm -f $(TARGET)

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/sbin/$(TARGET)
