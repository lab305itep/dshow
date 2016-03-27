#include <TFile.h>
#include <TH1D.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "recformat.h"

#define MAXHIT 500
#define ESIZE (sizeof(struct pre_event_struct) + MAXHIT * sizeof(struct pre_hit_struct))
#define MAXWFD 70
#define MINSIPM4TIME	60
#define MINPMT4TIME	10
#define PMT2SIPM4TIME	3
#define TCUT		10.0

struct chan_def_struct {
	char type;		// type = 0: SiPM, 1: PMT, 2: VETO
	char proj;		// proj = 0: Y, 1: X
	char xy;		// x or y 0 to 24
	char z;			// z: 0 to 99. y: even, x: odd.
	float dt;
	float ecoef;
};

struct chan_def_struct Map[MAXWFD][64];

struct hit_struct {
	float amp;
	float time;
	int type;
	int proj;
	int xy;
	int z;
};

struct event_struct {
	long long gtime;
	int systime;
	int number;
	int nhits;
	float e;
	float t;
	float x;
	float y;
	float z;
	struct hit_struct hit[0];
};

#define EESIZE (sizeof(struct event_struct) + MAXHIT * sizeof(struct hit_struct))

void FilterAndCopy(struct event_struct *Evt, struct pre_event_struct *event)
{
	struct chan_def_struct *chan;
	struct pre_hit_struct *hit;
	int i;
	
	for(i=0; i<event->nhits; i++) {
		hit = &event->hit[i];
		if (hit->mod < 1 || hit->mod > MAXWFD || hit->chan < 0 || hit->chan > 63) continue;
		chan = &Map[hit->mod][hit->chan];
		if (fabs(hit->time - chan->dt - Evt->t) > TCUT) continue;
		Evt->hit[Evt->nhits].time = hit->time - chan->dt;
		Evt->hit[Evt->nhits].amp = hit->amp * chan->ecoef;
		Evt->hit[Evt->nhits].type = chan->type;
		Evt->hit[Evt->nhits].proj = chan->proj;
		Evt->hit[Evt->nhits].xy = chan->xy;
		Evt->hit[Evt->nhits].z = chan->z;
		Evt->nhits++;
	}
}

float FindEventTime(struct pre_event_struct *event)
{
	float tsum;
	float asum;
	int i;
	struct chan_def_struct *chan;
	struct pre_hit_struct *hit;
	
	tsum = asum = 0;
	for (i=0; i<event->nhits; i++) {
		hit = &event->hit[i];
		if (hit->mod < 1 || hit->mod > MAXWFD || hit->chan < 0 || hit->chan > 63) {
			printf("Strange module/channel met: %d.%d\n", hit->mod, hit->chan);
			continue;
		}
		chan = &Map[hit->mod][hit->chan];
		switch (chan->type) {
		case TYPE_SIPM:
			if (hit->amp >= MINSIPM4TIME) {
				tsum += hit->amp * (hit->time - chan->dt);
				asum += hit->amp;
			}
			break;
		case TYPE_PMT:
		case TYPE_VETO:
			if (hit->amp >= MINPMT4TIME) {
				tsum += hit->amp * hit->time * PMT2SIPM4TIME;
				asum += hit->amp * PMT2SIPM4TIME;
			}
			break;
		}
	}
	return (asum > 0) ? tsum / asum : -1000;
}

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
	struct event_struct *Evt;
	int i, j, irc;
	long long oldtime;
	long long tbegin, tsum;
	float A;
	float E;
	int flag;
	time_t systime;
	int Cnt[20];
	const char *CntName[20] = {"Events read", "Time defined", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", ""};

	Evt = NULL;
	event = NULL;
	fIn = NULL;
	fOut = NULL;
	tsum = 0;
	memset(Cnt, 0, sizeof(Cnt));

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

	Evt = (struct event_struct *) malloc(EESIZE);
	event = (struct pre_event_struct *) malloc(ESIZE);
	if (!event || !Evt) {
		printf("Allocation failure: %m\n");
		goto fin;
	}
	
//		Book histogramms and trees
		
	for (j=1; j<argc; j++) {
		fIn = fopen(argv[j], "rb");
		if (!fIn) {
			printf("Can not open input file %s: %m\n", argv[j]);
			continue;
		}
		oldtime = -125000000;
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
			Cnt[0]++;
			memset(Evt, 0, sizeof(struct event_struct));
			Evt->t = FindEventTime(event);
			if (Evt->t < 0) continue;
			FilterAndCopy(Evt, event);
			oldtime = event->gtime;
		}
		printf("End: %s", ctime(&systime));
		tsum += event->gtime - tbegin;
		fclose(fIn);
	}
	printf("Total time %f s.\n", tsum / 125000000.0);
	printf("Counters:\n");
	for (i = 0; i < sizeof(Cnt)/sizeof(Cnt[0]); i++) if (Cnt[i]) printf("%50s: %10d\n", CntName[i], Cnt[i]);

fin:
	if (event) free(event);
	if (Evt) free(Evt);
	if (fOut && fOut->IsOpen()) {
		fOut->Close();
		delete fOut;
	}

	return 0;
}

