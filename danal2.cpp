#include <TFile.h>
#include <TH1D.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "recformat.h"

#define MAXHIT 500
#define ESIZE (sizeof(struct pre_event_struct) + MAXHIT * sizeof(struct pre_hit_struct))
#define MAXWFD 70
//	12 per pixel * 1.5 crosstalk 15 p.e./MeV ~ 250
#define SIPM2MEV 250.0	
//	20 MeV ~ 2000 channels
#define PMT2MEV 100.0

struct chan_def_struct {
	char type;		// type = 0: SiPM, 1: PMT, 2: VETO
	char proj;		// proj = 0: Y, 1: X
	char xy;		// x or y 0 to 24
	char z;			// z: 0 to 99. y: even, x: odd.
	float dt;
	float ecoef;
};

struct chan_def_struct Map[MAXWFD][64];

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

int ReadMap(const char *fname)
{
	FILE *f;
	int type;
	int proj;
	int xy;
	int z;
	float dt;
	float ecoef;
	int mod, chan;
	int cnt;
	char c;

	memset(Map, -1, sizeof(Map));
	f = fopen(fname, "rt");
	if (!f) {
		printf("Can not open map-file %s: %m\n", fname);
		return -10;
	}
	cnt = 0;
	while (!feof(f)) {
//		mod chan type proj xy z dt ecoef
		fscanf(f, "%c %d %d %d %d %d %d %f %f\n", &c, &mod, &chan, &type, &proj, &xy, &z, &dt, &ecoef);
		if (c != 'C') continue;
		if (mod < 1 || mod > MAXWFD) {
			printf("Module number = %d out of range: 1 - %d\n", mod, MAXWFD);
			return -20;
		}
		if (chan < 0 || chan > 63) {
			printf("Channel number = %d out of range: 0 - 63\n", chan);
			return -20;
		}
		if (Map[mod-1][chan].type != -1) {
			printf("Duplicated channel found: %d.%2.2d\n", mod, chan);
			return -30;
		}
		Map[mod-1][chan].type  = type;
		Map[mod-1][chan].proj  = proj;
		Map[mod-1][chan].xy    = xy;
		Map[mod-1][chan].z     = z;
		Map[mod-1][chan].dt    = dt;
		Map[mod-1][chan].ecoef = ecoef;
		cnt++;
	}
	fclose(f);
	printf("%s: %d channels found\n", fname, cnt);
	return 0;
}

int main(int argc, char**argv)
{
	FILE *fIn;
	TFile *fOut;
	struct pre_event_struct *event;
	int i, j, irc;
	long long oldtime, oldtimec;
	long long tbegin, tsum;
	float A;
	float E;
	int flag;
	TH1D *hDt;
	TH1D *hDtc;
	TH1D *hMaxSi;
	TH1D *hMaxPMT;
	TH1D *hT;
	time_t systime;

	event = NULL;
	fIn = NULL;
	fOut = NULL;
	tsum = 0;

	if (ReadMap("danss_map.txt")) return 10;

//		Initializing files and memory
	if (argc < 2) {
		printf("Usage: ./danal2 file.rdat ...\n");
		return 20;
	}

	printf("%d files requested\n", argc - 1);
	fOut = OpenRootFile(argv[1]);
	if (!fOut->IsOpen()) {
		printf("Can not open output root file for %s\n", argv[1]);
		goto fin;
	}

	event = (struct pre_event_struct *) malloc(ESIZE);
	if (!event) {
		printf("Allocation failure: %m\n");
		goto fin;
	}
//		Book histogramms and trees
	hDt = new TH1D("hDt", "Time difference;us", 50, 0, 50);
	hDtc = new TH1D("hDtc", "Time difference, with PMT energy cuts;us", 50, 0, 50);
	hMaxSi = new TH1D("hMaxSi", "Maximum SiPM energy in event;MeV", 500, 0, 10);
	hMaxPMT = new TH1D("hMaxPMT", "Maximum PMT energy in event;MeV", 500, 0, 10);
	hT = new TH1D("hT", "Time in event distribution", 1000, -100, 100);
		
	for (j=1; j<argc; j++) {
		fIn = fopen(argv[j], "rb");
		if (!fIn) {
			printf("Can not open input file %s: %m\n", argv[j]);
			continue;
		}
		oldtime = -125000000;
		oldtimec = -125000000;
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
			systime = event->systime;
			if (oldtime < 0) {
				printf("File %s. Start: %s", argv[j], ctime(&systime));
				tbegin = event->gtime;
			}
			hDt->Fill((event->gtime - oldtime) / 125.0);
			oldtime = event->gtime;
			E = 0;
			for (i=0; i<event->nhits; i++) if (event->hit[i].mod == 1) E += event->hit[i].amp;
			E /= PMT2MEV;
			if (E > 2.0) hDtc->Fill((event->gtime - oldtimec) / 125.0);
			if (E > 0.7) oldtimec = event->gtime;
			A = 0;
			for (i=0; i<event->nhits; i++) if (event->hit[i].mod != 1 && event->hit[i].mod != 3 && event->hit[i].amp > A) A = event->hit[i].amp;
			if (E > 0.7) hMaxSi->Fill(A / SIPM2MEV);
			A = 0;
			for (i=0; i<event->nhits; i++) if (event->hit[i].mod == 1 && event->hit[i].amp > A) A = event->hit[i].amp;
			if (E > 0.7) hMaxPMT->Fill(A / PMT2MEV);
		}
		printf("End: %s", ctime(&systime));
		tsum += event->gtime - tbegin;
		fclose(fIn);
	}
	hDt->Write();
	hDtc->Write();
	hMaxSi->Write();
	hMaxPMT->Write();
	printf("Total time %f s.\n", tsum / 125000000.0);

fin:
	if (event) free(event);
	if (fOut && fOut->IsOpen()) {
		fOut->Close();
		delete fOut;
	}

	return 0;
}

