E=circsteer.exe
Z=circsteer-${shell date -I}.zip
CXX=g++
CXXFLAGS=-I . -Wall -O3 -march=core2 -mtune=generic
LDLIBS=-lxinput -lwinmm -lvJoyInterface -l:pdcurses.a -static-libgcc -static-libstdc++
LDFLAGS=-L vJoy/SDK/lib/amd64
OFILES=src/main.o src/uicon.o src/uicurses.o
RELFILES=$E circsteer.cfg vJoyInterface.dll README.md Makefile LICENSE.txt

all: $E
$E: ${OFILES} src/circsteer.res vJoyInterface.dll
	${CXX} ${LDFLAGS} -o $@ ${OFILES} src/circsteer.res ${LDLIBS}
main.o: src/a.h
uicon.o: src/a.h
uicurses.o: src/a.h
src/circsteer.res: src/circsteer.rc
	windres $^ -O coff $@
vJoyInterface.dll:
	cp vJoy/SDK/lib/amd64/vJoyInterface.dll ./
rel: $Z
$Z: ${RELFILES}
	strip -s $E
	zip $@ $^
clean:
	rm -f $E ${OFILES} $R circsteer*.zip vJoy*.dll
