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

	install -d $(DESTDIR)$(PREFIX)/lib/systemd/system
	install -m 0644 data/timekeeper.service $(DESTDIR)$(PREFIX)/lib/systemd/system/

clean:
	rm -f $(TARGET)

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/sbin/$(TARGET)
	rm -rf $(DESTDIR)$(PREFIX)/lib/systemd/system/timekeeper.service
