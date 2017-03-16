//		Simple data analysis and show for DANSS
//	Analysis thread
//	I.G. Alekseev & D.N. Svirida, ITEP, Moscow, 2016-17

#define _FILE_OFFSET_BITS 64
#include <TApplication.h>
#include <TBox.h>
#include <TCanvas.h>
#include <TColor.h>
#include <TF1.h>
#include <TFile.h>
#include <TGaxis.h>
#include <TGButton.h>
#include <TGButtonGroup.h>
#include <TGClient.h>
#include <TGComboBox.h>
#include <TGFileDialog.h>
#include <TGLabel.h>
#include <TGNumberEntry.h>
#include <TGProgressBar.h>
#include <TGDoubleSlider.h>
#include <TGStatusBar.h>
#include <TGTab.h>
#include <TGraph.h>
#include <TH2D.h>
#include <TLegend.h>
#include <TLine.h>
#include <TROOT.h>
#include <TRootEmbeddedCanvas.h>
#include <TString.h>
#include <TStyle.h>
#include <TSystem.h>
#include <TText.h>
#include <TThread.h>
#include <errno.h>
#include <arpa/inet.h>
#include <libconfig.h>
#include <math.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "dshow.h"
#include "recformat.h"
#include "dshow_global.h"

/*	Process an event					*/
void ProcessEvent(char *data)
{
	struct hw_rec_struct *rec;
	struct rec_header_struct *head;
	int i, iptr, len;
	int amp;
	float t, at;
	int nat;
	char strl[1024];
	int mod, chan;
	int evt_len;
	void *ptr;
	int sumPMT, sumSiPM, sumSiPMc;
	int nSiPM, nSiPMc, nPMT;
	long long gTime;
	int gTrig;
	int nHits;
	int cHits;
	float dtM, dtN, dr, drZ;
	int PMTmask[2][5][5];

	sumPMT = 0;
	sumSiPM = 0;
	nSiPM = 0;
	sumSiPMc = 0;
	nSiPMc = 0;
	nPMT = 0;
	gTime = -1;
	nHits = 0;
	cHits = 0;

	head = (struct rec_header_struct *)data;
	for (iptr = sizeof(struct rec_header_struct); iptr < head->len; iptr += len) {		// record by record
		rec = (struct hw_rec_struct *) &data[iptr];
		len = (rec->len + 1) * sizeof(short);

		if (iptr + len > head->len) {
			Log("Unexpected end of event: iptr = %d, len = %d, total len = %d.\n", iptr, len, head->len);
			CommonData->ErrorCnt++;
			return;
		}
		if (rec->module < 1 || rec->module > MAXWFD) {
			Log("Module %d is out of range.\n", rec->module);
			CommonData->ErrorCnt++;
			continue;
		}
		mod = rec->module;
		chan = rec->chan;
		switch(rec->type) {
		case TYPE_MASTER:
			for (i = 0; i < rec->len - 2; i++) if (rec->d[i+1] & 0x4000) rec->d[i+1] |= 0x8000;		// do waveform data sign extension
			amp = FindMaxofShort(&rec->d[1], rec->len - 2);							// Amplitude
			t = NSPERCHAN * (FindHalfTime(&rec->d[1], rec->len - 2, amp) - rec->d[0] / 6.0);		// Time
//			Extract Waveform for drawing
			if (!CommonData->hWaveForm && rWaveTrig->IsOn() && nWFD->GetIntNumber() == mod && nChan->GetIntNumber() == chan 
				&& amp > nWaveThreshold->GetIntNumber()) {
				sprintf(strl, "Event WaveForm for %d.%2.2d", mod, chan);
				CommonData->hWaveForm = new TH1D("hWave", strl, rec->len - 2, 0, NSPERCHAN * (rec->len - 2));
				CommonData->hWaveForm->SetStats(0);
				for (i = 0; i < rec->len - 2; i++) CommonData->hWaveForm->SetBinContent(i + 1, (double) rec->d[i+1]);
				CommonData->thisWaveFormCnt = CommonData->EventCnt;
			}
//			Fill histogramms
			CommonData->hAmpE[mod-1]->Fill((double)chan, (double)amp);
			CommonData->hTimeA[mod-1]->Fill((double)chan, t);
			if (amp > CommonData->TimeBThreshold) CommonData->hTimeB[mod-1]->Fill((double)chan, t);
//			Add hit to event structure
			if (Map[mod-1][chan].type >= 0) {
				if (nHits >= EventLength) {
					ptr = realloc(Event, (EventLength + 1024) * sizeof(struct evt_disp_struct));
					if (!ptr) break;
					Event = (struct evt_disp_struct *)ptr;
					EventLength += 1024;
				}
				Event[nHits].mod = mod;
				Event[nHits].chan = chan;
				Event[nHits].amp = amp;
				Event[nHits].time = t;
				Event[nHits].type = Map[mod-1][chan].type;
				Event[nHits].xy = Map[mod-1][chan].xy;
				Event[nHits].z = Map[mod-1][chan].z;
				nHits++;
			}
			break;
		case TYPE_RAW:
			amp = FindMaxofShort(&rec->d[1], rec->len - 2);
//			Extract Waveform for drawing
			if (!CommonData->hWaveForm && rWaveTrig->IsOn() && nWFD->GetIntNumber() == mod && nChan->GetIntNumber() == chan
				&& amp > nWaveThreshold->GetIntNumber()) {
				sprintf(strl, "Event WaveForm for %d.%2.2d", mod, chan);
				CommonData->hWaveForm = new TH1D("hWave", strl, rec->len - 2, 0, NSPERCHAN * (rec->len - 2));
				CommonData->hWaveForm->SetStats(0);
				for (i = 0; i < rec->len - 2; i++) CommonData->hWaveForm->SetBinContent(i + 1, (double) rec->d[i+1]);
			}
			break;
		case TYPE_SUM:
			for (i = 0; i < rec->len - 2; i++) if (rec->d[i+1] & 0x4000) rec->d[i+1] |= 0x8000;		// do waveform data sign extension
			amp = FindMaxofShort(&rec->d[1], rec->len - 2);
//			Extract Waveform for drawing
			if (!CommonData->hWaveForm && rWaveHist->IsOn() && nWFD->GetIntNumber() == mod && nChan->GetIntNumber() == chan
				&& amp > nWaveThreshold->GetIntNumber()) {
				sprintf(strl, "Event WaveForm for %d.%2.2d", mod, chan);
				CommonData->hWaveForm = new TH1D("hWave", strl, rec->len - 2, 0, NSPERCHAN * (rec->len - 2));
				CommonData->hWaveForm->SetStats(0);
				for (i = 0; i < rec->len - 2; i++) CommonData->hWaveForm->SetBinContent(i + 1, (double) rec->d[i+1]);
			}
			break;
		case TYPE_TRIG:
			gTime = rec->d[1] + (((long long) rec->d[2]) << 15) + (((long long) rec->d[3]) << 30);
			gTrig = rec->d[4] + (((int) rec->d[5]) << 15);
			break;
		}
	}
	if (gTime < 0) {
		Log("Trigger record (type = 2) was not found.\n");
		CommonData->ErrorCnt++;
		return;
	}
//	check if a second passed
	if (CommonData->gLastTime < 0) {
		CommonData->gLastTime = gTime;
		CommonData->gTime = gTime;
		CommonData->gTrig = gTrig;
	} else if (gTime - CommonData->gLastTime >= RATEPER * ONESECOND) {
		CommonData->fRate[CommonData->iRatePos] = ((float) ONESECOND) * (gTrig - CommonData->gTrig) / (gTime - CommonData->gTime);
		CommonData->iRatePos = (CommonData->iRatePos + 1) % RATELEN;
		CommonData->gLastTime += RATEPER * ONESECOND;
		CommonData->gTime = gTime;
		CommonData->gTrig = gTrig;
	}
//
//		TimeC
	nat = 0;
	at = 0;
	for (i = 0; i<nHits; i++) if (Event[i].amp > CommonData->TimeBThreshold && Event[i].type == TYPE_SIPM) {
		at += Event[i].time;
		nat++;
	}
	if (nat) {
		at /= nat;
		if (nat > 1) for (i = 0; i<nHits; i++) if (Event[i].amp > CommonData->TimeBThreshold) CommonData->hTimeC[Event[i].mod - 1]->Fill((double) Event[i].chan, Event[i].time - at);
	} else {
		CommonData->hCuts->Fill(CUT_NOSITIME);
	}
//		Create PMTmask
	memset(PMTmask, 0, sizeof(PMTmask));
	for (i=0; i<nHits; i++) if (Event[i].type == TYPE_PMT && Event[i].xy >= 0) PMTmask[Event[i].z & 1][Event[i].z/2][Event[i].xy] = 1;
//		Clean the event
	if (CleanLength < EventLength) {
		ptr = realloc(CleanEvent, EventLength * sizeof(struct evt_disp_struct));
		if (!ptr) return;
		CleanEvent = (struct evt_disp_struct *) ptr;
		CleanLength = EventLength;
	}
	for (i=0; i<nHits; i++) {
		if (Event[i].xy < 0) continue;											// unknown position
		if (Event[i].type == TYPE_SIPM && ((!nat) || fabs(Event[i].time - at) > CommonData->SiPMWindow)) continue;	// time window
		if (Event[i].type == TYPE_SIPM && Event[i].amp < CommonData->TimeBThreshold && (!PMTmask[Event[i].z & 1][Event[i].z/20][Event[i].xy/5])) continue;	// support in PMT for small amplitudes
		memcpy(&CleanEvent[cHits], &Event[i], sizeof(struct evt_disp_struct));
		cHits++;
	}
	CalculateTags(cHits);
	EventStuck[EventStuckPos].gTime = gTime;
	for (i=0; i<cHits; i++) {
		switch (CleanEvent[i].type) {
		case TYPE_SIPM:
			sumSiPM += CleanEvent[i].amp;
			nSiPM++;
			if (CleanEvent[i].amp > CommonData->SummaSiPMThreshold) {
				sumSiPMc += CleanEvent[i].amp;
				nSiPMc++;					
			}
			break;
		case TYPE_PMT:
			sumPMT += CleanEvent[i].amp;
			nPMT++;
			break;
		}
	}
//	TH1D *hSiPMsum[2];	// SiPM summa, all/no veto
	CommonData->hSiPMsum[0]->Fill(sumSiPMc / SIPM2MEV);
//	TH1D *hSiPMhits[2];	// SiPM Hits, all/no veto
	CommonData->hSiPMhits[0]->Fill(1.0*nSiPMc);
//	TH1D *hPMTsum[2];	// PMT summa, all/no veto
	CommonData->hPMTsum[0]->Fill(sumPMT / PMT2MEV);
//	TH1D *hPMThits[2];	// PMT Hits, all/no veto
	CommonData->hPMThits[0]->Fill(1.0*nPMT);
//	TH1D *hSPRatio[2];	// SiPM sum to PTMT sum ratio, all/no veto
	if (sumPMT > 0) CommonData->hSPRatio[0]->Fill(sumSiPMc * PMT2MEV / (sumPMT * SIPM2MEV));
	if (!(EventFlags & FLAG_VETO)) {
//	TH1D *hSiPMsum[2];	// SiPM summa, all/no veto
		CommonData->hSiPMsum[1]->Fill(sumSiPMc / SIPM2MEV);
//	TH1D *hSiPMhits[2];	// SiPM Hits, all/no veto
		CommonData->hSiPMhits[1]->Fill(1.0*nSiPMc);
//	TH1D *hPMTsum[2];	// PMT summa, all/no veto
		CommonData->hPMTsum[1]->Fill(sumPMT / PMT2MEV);
//	TH1D *hPMThits[2];	// PMT Hits, all/no veto
		CommonData->hPMThits[1]->Fill(1.0*nPMT);
//	TH1D *hSPRatio[2];	// SiPM sum to PTMT sum ratio, all/no veto
		if (sumPMT > 0) CommonData->hSPRatio[1]->Fill(sumSiPMc * PMT2MEV / (sumPMT * SIPM2MEV));
	}
//		Copy event for drawing
	if ((!CommonData->EventHits) && sumPMT >= nPMTSumThreshold->GetIntNumber() && sumSiPM >= nSiPMSumThreshold->GetIntNumber() && 
		(rEvtAll->IsOn() || (rEvtNone->IsOn() && !EventFlags) || (rEvtVeto->IsOn() && (EventFlags & FLAG_VETO)) 
		|| (rEvtPositron->IsOn() && (EventFlags & FLAG_POSITRON)) || (rEvtNeutron->IsOn() && (EventFlags & FLAG_NEUTRON)))) {
		if (cHits > CommonData->EventLength) {
			ptr = realloc(CommonData->Event, cHits * sizeof(struct evt_disp_struct));
			if (ptr) {
				CommonData->Event = (struct evt_disp_struct *) ptr;
				memcpy(CommonData->Event, CleanEvent, cHits * sizeof(struct evt_disp_struct));
				CommonData->EventLength = cHits;
				CommonData->EventHits = cHits;
			}
		} else {
			memcpy(CommonData->Event, CleanEvent, cHits * sizeof(struct evt_disp_struct));
			CommonData->EventHits = cHits;
		}
		CommonData->EventFlags = EventFlags;
		CommonData->EventEnergy = EventEnergy;
		CommonData->thisEventCnt = CommonData->EventCnt;
	}
}

/*	Check event for veto/positron/neutron signature		*/
void dshowMainFrame::CalculateTags(int cHits)
{
	float xsum, exsum, ysum, eysum, zsum;
	int nx, ny;
	int ncHits;
	float eall, eclust, r2, xy0;
	float E;
	int i, j;
	int nVeto;
	float sumVeto;
	int newhits;

	eall = 0;
	for (i=0; i<cHits; i++) if (CleanEvent[i].type == TYPE_SIPM) eall += CleanEvent[i].amp;
	eall /= SIPM2MEV;

	EventFlags = 0;
	EventEnergy = eall;

//	VETO
	nVeto = 0;
	sumVeto  = 0;
	for (i=0; i<cHits; i++) if (CleanEvent[i].type == TYPE_VETO) {
		sumVeto += CleanEvent[i].amp;
		nVeto++;
	}
	
	if (nVeto > 1 || sumVeto > VETOMIN || EventEnergy > DVETOMIN) {
		EventFlags |= FLAG_VETO;
		// don't fill vertex
		CommonData->hCuts->Fill(CUT_VETO);
	}
//	Check for neutron
//		Calculate parameters
//		"Gamma" center
	xsum = ysum = zsum = 0;
	nx = ny = 0;
	
	for (i=0; i<cHits; i++) if (CleanEvent[i].type == TYPE_SIPM) {
		if (CleanEvent[i].z & 1) {	// X
			nx++;
			xsum += CleanEvent[i].xy;
		} else {			// Y
			ny++;
			ysum += CleanEvent[i].xy;
		}
		zsum += CleanEvent[i].z;
	}
	if (nx) {
		xsum *= WIDTH/nx;
	} else {
		xsum = -1;
		CommonData->hCuts->Fill(CUT_NONX);
	}
	if (ny) {
		ysum *= WIDTH/ny;
	} else {
		ysum = -1;
		CommonData->hCuts->Fill(CUT_NONY);
	}
	if (nx + ny) {
		zsum *= THICK/(nx + ny);
	} else {
		zsum = -1;
		CommonData->hCuts->Fill(CUT_NONZ);
	}
	EventStuck[EventStuckPos].VertexN[0] = xsum;
	EventStuck[EventStuckPos].VertexN[1] = ysum;
	EventStuck[EventStuckPos].VertexN[2] = zsum;

//		Count clusters and neutron energy
	ncHits = nx + ny;
//		Implement criteria
	if (ncHits >= Pars.nMin && EventEnergy > Pars.eNMin && EventEnergy < Pars.eNMax) {
		EventFlags |= FLAG_NEUTRON; 
		CommonData->hTagEnergy[1]->Fill(EventEnergy);
		CommonData->hTagXY[1]->Fill(xsum, ysum);
		CommonData->hTagZ[1]->Fill(zsum);
		CommonData->hCuts->Fill(CUT_NEUTRON);
	}
	if (ncHits < Pars.nMin) CommonData->hCuts->Fill(CUT_NMULT);
	if (eclust <= Pars.eNMin || eclust >= Pars.eNMax) CommonData->hCuts->Fill(CUT_NENERGY);
	
//		Check for pisitron
//		Find consequtive cluster near EMAX strip
//		find EMAX hit
	E = 0;
	j = 0;
	eclust = 0;
	xsum = ysum = zsum = 0;
	exsum = eysum = 0;
	for (i=0; i<cHits; i++) if (CleanEvent[i].type == TYPE_SIPM) if (CleanEvent[i].amp > E) {
		E = CleanEvent[i].amp;
		j = i;
	}
	if (E > 0) {
//		Add EMAX hit to the sum
		CleanEvent[j].in = 1;
		ncHits = 1;
		eclust = CleanEvent[j].amp;
		if (CleanEvent[j].z & 1) {	// X
			exsum = CleanEvent[j].amp;
			xsum = CleanEvent[j].amp * CleanEvent[j].xy;
		} else {			// Y
			eysum = CleanEvent[j].amp;
			ysum = CleanEvent[j].amp * CleanEvent[j].xy;
		}
		zsum = CleanEvent[j].amp * CleanEvent[j].z;
//		Look for neighbors
		for (;;) {
			newhits = 0;
			for (i=0; i<cHits; i++) for (j=0; j<cHits; j++) if (CleanEvent[i].type == TYPE_SIPM && CleanEvent[j].in && !CleanEvent[i].in && neighbors(CleanEvent[i].xy, CleanEvent[j].xy, CleanEvent[i].z, CleanEvent[j].z)) {
				if (CleanEvent[i].z & 1) {	// X
					exsum += CleanEvent[i].amp;
					xsum += CleanEvent[i].amp * CleanEvent[i].xy;
				} else {			// Y
					eysum += CleanEvent[i].amp;
					ysum += CleanEvent[i].amp * CleanEvent[i].xy;
				}
				zsum += CleanEvent[i].amp * CleanEvent[i].z;
				ncHits++;
				eclust += CleanEvent[i].amp;
				newhits++;
				CleanEvent[i].in = 1;
			}
			if (!newhits) break;
		}
	}
	eclust /= SIPM2MEV;
	if (exsum > 0) {
		xsum *= WIDTH/exsum;
	} else {
		xsum = -1;
	}
	if (eysum > 0) {
		ysum *= WIDTH/eysum;
	} else {
		ysum = -1;
	}
	if (exsum + eysum > 0) {
		zsum *= THICK/(exsum + eysum);
	} else {
		zsum = -1;
	}
	ClusterEnergy = eclust;
	EventStuck[EventStuckPos].VertexP[0] = xsum;
	EventStuck[EventStuckPos].VertexP[1] = ysum;
	EventStuck[EventStuckPos].VertexP[2] = zsum;
	
//		Implement criteria
	if (ncHits <= Pars.nClustMax && eclust > Pars.ePosMin && eclust < Pars.ePosMax && EventEnergy - eclust < Pars.eGammaMax) {
		EventFlags |= FLAG_POSITRON;
		EventEnergy = eclust;
		CommonData->hTagEnergy[0]->Fill(ClusterEnergy);
		CommonData->hTagXY[0]->Fill(xsum, ysum);
		CommonData->hTagZ[0]->Fill(zsum);
		CommonData->hCuts->Fill(CUT_POSITRON);
	}
	if (ncHits > Pars.nClustMax) CommonData->hCuts->Fill(CUT_PMULT);
	if (eclust <= Pars.ePosMin || eclust >= Pars.ePosMax) CommonData->hCuts->Fill(CUT_PENERGY);
	if (EventEnergy - eclust >= Pars.eGammaMax) CommonData->hCuts->Fill(CUT_PFRACTION);
	CommonData->hCuts->Fill(CUT_NONE);

	EventStuck[EventStuckPos].Energy = EventEnergy;
	EventStuck[EventStuckPos].ClusterEnergy = ClusterEnergy;
	EventStuck[EventStuckPos].Flags = EventFlags;
}

/*	Process SelfTrigger					*/
void dshowMainFrame::ProcessSelfTrig(int mod, int chan, struct hw_rec_struct_self *data)
{
	int i, amp;
	char strl[1024];

//	Extend sign
	for (i = 0; i < data->len - 2; i++) if (data->d[i] & 0x4000) data->d[i] |= 0x8000;
	amp = FindMaxofShort(data->d, data->len - 2);
//	Graph
	if (!CommonData->hWaveForm && rWaveSelf->IsOn() && nWFD->GetIntNumber() == mod && nChan->GetIntNumber() == chan &&
		amp > nWaveThreshold->GetIntNumber()) {
		sprintf(strl, "Self WaveForm for %d.%2.2d", mod, chan);
		CommonData->hWaveForm = new TH1D("hWave", strl, data->len - 2, 0, NSPERCHAN * (data->len - 2));
		for (i = 0; i < data->len - 2; i++) CommonData->hWaveForm->SetBinContent(i + 1, (double) data->d[i]);
	}
//	WFD->MAX(Chan) (self)
	CommonData->hAmpS[mod-1]->Fill((double)chan, (double)amp);
}

/*	Read one record.						*/
/*	Return 1 if OK, negative on error, 0 on the EOF.		*/
int GetFileRecord(char *buf, int buf_size, FILE *f)
{
	int irc;
	int len;
//		Read length
	irc = fread(buf, sizeof(int), 1, f);
	if (irc <= 0) return irc;	// Error or _normal_ EOF

	len = *((int *)buf);
	if (len < sizeof(int) || len > buf_size) return -2;	// strange length

//		Read the rest
	irc = fread(&buf[sizeof(int)], len - sizeof(int), 1, f);
	return irc;	// Error and strange EOF and normal read
}

/*	Read one record	from TCP stream.				*/
/*	Return 1 if OK, negative on error.				*/
/*	Watch for iStop. Return 0 on iStop.				*/
int GetTCPRecord(char *buf, int buf_size, int fd, int *iStop)
{
	int Cnt;
	int irc;
	int len;
	struct timeval tm;
//		Read length
	Cnt = 0;
	while (!(*iStop) && Cnt < sizeof(int)) {
		irc = read(fd, &buf[Cnt], sizeof(int) - Cnt);
		if (irc < 0) Log("Read error (irc=%d) %m\n", irc);
		if (irc < 0 && (errno == EINTR || errno == EAGAIN)) {
			continue;
		} else if (irc < 0) {
			shutdown(fd, 2);
			return irc;		// Error !!!
		} else if (irc == 0) {
			tm.tv_sec = 0;		// do some sleep and try to get more data
			tm.tv_usec = 100000;
			select(FD_SETSIZE, NULL, NULL, NULL, &tm);
		} else {
			Cnt += irc;
		}
	}

	if (*iStop) return 0;

	len = *((int *)buf);
	if (len < sizeof(int) || len > buf_size) {
		Log("Strange block length (%d) from TCP. Buf_size = %d\n", len, buf_size);
		shutdown(fd, 2);
		return -2;	// strange length
	}

//		Read the rest
	while (!(*iStop) && Cnt < len) {
		irc = read(fd, &buf[Cnt], len - Cnt);
		if (irc < 0) Log("Read error (irc=%d) %m\n", irc);
		if (irc < 0 && (errno == EINTR || errno == EAGAIN)) {
			continue;
		} else if (irc < 0) {
			shutdown(fd, 2);
			return irc;
		} else if (irc == 0) {
			tm.tv_sec = 0;		// do some sleep and try to get more data
			tm.tv_usec = 100000;
			select(FD_SETSIZE, NULL, NULL, NULL, &tm);			
		} else {
			Cnt += irc;
		}
	}
	if (*iStop) return 0;
	return 1;
}
/*	Open data file either directly or via zcat etc, depending on the extension	*/
/*	Return some large length (12G) on compressed files				*/
FILE* OpenDataFile(const char *fname, off_t *size) 
{
	char cmd[1024];
	FILE *f;

	*size = 0x300000000LL;
	if (strstr(fname, ".bz2")) {
		sprintf(cmd, "bzcat %s", fname);
		f = popen(cmd, "r");
	} else if (strstr(fname, ".gz")) {
		sprintf(cmd, "zcat %s", fname);
		f = popen(cmd, "r");
	} else if (strstr(fname, ".xz")) {
		sprintf(cmd, "xzcat %s", fname);
		f = popen(cmd, "r");
	} else {
		f = fopen(fname, "rb");
		if (f) {
			fseek(f, 0, SEEK_END);
			*size = ftello(f);
			fseek(f, 0, SEEK_SET);
		}
	}
	return f;
}

/*	Open TCP connection to dsink							*/
int OpenTCP(const char *hname, int port) 
{
	struct hostent *host;
	struct sockaddr_in hostname;
	int fd, flags, irc;
	
	host = gethostbyname(hname);
	if (!host) return -errno;
	
	hostname.sin_family = AF_INET;
	hostname.sin_port = htons(port);
	hostname.sin_addr = *(struct in_addr *) host->h_addr;

	fd = socket(PF_INET, SOCK_STREAM, 0);
	if (fd < 0) return -errno;
	
	if (connect(fd, (struct sockaddr *)&hostname, sizeof(hostname))) {
		irc = -errno;
		close(fd);
		return irc;
	}
	return fd;
}

/*	Data analysis thread				*/
void *DataThreadFunction(void *ptr)
{
	struct common_data_struct *CommonData;
	dshowMainFrame *Main;
	char *buf;
	struct rec_header_struct *head;
	int irc;
	int mod, chan;
	FILE *fIn;
	int fdIn;
	off_t fsize;
	off_t rsize;
	int iNum;
	int Cnt;
	TObject *f;
	int FileCnt;
	int nFiles;
	char str[256];
	time_t lastConnect;
	
	Main = (dshowMainFrame *)ptr;
	CommonData = Main->CommonData;
	CommonData->iError = 0;
	
	buf = (char *)malloc(BSIZE);
	if (!buf) {
		CommonData->iError = 10;
		goto Ret;
	}
	head = (struct rec_header_struct *) buf;
	fIn = NULL;
	fdIn = 0;
	
	if (Main->Follow->IsDown()) {
		fdIn = OpenTCP(Main->Pars.HostName, Main->Pars.HostPort);
		lastConnect = time(NULL);
		nFiles = 0;
		fsize = 0;
		snprintf(str, sizeof(str), "Online(%s:%d)", Main->Pars.HostName, Main->Pars.HostPort);
	} else {
		f = Main->PlayFile->fFileNamesList->First();
		fIn = OpenDataFile(f->GetName(), &fsize);
		nFiles = Main->PlayFile->fFileNamesList->GetEntries();
		snprintf(str, sizeof(str), "%s (%d/%d)", f->GetName(), FileCnt + 1, nFiles);
	}
	if (!fIn && fdIn <= 0) {
		CommonData->iError = 15;
		if (Main->Follow->IsDown()) Main->Follow->SetState(kButtonUp);
		goto Ret;
	}
	Main->fStatusBar->SetText(str, 5);
	iNum = Main->nPlayBlocks->GetIntNumber();
	FileCnt = 0;
	Main->PlayProgress->SetPosition(0);
	Main->FileProgress->SetPosition(0);
	rsize = 0;
	Cnt = 0;
	
	while (!CommonData->iStop) {
		irc = (Main->Follow->IsDown()) ? GetTCPRecord(buf, BSIZE, fdIn, &CommonData->iStop) : GetFileRecord(buf, BSIZE, fIn);
		if (irc < 0) {
			if (Main->Follow->IsDown()) {
				if (time(NULL) - lastConnect > 30) {	// Try to reconnect
					sleep(5);
					fdIn = OpenTCP(Main->Pars.HostName, Main->Pars.HostPort);
					if (fdIn <= 0) {
						CommonData->iError = 15;
						Main->Follow->SetState(kButtonUp);
						break;
					}
					lastConnect = time(NULL);
					continue;
				} else {
					fdIn = irc;
					Main->Follow->SetState(kButtonUp);
				}
			}
			CommonData->iError = -irc;
			break;
		} else if (CommonData->iStop) {
			break;
		} else if (irc == 0) {
			//	check if we have file after
			FileCnt++;
			Main->PlayProgress->SetPosition(0);
			Main->FileProgress->SetPosition(FileCnt * 100.0 / nFiles);
			if (Main->PlayFile->fFileNamesList->After(f)) {
				f = Main->PlayFile->fFileNamesList->After(f);
				snprintf(str, sizeof(str), "%s (%d/%d)", f->GetName(), FileCnt + 1, nFiles);
				Main->fStatusBar->SetText(str, 5);
				fclose(fIn);
				fIn = OpenDataFile(f->GetName(), &fsize);
				if (!fIn) {
					CommonData->iError = 25;
					break;
				}
				rsize = 0;
				Cnt = 0;
				continue;
			} else {
				break;
			}
		}
		rsize += head->len;
		
		TThread::Lock();
		if (head->len >= sizeof(struct rec_header_struct)) {
			CommonData->BlockCnt++;
			if (head->type & REC_EVENT) {
				Main->ProcessEvent(buf);
				CommonData->EventCnt++;
			} else if ((head->type & 0xFF000000) == REC_SELFTRIG) {
				mod = head->type & 0xFFFF;
				chan = (head->type >> 16) & 0xFF;
				if (chan >=64 || mod > MAXWFD) {
					Log("Channel (%d) or module (%d) out of range for selftrigger.\n", chan, mod);
					CommonData->ErrorCnt++;					
				} else {
					Main->ProcessSelfTrig(mod, chan, (struct hw_rec_struct_self *)(buf + sizeof(struct rec_header_struct)));
					CommonData->SelfTrigCnt++;
				}
			} else {
				Log("Unknown block received: type=0x%X, len=%d.\n", head->type, head->len);
				CommonData->ErrorCnt++;	
			}
		} else {
			Log("The block length (%d) is too small.\n", head->len);
			CommonData->ErrorCnt++;
		}
		TThread::UnLock();
		
		Cnt++;
		if ((iNum > 0 && Cnt >= iNum) || (iNum == 0 && Cnt >= 20000)) {
			if (iNum > 0) sleep(1);
			iNum = Main->nPlayBlocks->GetIntNumber();
			if (!Main->Follow->IsDown()) {
				if (fsize) Main->PlayProgress->SetPosition(100.0*rsize/fsize);
				snprintf(str, sizeof(str), "%s (%d/%d)", f->GetName(), FileCnt + 1, nFiles);
				Main->fStatusBar->SetText(str, 5);
			}
			Cnt = 0;
		}
	}
Ret:
	if (fIn) fclose(fIn);
	if (fdIn > 0) close(fdIn);
	Main->fStatusBar->SetText("", 5);
	if (buf) free(buf);
	return NULL;
}

/*	Return 1 if strips are neighbors.		*/
int IsNeighbors(int xy1, int xy2, int z1, int z2)
{
	if (abs(z1-z2) == 1) return 1;
	if (z1 == z2 && abs(xy1 - xy2) <= 1) return 1;
	return 0;
}

int FindMaxofShort(short int *data, int cnt)
{
	int i, M;
	M = -32000;		// nearly minimum 16-bit signed value
	for (i=0; i<cnt; i++) if (data[i] > M) M = data[i];
	return M;
}

float FindHalfTime(short int *data, int cnt, int amp)
{
	int i;
	float r;
	for (i=0; i<cnt; i++) if (data[i] > amp/2) break;
	if (!i || i == cnt) return 0;
	r = amp;
	r = (r/2 - data[i-1]) / (data[i] - data[i-1]);
	return i + r - 1;
}

void Log(const char *msg, ...)
{
	char str[1024];
	time_t t;
	FILE *f;
	va_list ap;

	va_start(ap, msg);
	t = time(NULL);
	strftime(str, sizeof(str),"%F %T ", localtime(&t));
	f = fopen(LOGFILENAME, "at");
	fprintf(f, str);
	vfprintf(f, msg, ap);
	va_end(ap);
	fclose(f);
}

