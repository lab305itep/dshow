#ROOTCINT=rootcint
ROOTCINT=rootcling


all : dshow dplay

dshow : dshow.cpp dshowDict.cpp
	g++ -o dshow dshow.cpp dshowDict.cpp `root-config --cflags --glibs` -lconfig

dshowDict.cpp : dshowLinkDef.h dshow.h
	$(ROOTCINT) -f dshowDict.cpp -c dshow.h dshowLinkDef.h

dplay : dplay.cpp

