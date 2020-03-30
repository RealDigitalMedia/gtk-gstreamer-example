BUILD_PACKAGES=gtk+-3.0 gstreamer-1.0 gstreamer-video-1.0 x11
CC=g++
CFLAGS=-g -Wall -Wno-multichar -fPIC $(shell pkg-config --cflags --libs ${BUILD_PACKAGES})

all: player

pkg_config_check:
	pkg-config --cflags --libs ${BUILD_PACKAGES}

player: player.o
	$(CC) -o $@ $^ $(INCLUDES) $(CFLAGS)

%.o: %.cpp
	$(CC) -o $@ -c $< $(INCLUDES) $(CFLAGS)

%.o: %.cc
	$(CC) -o $@ -c $< $(INCLUDES) $(CFLAGS)
clean:
	rm -f player
	find . -name "*.o" -delete
	find . -name "*.so" -delete

