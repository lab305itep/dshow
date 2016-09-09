ROOTCINT=rootcint
#ROOTCINT=rootcling

all : dshow danal1 danal2

dshow : dshow.cpp dshowDict.cpp
	g++ -g -o dshow dshow.cpp dshowDict.cpp `root-config --cflags --glibs` -lconfig

dshowDict.cpp : dshowLinkDef.h dshow.h
	$(ROOTCINT) -f dshowDict.cpp -c dshow.h dshowLinkDef.h

danal1 : danal1.cpp
	g++ -o $@ $^

danal2 : danal2.cpp
	g++ -g -o $@ $^ `root-config --cflags --glibs`

