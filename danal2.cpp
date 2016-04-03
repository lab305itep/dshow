#include <TFile.h>
#include <TH1D.h>
#include <TH2D.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "recformat.h"

#define MAXHIT 500
#define ESIZE (sizeof(struct pre_event_struct) + MAXHIT * sizeof(struct pre_hit_struct))
#define MAXWFD 70
#define MINSIPM4TIME	60
#define MINPMT4TIME	20
#define PMT2SIPM4TIME	2
#define TCUT		12.5
#define MAXTDIFF	50
#define FREQ		125.0
#define E1MIN		1.0
#define E2MIN		4.0
#define SHEIGHT		1.0
#define SWIDTH		4.0
#define VETOEMIN	1.0
#define VETONMIN	2

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
	long long gtime;	// 125MHz global time counter
	int systime;		// Linux system time 
	int number;		// Event number
	int nhits;		// number of hits
	int ns;			// number of strips
	int np;			// number of PMT
	int nv;			// number of VETO
	float es;		// SiPM energy
	float ep;		// PMT energy
	float ev;		// VETO energy
	float ee;		// Positron energy
	float t;		// precise time
	float x;		// x, y, z of the vertex
	float y;
	float z;
	struct hit_struct hit[0];
};

#define EESIZE (sizeof(struct event_struct) + MAXHIT * sizeof(struct hit_struct))

union hist_union {
	TObject *obj[0];
	struct hist_struct {
		TH1D *hDt[MAXWFD];
		TH1D *hDtc[MAXWFD];
		TH1D *hDtr[MAXWFD];
		TH1D *hDtn[3][MAXWFD];
		TH1D *hT[4];
		TH2D *hEN[5];
		TH2D *hESEP[5];
		TH2D *hXZ[5];
		TH2D *hYZ[5];
		TH2D *mXZ[5];
		TH2D *mYZ[5];
		TH1D *hM;
		TH1D *hV;
		TH1D *hR;
		TH1D *hE;
	} h;
} Hist;

void BookHist(void)
{
	char strs[32];
	char strl[1024];
	int i;
	for (i=0; i<MAXWFD; i++) {
		sprintf(strs, "hDt%2.2d", i+1);
		sprintf(strl, "Time from average for Module %d;ns", i+1);
		Hist.h.hDt[i] = new TH1D(strs, strl, 100, -5*TCUT, 5*TCUT);
		sprintf(strs, "hDtc%2.2d", i+1);
		sprintf(strl, "Time from average for Module %d, A>30;ns", i+1);
		Hist.h.hDtc[i] = new TH1D(strs, strl, 100, -5*TCUT, 5*TCUT);
		sprintf(strs, "hDtr%2.2d", i+1);
		sprintf(strl, "Time from average for Module %d, 60>A>30;ns", i+1);
		Hist.h.hDtr[i] = new TH1D(strs, strl, 100, -5*TCUT, 5*TCUT);
		sprintf(strs, "hDtn0%2.2d", i+1);
		sprintf(strl, "Time from average for Module %d, second event;ns", i+1);
		Hist.h.hDtn[0][i] = new TH1D(strs, strl, 100, -5*TCUT, 5*TCUT);
		sprintf(strs, "hDtn1%2.2d", i+1);
		sprintf(strl, "Time from average for Module %d, third event;ns", i+1);
		Hist.h.hDtn[1][i] = new TH1D(strs, strl, 100, -5*TCUT, 5*TCUT);
		sprintf(strs, "hDtn2%2.2d", i+1);
		sprintf(strl, "Time from average for Module %d, fourth event;ns", i+1);
		Hist.h.hDtn[2][i] = new TH1D(strs, strl, 100, -5*TCUT, 5*TCUT);
	}
	Hist.h.hT[0] = new TH1D("hT0", "Second to first time difference;us", 100, 0, MAXTDIFF);
	Hist.h.hT[1] = new TH1D("hT1", "Second to first time difference 1, selected events;us", 100, 0, MAXTDIFF);
	Hist.h.hT[2] = new TH1D("hT2", "Second to first time difference 2, selected events;us", 100, 0, MAXTDIFF);
	Hist.h.hT[3] = new TH1D("hT3", "Second to first time difference 3, selected events;us", 100, 0, MAXTDIFF);

	Hist.h.hEN[0] = new TH2D("hEN0", "Energy versus multiplicity for SiPM, no cuts;;MeV", 20, 0, 20, 60, 0, 15);
	Hist.h.hEN[1] = new TH2D("hEN1", "Energy versus multiplicity for SiPM, selected the first event;;MeV", 20, 0, 20, 60, 0, 15);
	Hist.h.hEN[2] = new TH2D("hEN2", "Energy versus multiplicity for SiPM, selected the second event;;MeV", 20, 0, 20, 60, 0, 15);
	Hist.h.hEN[3] = new TH2D("hEN3", "Energy versus multiplicity for SiPM, selected the third event;;MeV", 20, 0, 20, 60, 0, 15);
	Hist.h.hEN[4] = new TH2D("hEN4", "Energy versus multiplicity for SiPM, selected the fourth event;;MeV", 20, 0, 20, 60, 0, 15);

	Hist.h.hESEP[0] = new TH2D("hESEP0", "PMT versus SiPM energy, no cuts;MeV;MeV", 60, 0, 15, 60, 0, 15);
	Hist.h.hESEP[1] = new TH2D("hESEP1", "PMT versus SiPM energy, selected the first event;MeV;MeV", 60, 0, 15, 60, 0, 15);
	Hist.h.hESEP[2] = new TH2D("hESEP2", "PMT versus SiPM energy, selected the second event;MeV;MeV", 60, 0, 15, 60, 0, 15);
	Hist.h.hESEP[3] = new TH2D("hESEP3", "PMT versus SiPM energy, selected the third event;MeV;MeV", 60, 0, 15, 60, 0, 15);
	Hist.h.hESEP[4] = new TH2D("hESEP4", "PMT versus SiPM energy, selected the fourth event;MeV;MeV", 60, 0, 15, 60, 0, 15);

	Hist.h.hXZ[0] = new TH2D("hXZ0", "XZ - SiPM distribution, no cuts", 25, 0, 25, 50, 0, 50);
	Hist.h.hXZ[1] = new TH2D("hXZ1", "XZ - SiPM distribution, selected the first event", 25, 0, 25, 50, 0, 50);
	Hist.h.hXZ[2] = new TH2D("hXZ2", "XZ - SiPM distribution, selected the second event", 25, 0, 25, 50, 0, 50);
	Hist.h.hXZ[3] = new TH2D("hXZ3", "XZ - SiPM distribution, selected the third event", 25, 0, 25, 50, 0, 50);
	Hist.h.hXZ[4] = new TH2D("hXZ4", "XZ - SiPM distribution, selected the fourth event", 25, 0, 25, 50, 0, 50);

	Hist.h.hYZ[0] = new TH2D("hYZ0", "YZ - SiPM distribution, no cuts", 25, 0, 25, 50, 0, 50);
	Hist.h.hYZ[1] = new TH2D("hYZ1", "YZ - SiPM distribution, selected the first event", 25, 0, 25, 50, 0, 50);
	Hist.h.hYZ[2] = new TH2D("hYZ2", "YZ - SiPM distribution, selected the second event", 25, 0, 25, 50, 0, 50);
	Hist.h.hYZ[3] = new TH2D("hYZ3", "YZ - SiPM distribution, selected the third event", 25, 0, 25, 50, 0, 50);
	Hist.h.hYZ[4] = new TH2D("hYZ4", "YZ - SiPM distribution, selected the fourth event", 25, 0, 25, 50, 0, 50);

	Hist.h.mXZ[0] = new TH2D("mXZ0", "XZ - center distribution, no cuts", 25, 0, 100, 25, 0, 100);
	Hist.h.mXZ[1] = new TH2D("mXZ1", "XZ - center distribution, selected the first event", 25, 0, 100, 25, 0, 100);
	Hist.h.mXZ[2] = new TH2D("mXZ2", "XZ - center distribution, selected the second event", 25, 0, 100, 25, 0, 100);
	Hist.h.mXZ[3] = new TH2D("mXZ3", "XZ - center distribution, selected the third event", 25, 0, 100, 25, 0, 100);
	Hist.h.mXZ[4] = new TH2D("mXZ4", "XZ - center distribution, selected the fourth event", 25, 0, 100, 25, 0, 100);

	Hist.h.mYZ[0] = new TH2D("mYZ0", "YZ - center distribution, no cuts", 25, 0, 100, 25, 0, 100);
	Hist.h.mYZ[1] = new TH2D("mYZ1", "YZ - center distribution, selected the first event", 25, 0, 100, 25, 0, 100);
	Hist.h.mYZ[2] = new TH2D("mYZ2", "YZ - center distribution, selected the second event", 25, 0, 100, 25, 0, 100);
	Hist.h.mYZ[3] = new TH2D("mYZ3", "YZ - center distribution, selected the third event", 25, 0, 100, 25, 0, 100);
	Hist.h.mYZ[4] = new TH2D("mYZ4", "YZ - center distribution, selected the fourth event", 25, 0, 100, 25, 0, 100);

	Hist.h.hM = new TH1D("hM", "Number of events in time window (50 us)", 20, 0, 20);
	Hist.h.hV = new TH1D("hV", "Veto energy", 100, 0, 10);
	Hist.h.hR = new TH1D("hR", "Distance between 1st and 2nd events;cm", 25, 0, 100);
	Hist.h.hE = new TH1D("hE", "\"Positron\" energy", 25, 0, 10);
}

void CalculateEventParameters(struct event_struct *Evt)
{
	int i;
	float sex, sey;	

	Evt->es = Evt->ep = Evt->ev = 0;
	Evt->ns = 0;
	Evt->x = Evt->y = Evt->z = 0;
	sex = sey = 0;
	for (i=0; i<Evt->nhits; i++) {
		switch(Evt->hit[i].type) {
		case TYPE_SIPM:
			Evt->es += Evt->hit[i].amp;
			Evt->ns++;
			if (Evt->hit[i].proj) {
				Evt->x += Evt->hit[i].xy * SWIDTH * Evt->hit[i].amp;
				sex += Evt->hit[i].amp;
			} else {
				Evt->y += Evt->hit[i].xy * SWIDTH * Evt->hit[i].amp;
				sey += Evt->hit[i].amp;
			}
			Evt->z += Evt->hit[i].z * SHEIGHT * Evt->hit[i].amp;
			break;
		case TYPE_PMT:
			Evt->ep += Evt->hit[i].amp;
			Evt->np++;
			break;
		case TYPE_VETO:
			Evt->ev += Evt->hit[i].amp;
			Evt->nv++;
			break;
		}
	}
	if (sex > 0) Evt->x /= sex;
	if (sey > 0) Evt->y /= sey;
	if (sex + sey > 0) Evt->z /= sex + sey;
}

void FillR(struct event_struct *Evt, struct event_struct *EvtOld)
{
	float R;
	
	R = 0;
	if (Evt->x <= 0 || EvtOld->x <= 0 || Evt->y <= 0 || EvtOld->y <= 0 || Evt->z <= 0 || EvtOld->z <= 0) return;
	R += (Evt->x - EvtOld->x) * (Evt->x - EvtOld->x);	
	R += (Evt->y - EvtOld->y) * (Evt->y - EvtOld->y);
	R += (Evt->z - EvtOld->z) * (Evt->z - EvtOld->z);
	R = sqrt(R);
	Hist.h.hR->Fill(R);
}

void FillN(struct pre_event_struct *event, float event_time, int num)
{
	struct chan_def_struct *chan;
	struct pre_hit_struct *hit;
	float delta;
	int i;

	for(i=0; i<event->nhits; i++) {
		hit = &event->hit[i];
		if (hit->mod < 1 || hit->mod > MAXWFD || hit->chan < 0 || hit->chan > 63) continue;
		chan = &Map[hit->mod-1][hit->chan];
		delta = hit->time - chan->dt - event_time;
		Hist.h.hDtn[num][hit->mod - 1]->Fill(delta);
	}
}

void FillXYZ(struct event_struct *Evt, int num)
{
	int i;
	for (i=0; i<Evt->nhits; i++) if (Evt->hit[i].type == TYPE_SIPM) {
		if (Evt->hit[i].proj) {
			Hist.h.hXZ[num]->Fill(Evt->hit[i].xy+0.5, Evt->hit[i].z/2+0.5, Evt->hit[i].amp);
		} else {
			Hist.h.hYZ[num]->Fill(24.5 - Evt->hit[i].xy, Evt->hit[i].z/2+0.5, Evt->hit[i].amp);
		}
	}
}

void FilterAndCopy(struct event_struct *Evt, struct pre_event_struct *event)
{
	struct chan_def_struct *chan;
	struct pre_hit_struct *hit;
	float delta;
	int i;
	
	Evt->nhits = 0;
	Evt->gtime = event->gtime;
	Evt->systime = event->systime;
	Evt->number = event->number;
	for(i=0; i<event->nhits; i++) {
		hit = &event->hit[i];
		if (hit->mod < 1 || hit->mod > MAXWFD || hit->chan < 0 || hit->chan > 63) continue;
		chan = &Map[hit->mod-1][hit->chan];
		delta = hit->time - chan->dt - Evt->t;
		Hist.h.hDt[hit->mod - 1]->Fill(delta);
		if (hit->amp > 30) Hist.h.hDtc[hit->mod - 1]->Fill(delta);
		if (hit->amp > 30 && hit->amp < 60) Hist.h.hDtr[hit->mod - 1]->Fill(delta);
		if (fabs(delta) > TCUT) continue;
		Evt->hit[Evt->nhits].time = hit->time - chan->dt;
		Evt->hit[Evt->nhits].amp  = hit->amp * chan->ecoef;
		Evt->hit[Evt->nhits].type = chan->type;
		Evt->hit[Evt->nhits].proj = chan->proj;
		Evt->hit[Evt->nhits].xy   = chan->xy;
		Evt->hit[Evt->nhits].z    = chan->z;
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
		chan = &Map[hit->mod-1][hit->chan];
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
				tsum += hit->amp * (hit->time - chan->dt) * PMT2SIPM4TIME;
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
	char str[1024];

	memset(Map, -1, sizeof(Map));
	f = fopen(fname, "rt");
	if (!f) {
		printf("Can not open map-file %s: %m\n", fname);
		return -10;
	}
	cnt = 0;
	while (!feof(f)) {
//		mod chan type proj xy z dt ecoef
		if (!fgets(str, sizeof(str), f)) break;
		if (str[0] != 'C') continue;
		sscanf(&str[1], "%d %d %d %d %d %d %f %f", &mod, &chan, &type, &proj, &xy, &z, &dt, &ecoef);
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
		Map[mod-1][chan].ecoef = ecoef / 1000;	// keV -> MeV
		cnt++;
	}
	fclose(f);
	printf("%s: %d channels found\n", fname, cnt);
	return 0;
}

void TryAsPositron(struct event_struct *Evt)
{
	int i, j;
	float A;
	float sex, sey, ex, ey, ez;	

//	Find MAX SiPM
	A = 0;
	j = -1;
	for (i=0; i<Evt->nhits; i++) if (Evt->hit[i].type == TYPE_SIPM && Evt->hit[i].amp > A) {
		A = Evt->hit[i].amp;
		j = i;
	}
	if (j < 0) return;
	
	sex = sey = 0;
	ex = ey = ez =0;
	for (i=0; i<Evt->nhits; i++) if (Evt->hit[i].type == TYPE_SIPM && 
		((Evt->hit[i].proj == Evt->hit[j].proj && Evt->hit[i].z == Evt->hit[j].z && abs(Evt->hit[i].xy - Evt->hit[j].xy) <= 1) ||
		 (Evt->hit[i].proj != Evt->hit[j].proj && abs(Evt->hit[i].z - Evt->hit[j].z) <= 1))) {
		Evt->ee += Evt->hit[i].amp;
		if (Evt->hit[i].proj) {
			ex += Evt->hit[i].amp * Evt->hit[i].xy * SWIDTH;
			sex += Evt->hit[i].amp * SWIDTH;
		} else {
			ey += Evt->hit[i].amp * Evt->hit[i].xy * SWIDTH;
			sey += Evt->hit[i].amp * SWIDTH;
		}
		ez += Evt->hit[i].amp * Evt->hit[i].z * SHEIGHT;
	}
	Evt->x = (sex > 0) ? ex / sex : 0;
	Evt->y = (sey > 0) ? ey / sey : 0;
	Evt->z = (sex + sey > 0) ? ez / (sex + sey) : 0;	
}

int main(int argc, char**argv)
{
	FILE *fIn;
	TFile *fOut;
	struct pre_event_struct *event;
	struct event_struct *Evt;
	struct event_struct *EvtOld;
	int i, j, irc;
	long long tbegin, tsum;
	int EvWin;
	time_t systime;
	int Cnt[20];
	const char *CntName[20] = {"Events read", "Time defined", "In diff time cut", "In energy cuts", "Global events", "Global events+2", "Global events+3", 
		"Veto", "", "", "", "", "", "", "", "", "", "", "", ""};

	Evt = NULL;
	event = NULL;
	fIn = NULL;
	fOut = NULL;
	tsum = 0;
	memset(Cnt, 0, sizeof(Cnt));
	memset(Hist.obj, 0, sizeof(Hist));

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
	EvtOld = (struct event_struct *) malloc(EESIZE);
	event = (struct pre_event_struct *) malloc(ESIZE);
	if (!event || !Evt || !EvtOld) {
		printf("Allocation failure: %m\n");
		goto fin;
	}
	BookHist();
	
//		Book histogramms and trees
		
	for (j=1; j<argc; j++) {
		fIn = fopen(argv[j], "rb");
		if (!fIn) {
			printf("Can not open input file %s: %m\n", argv[j]);
			continue;
		}
		EvtOld->gtime = -125000000;
		tbegin = -1;
		EvWin = 0;
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
			if (tbegin < 0) {
				printf("File %s. Start: %s", argv[j], ctime(&systime));
				tbegin = event->gtime;
			}
			Cnt[0]++;
			memset(Evt, 0, sizeof(struct event_struct));
			Evt->t = FindEventTime(event);
			if (Evt->t < 0) continue;
			Cnt[1]++;
			FilterAndCopy(Evt, event);
			CalculateEventParameters(Evt);
			Hist.h.hT[0]->Fill((Evt->gtime - EvtOld->gtime) / FREQ);
			Hist.h.hEN[0]->Fill(Evt->ns, Evt->es);
			Hist.h.hESEP[0]->Fill(Evt->es, Evt->ep);
			FillXYZ(Evt, 0);
			Hist.h.hV->Fill(Evt->ev);
			Hist.h.mXZ[0]->Fill(Evt->x, Evt->z);
			Hist.h.mYZ[0]->Fill(Evt->y, Evt->z);
			if (Evt->ev >= VETOEMIN || Evt->nv >= VETONMIN) {
				Cnt[7]++;
				continue;
			}
			if (Evt->gtime - EvtOld->gtime <= MAXTDIFF * FREQ) {
				Cnt[2]++;
				if (Evt->ep > E2MIN && Evt->es > E2MIN) {
					Cnt[3]++;
					switch (EvWin) {
					case 0:
						Hist.h.hT[1]->Fill((Evt->gtime - EvtOld->gtime) / FREQ);
						Hist.h.hEN[1]->Fill(EvtOld->ns, EvtOld->es);
						Hist.h.hESEP[1]->Fill(EvtOld->es, EvtOld->ep);
						Hist.h.mXZ[1]->Fill(EvtOld->x, EvtOld->z);
						Hist.h.mYZ[1]->Fill(EvtOld->y, EvtOld->z);
						FillXYZ(EvtOld, 1);
						Hist.h.hEN[2]->Fill(Evt->ns, Evt->es);
						Hist.h.hESEP[2]->Fill(Evt->es, Evt->ep);
						FillXYZ(Evt, 2);
						Hist.h.mXZ[2]->Fill(Evt->x, Evt->z);
						Hist.h.mYZ[2]->Fill(Evt->y, Evt->z);
						FillN(event, Evt->t, 0);
						FillR(Evt, EvtOld);
						Hist.h.hE->Fill(EvtOld->ee);
						Cnt[4]++;
						break;
					case 1:
						Hist.h.hT[2]->Fill((Evt->gtime - EvtOld->gtime) / FREQ);
						Hist.h.hEN[3]->Fill(Evt->ns, Evt->es);
						Hist.h.hESEP[3]->Fill(Evt->es, Evt->ep);
						FillXYZ(Evt, 3);
						Hist.h.mXZ[3]->Fill(Evt->x, Evt->z);
						Hist.h.mYZ[3]->Fill(Evt->y, Evt->z);
						FillN(event, Evt->t, 1);
						Cnt[5]++;
						break;
					case 2:
						Hist.h.hT[3]->Fill((Evt->gtime - EvtOld->gtime) / FREQ);
						Hist.h.hEN[4]->Fill(Evt->ns, Evt->es);
						Hist.h.hESEP[4]->Fill(Evt->es, Evt->ep);
						FillXYZ(Evt, 4);
						Hist.h.mXZ[4]->Fill(Evt->x, Evt->z);
						Hist.h.mYZ[4]->Fill(Evt->y, Evt->z);
						FillN(event, Evt->t, 2);
						Cnt[6]++;
						break;
					}
					EvWin++;
				}
			} else {
				TryAsPositron(Evt);
				if (Evt->ep > E1MIN && Evt->es > E1MIN && Evt->ee > E1MIN) memcpy(EvtOld, Evt, EESIZE);
				Hist.h.hM->Fill(1.0 * EvWin);
				EvWin = 0;
			}
		}
		printf("End: %s", ctime(&systime));
		tsum += event->gtime - tbegin;
		fclose(fIn);
	}
	printf("Total time %f s.\n", tsum / (FREQ*1000000.0));
	printf("Counters:\n");
	for (i = 0; i < sizeof(Cnt)/sizeof(Cnt[0]); i++) if (Cnt[i]) printf("%50s: %10d\n", CntName[i], Cnt[i]);
	for (i = 0; i< sizeof(Hist) / sizeof(Hist.obj[0]); i++) if (Hist.obj[i]) {
		Hist.obj[i]->Write();
		delete Hist.obj[i];
	}
fin:
	if (event) free(event);
	if (Evt) free(Evt);
	if (EvtOld) free(EvtOld);
	if (fOut && fOut->IsOpen()) {
		fOut->Close();
		delete fOut;
	}

	return 0;
}

