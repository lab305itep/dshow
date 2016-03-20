#include <TFile.h>
#include <TH1D.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "recformat.h"

#define MAXHIT 500
#define ESIZE (sizeof(struct pre_event_struct) + MAXHIT * sizeof(struct pre_hit_struct))

TFile *OpenRootFile(const char *fname)
{
	char outname[1024];
	char *ptr;
	
	strcpy(outname, fname);
	ptr = strchr(outname, '.');
	if (ptr) *ptr = '\0';
	strcat(outname, ".root");
	return new TFile(outname, "RECREATE");
}

int main(int argc, char**argv)
{
	FILE *fIn;
	TFile *fOut;
	struct pre_event_struct *event;
	int i, irc;
	long long oldtime;
	float A;
	TH1D *hDt;
	TH1D *hMaxSi; 

	event = NULL;
	fIn = NULL;
	fOut = NULL;
	oldtime = -125000000;

//		Initializing files and memory
	if (argc < 1) {
		printf("Usage: ./danal2 file.rdat\n");
		goto fin;
	}

	fIn = fopen(argv[1], "rb");
	if (!fIn) {
		printf("Can not open input file %s: %m\n", argv[1]);
		goto fin;
	}

	fOut = OpenRootFile(argv[1]);
	if (!fOut->IsOpen()) {
		printf("Can not open output root file for %s\n", argv[1]);
		goto fin;
	}

	event = (struct pre_event_struct *) malloc(ESIZE);
	if (!event) {
		printf("Allocation failure %s: %m\n", argv[1]);
		goto fin;
	}
//		Book histogramms and trees
	hDt = new TH1D("hDt", "Time difference;us", 50, 0, 50);
	hMaxSi = new TH1D("hMaxSi", "Maximum SiPM energy in event;MeV", 500, 0, 10);
		
	for(;!feof(fIn);) {
		irc = fread(event, sizeof(int), 1, fIn);
		if (irc != 1) break;
		if (event->len > ESIZE || event->len < sizeof(struct pre_event_struct)) {
			printf("Odd block size: %d\n", event->len);
			break;
		}
		irc = fread(&event->systime, event->len - sizeof(int), 1, fIn);
		if (irc != 1) {
			printf("Unexpected EOF - %m\n");
			break;
		}
		hDt->Fill((event->gtime - oldtime) / 125.0);
		oldtime = event->gtime;
		A = 0;
		for (i=0; i<event->nhits; i++) if (event->hit[i].mod != 1 && event->hit[i].mod != 3 && event->hit[i].amp > A) A = event->hit[i].amp;
		hMaxSi->Fill(A);
	}

fin:
	hDt->Write();
	hMaxSi->Write();
	if (event) free(event);
	if (fIn) fclose(fIn);
	if (fOut && fOut->IsOpen()) {
		fOut->Close();
		delete fOut;
	}

	return 0;
}

