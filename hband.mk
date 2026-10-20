
.PHONY: build install

build:
	./configure --enable-maintainer-mode --enable-dependency-tracking CPPFLAGS=-D_FORTIFY_SOURCE=2 'CFLAGS=-g -ggdb -fvar-tracking-assignments -Wall -Werror -march=x86-64 -mtune=generic -O2 -pipe -fstack-protector-strong -fno-plt' 'CXXFLAGS=-g -ggdb -fvar-tracking-assignments -Wall -Werror -march=x86-64 -mtune=generic -O2 -pipe -fstack-protector-strong -fno-plt' LDFLAGS=-Wl,-O1,--sort-common,--as-needed,-z,relro,-z,now 'DEBUG_CFLAGS=-g -ggdb -fvar-tracking-assignments' 'DEBUG_CXXFLAGS=-g -ggdb -fvar-tracking-assignments' --no-create --no-recursion --enable-icewmtray --prefix=/usr
	mkdir -p build
	cd build && cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr && make

install:
	cd build && make install
