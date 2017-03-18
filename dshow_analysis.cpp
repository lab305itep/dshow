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
#include "recformat.h"
#include "dshow_global.h"

/*	Process event data - get hits and parameters		*/
void ExtractHits(char *data)
{
	struct hw_rec_struct *rec;
	struct rec_header_struct *head;
	int i, iptr, len;
	int amp;
	float t;
	char strl[1024];
	int mod, chan;

/*	Clear parameters	*/
	Run.Event->gTime = -1;
	Run.Event->NHits = 0;
	Run.Event->Flags = 0;
	Run.Event->EventCnt = Run.EventCnt;
/*		Get records	*/
	head = (struct rec_header_struct *)data;
	for (iptr = sizeof(struct rec_header_struct); iptr < head->len; iptr += len) {		// record by record
		rec = (struct hw_rec_struct *) &data[iptr];
		len = (rec->len + 1) * sizeof(short);

		if (iptr + len > head->len) {
			Log("Unexpected end of event: iptr = %d, len = %d, total len = %d.\n", iptr, len, head->len);
			Run.ErrorCnt++;
			return;
		}
		if (rec->module < 1 || rec->module > MAXWFD) {
			Log("Module %d is out of range.\n", rec->module);
			Run.ErrorCnt++;
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
			if (!Run.hWaveForm && Run.WaveFormType == WAVE_TRIG && Run.WaveFormChan == mod*100 + chan &&	amp > Run.WaveFormThr) {
				sprintf(strl, "Event WaveForm for %d.%2.2d", mod, chan);
				Run.hWaveForm = new TH1D("hWave", strl, rec->len - 2, 0, NSPERCHAN * (rec->len - 2));
				Run.hWaveForm->SetStats(0);
				for (i = 0; i < rec->len - 2; i++) Run.hWaveForm->SetBinContent(i + 1, (double) rec->d[i+1]);
				Run.thisWaveFormCnt = Run.EventCnt;
			}
//			Fill histogramms
			Run.hAmpE[mod-1]->Fill((double)chan, (double)amp);
			if (amp >= MINAMP) Run.hTimeA[mod-1]->Fill((double)chan, t);
			if (amp > Conf.TimeBThreshold) Run.hTimeB[mod-1]->Fill((double)chan, t);
//			Add hit to event structure
			if (Map[mod-1][chan].type >= 0 && amp >= MINAMP) {
				if (Run.Event->NHits >= MAXHITS) {
					Log("Too many hits %d > %d\n", Run.Event->NHits, MAXHITS);
				} else {
					Run.Event->hits[Run.Event->NHits].energy = amp * Map[mod-1][chan].Coef;
					Run.Event->hits[Run.Event->NHits].time = t - Map[mod-1][chan].T0;
					Run.Event->hits[Run.Event->NHits].mod = mod;
					Run.Event->hits[Run.Event->NHits].type = Map[mod-1][chan].type;
					Run.Event->hits[Run.Event->NHits].xy = Map[mod-1][chan].xy;
					Run.Event->hits[Run.Event->NHits].z = Map[mod-1][chan].z;
					Run.Event->hits[Run.Event->NHits].chan = chan;
					Run.Event->hits[Run.Event->NHits].flag = 0;
				}
				Run.Event->NHits++;
			}
			break;
		case TYPE_RAW:
			amp = FindMaxofShort(&rec->d[1], rec->len - 2);
//			Extract Waveform for drawing only
			if (!Run.hWaveForm && Run.WaveFormType == WAVE_TRIG && Run.WaveFormChan == mod*100 + chan && amp > Run.WaveFormThr) {
				sprintf(strl, "Event WaveForm for %d.%2.2d", mod, chan);
				Run.hWaveForm = new TH1D("hWave", strl, rec->len - 2, 0, NSPERCHAN * (rec->len - 2));
				Run.hWaveForm->SetStats(0);
				for (i = 0; i < rec->len - 2; i++) Run.hWaveForm->SetBinContent(i + 1, (double) rec->d[i+1]);
			}
			break;
		case TYPE_SUM:
			for (i = 0; i < rec->len - 2; i++) if (rec->d[i+1] & 0x4000) rec->d[i+1] |= 0x8000;		// do waveform data sign extension
			amp = FindMaxofShort(&rec->d[1], rec->len - 2);
//			Extract Waveform for drawing
			if (!Run.hWaveForm && Run.WaveFormType == WAVE_HIST && Run.WaveFormChan / 100 == mod && amp > Run.WaveFormThr) {
				sprintf(strl, "Hist WaveForm for %d", mod);
				Run.hWaveForm = new TH1D("hWave", strl, rec->len - 2, 0, NSPERCHAN * (rec->len - 2));
				Run.hWaveForm->SetStats(0);
				for (i = 0; i < rec->len - 2; i++) Run.hWaveForm->SetBinContent(i + 1, (double) rec->d[i+1]);
			}
			break;
		case TYPE_TRIG:
			Run.Event->gTime = rec->d[1] + (((long long) rec->d[2]) << 15) + (((long long) rec->d[3]) << 30);
			Run.Event->gTrig = rec->d[4] + (((int) rec->d[5]) << 15);
			break;
		}
	}
	if (Run.Event->gTime < 0) {
		Log("Trigger record (type = 2) was not found.\n");
		Run.ErrorCnt++;
	}
}

void FineTimeAnalysis(void)
{
	int i;
	double wSum;
	double at;
	int nSum;
//		find fine time (energy weighted)
	wSum = 0;
	at = 0;
	nSum = 0;
	for (i = 0; i<Run.Event->NHits; i++) if (Run.Event->hits[i].energy > Conf.FineTimeThreshold && (Run.Event->hits[i].type == TYPE_SIPM || Run.Event->hits[i].type == TYPE_PMT)) {
		at += Run.Event->hits[i].time * Run.Event->hits[i].energy;
		wSum += Run.Event->hits[i].energy;
		nSum++;
	}
	if (nSum) {
		at /= wSum;
		if (nSum > 1) for (i = 0; i<Run.Event->NHits; i++) Run.hTimeC[Run.Event->hits[i].mod - 1]->Fill((double) Run.Event->hits[i].chan, Run.Event->hits[i].time - at);
		for (i = 0; i<Run.Event->NHits; i++) if (fabs(Run.Event->hits[i].time - at) > Conf.SiPmWindow && Run.Event->hits[i].type == TYPE_SIPM) Run.Event->hits[i].flag = -10;	// clean by time
	} else {
		Run.Event->Flags |= FLAG_NOFINETIME;
	}
}

void CleanByPmtConfirmation(void)
{
	int i, j;
	for (i = 0; i<Run.Event->NHits; i++) if (Run.Event->hits[i].energy < Conf.SmallSiPmAmp && Run.Event->hits[i].type == TYPE_SIPM && Run.Event->hits[i].flag >= 0) {
		for (j = 0; j<Run.Event->NHits; j++) if (Run.Event->hits[j].type == TYPE_PMT) {
			if ((Run.Event->hits[i].z & 1) != (Run.Event->hits[j].z & 1)) continue;
			if ((Run.Event->hits[i].z / 20) != (Run.Event->hits[j].z / 5)) continue;
			if ((Run.Event->hits[i].xy / 5) != Run.Event->hits[j].xy) continue;
			break;
		}
		if (j == Run.Event->NHits) Run.Event->hits[i].flag = -20;
	}
}

void CalculateParameters(void)
{
	int i, j, k;
	double sumX, sumY;
	double Amax;
	int iMax;
	int iFound;
//		Zero...	
	sumX = sumY = 0;
	Run.Event->Energy = Run.Event->ClusterEnergy = Run.Event->VetoEnergy = 0;
	Run.Event->VertexN[0] = Run.Event->VertexN[1] = Run.Event->VertexN[2] = 0;
	Run.Event->VertexP[0] = Run.Event->VertexP[1] = Run.Event->VertexP[2] = 0;
	Run.Event->SiPmHits = Run.Event->PmtHits = Run.Event->VetoHits = 0;
//		Sum
	for (i = 0; i<Run.Event->NHits; i++) if (Run.Event->hits[i].flag >= 0) {
		switch(Run.Event->hits[i].type) {
		case TYPE_SIPM:
			Run.Event->Energy += Run.Event->hits[i].energy;
			Run.Event->SiPmHits++;
			Run.Event->VertexN[2] += Run.Event->hits[i].z * THICK;
			if (Run.Event->hits[i].z & 1) {
				sumX += 1;
				Run.Event->VertexN[0] += Run.Event->hits[i].xy * WIDTH;
			} else {
				sumY += 1;
				Run.Event->VertexN[1] += Run.Event->hits[i].xy * WIDTH;
			}
			break;
		case TYPE_PMT:
			Run.Event->Energy += Run.Event->hits[i].energy;
			Run.Event->PmtHits++;
			break;
		case TYPE_VETO:
			Run.Event->VetoEnergy += Run.Event->hits[i].energy;
			Run.Event->VetoHits++;
		}
	}
//		Calculate
	Run.Event->Energy /= 2;	// SiPm + Pmt
	if (sumX) {
		Run.Event->VertexN[0] /= sumX;
	} else {
		Run.Event->VertexN[0] = -1;
	}
	if (sumY) {
		Run.Event->VertexN[1] /= sumY;
	} else {
		Run.Event->VertexN[1] = -1;
	}
	if (sumX + sumY) {
		Run.Event->VertexN[2] /= sumX + sumY;
	} else {
		Run.Event->VertexN[2] = -1;
	}
//		Cluster
	Amax = 0;
	for (i = 0; i<Run.Event->NHits; i++) if (Run.Event->hits[i].flag >= 0 && Run.Event->hits[i].type == TYPE_SIPM && Run.Event->hits[i].energy > Amax) {
		Amax = Run.Event->hits[i].energy;
		iMax = i;
	}
	if (!Amax) {
		Run.Event->VertexP[0] = Run.Event->VertexP[1] = Run.Event->VertexP[2] = -1;
		return;
	}
	Run.Event->hits[iMax].flag = 10;

	for (k=0; k<MAXCLUSTITER; k++) {
		iFound = 0;
		for (i = 0; i<Run.Event->NHits; i++) if (Run.Event->hits[i].flag >= 10)	for (j = 0; j<Run.Event->NHits; j++) 
			if (Run.Event->hits[j].flag >= 0 && Run.Event->hits[j].flag < 10 && Run.Event->hits[j].type == TYPE_SIPM &&
			IsNeighbors(Run.Event->hits[i].xy, Run.Event->hits[j].xy, Run.Event->hits[i].z, Run.Event->hits[j].z)) {
			Run.Event->hits[j].flag = 20;
			iFound++;
		}
		if(!iFound) break;
	}

	sumX = sumY = 0;
	for (i = 0; i<Run.Event->NHits; i++) if (Run.Event->hits[i].flag >= 10) {
		Run.Event->ClusterEnergy += Run.Event->hits[i].energy;
		Run.Event->VertexP[2] += Run.Event->hits[i].energy * Run.Event->hits[i].z * THICK;
		if (Run.Event->hits[i].z & 1) {
			sumX += Run.Event->hits[i].energy;
			Run.Event->VertexP[0] += Run.Event->hits[i].energy * Run.Event->hits[i].xy * WIDTH;
		} else {
			sumY += Run.Event->hits[i].energy;
			Run.Event->VertexP[1] += Run.Event->hits[i].energy * Run.Event->hits[i].xy * WIDTH;
		}
	}
	if (sumX) {
		Run.Event->VertexP[0] /= sumX;
	} else {
		Run.Event->VertexP[0] = -1;
	}
	if (sumY) {
		Run.Event->VertexP[1] /= sumY;
	} else {
		Run.Event->VertexP[1] = -1;
	}
	if (sumX + sumY) {
		Run.Event->VertexP[2] /= sumX + sumY;
	} else {
		Run.Event->VertexP[2] = -1;
	}
}

void CalculateTags(void)
{
//	Veto ?
	if (Run.Event->VetoEnergy > Conf.VetoMin || Run.Event->VetoHits > 1 || Run.Event->Energy > Conf.DVetoMin) Run.Event->Flags |= FLAG_VETO;
//	Positron ?
	if (Run.Event->ClusterEnergy > Conf.PositronMin && Run.Event->ClusterEnergy < Conf.PositronMax && Run.Event->Energy - Run.Event->ClusterEnergy < Conf.MaxOffClusterEnergy ) Run.Event->Flags |= FLAG_POSITRON;
//	Neutron ?
	if (Run.Event->Energy > Conf.NeutronMin && Run.Event->Energy < Conf.NeutronMax) Run.Event->Flags |= FLAG_NEUTRON;
}

void FillHists(void) 
{
	;	// nothing rithg now - to be add
}

void Event2Draw(void)
{
	int n;
//		Check event type etc
	if (Run.DisplayEvent->NHits) return;	// previous is still waiting for drawing
	if (Run.Event->Energy < Run.DisplayEnergy) return;
	switch (Run.DisplayType) {
	case DISP_NONE:
		if (Run.Event->Flags & (FLAG_VETO | FLAG_POSITRON | FLAG_NEUTRON)) return;
		break;
	case DISP_VETO:
		if (!(Run.Event->Flags & FLAG_VETO)) return;
		break;
	case DISP_POSITRON:
		if (!(Run.Event->Flags & FLAG_POSITRON)) return;
		break;
	case DISP_NEUTRON:
		if (!(Run.Event->Flags & FLAG_NEUTRON)) return;
		break;
	}
//		Copy event for drawing
	memcpy(Run.DisplayEvent, Run.Event, sizeof(struct event_struct) - sizeof(struct hit_struct *));
	n = Run.Event->NHits;
	if (n > MAXHITS) n = MAXHITS;
	memcpy(Run.DisplayEvent->hits, Run.Event->hits, n * sizeof(struct hit_struct));
}

/*	Process an event					*/
void ProcessEvent(char *data)
{
	ExtractHits(data);
	FineTimeAnalysis();
	CleanByPmtConfirmation();
	CalculateParameters();
	CalculateTags();
	FillHists();
	Event2Draw();
}

/*	Process SelfTrigger					*/
void ProcessSelfTrig(int mod, int chan, struct hw_rec_struct_self *data)
{
	int i, amp;
	char strl[1024];

//	Extend sign
	for (i = 0; i < data->len - 2; i++) if (data->d[i] & 0x4000) data->d[i] |= 0x8000;
	amp = FindMaxofShort(data->d, data->len - 2);
//	Graph
	if (!Run.hWaveForm && Run.WaveFormType == WAVE_SELF && Run.WaveFormChan == mod*100 + chan &&	amp > Run.WaveFormThr) {
		sprintf(strl, "Self WaveForm for %d.%2.2d", mod, chan);
		Run.hWaveForm = new TH1D("hWave", strl, data->len - 2, 0, NSPERCHAN * (data->len - 2));
		for (i = 0; i < data->len - 2; i++) Run.hWaveForm->SetBinContent(i + 1, (double) data->d[i]);
	}
//	WFD->MAX(Chan) (self)
	Run.hAmpS[mod-1]->Fill((double)chan, (double)amp);
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
int GetTCPRecord(char *buf, int buf_size, int fd)
{
	int Cnt;
	int irc;
	int len;
	struct timeval tm;
//		Read length
	Cnt = 0;
	while (!Run.iThreadStop && Cnt < sizeof(int)) {
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

	if (Run.iThreadStop) return 0;

	len = *((int *)buf);
	if (len < sizeof(int) || len > buf_size) {
		Log("Strange block length (%d) from TCP. Buf_size = %d\n", len, buf_size);
		shutdown(fd, 2);
		return -2;	// strange length
	}

//		Read the rest
	while (!Run.iThreadStop && Cnt < len) {
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
	if (Run.iThreadStop) return 0;
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

/*	Open TCP connection to dsink			*/
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
	TGFileInfo *PlayFile;
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
	
	Message("");
	PlayFile = (TGFileInfo *)ptr;
	
	buf = (char *)malloc(BSIZE);
	if (!buf) {
		Message("DataThread FATAL: no memory!");
		goto Ret;
	}
	head = (struct rec_header_struct *) buf;
	fIn = NULL;
	fdIn = 0;
	
	if (!PlayFile) {
		fdIn = OpenTCP(Conf.ServerName, Conf.ServerPort);
		lastConnect = time(NULL);
		nFiles = 0;
		fsize = 0;
		Message("Online(%s:%d)", Conf.ServerName, Conf.ServerPort);
	} else {
		f = PlayFile->fFileNamesList->First();
		fIn = OpenDataFile(f->GetName(), &fsize);
		nFiles = PlayFile->fFileNamesList->GetEntries();
		Message("%s (%d/%d)", f->GetName(), FileCnt + 1, nFiles);
	}
	if (!fIn && fdIn <= 0) {
		Message("DataThread start failed!");
		goto Ret;
	}
	FileCnt = 0;
	rsize = 0;
	Cnt = 0;
	FileProgress(0, 0);
	
	while (!Run.iThreadStop) {
		irc = (!PlayFile) ? GetTCPRecord(buf, BSIZE, fdIn) : GetFileRecord(buf, BSIZE, fIn);
		if (irc < 0) {
			if (!PlayFile) {
				if (time(NULL) - lastConnect > 30) {	// Try to reconnect
					sleep(5);
					fdIn = OpenTCP(Conf.ServerName, Conf.ServerPort);
					if (fdIn <= 0) {
						Message("Reconnection to %s:%d failed", Conf.ServerName, Conf.ServerPort); 
						break;
					}
					lastConnect = time(NULL);
					continue;
				} else {
					Message("Too many reconnections to %s:%d", Conf.ServerName, Conf.ServerPort); 
					fdIn = irc;
				}
			}
			break;
		} else if (Run.iThreadStop) {
			break;
		} else if (irc == 0) {
			//	check if we have file after
			FileCnt++;
			FileProgress(0, FileCnt * 100.0 / nFiles);
			if (PlayFile->fFileNamesList->After(f)) {
				f = PlayFile->fFileNamesList->After(f);
				Message("%s (%d/%d)", f->GetName(), FileCnt + 1, nFiles);
				fclose(fIn);
				fIn = OpenDataFile(f->GetName(), &fsize);
				if (!fIn) {
					Message("Can not open the next file: %s", f->GetName());
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
			Run.BlockCnt++;
			if (head->type & REC_EVENT) {
				ProcessEvent(buf);
				Run.EventCnt++;
			} else if ((head->type & 0xFF000000) == REC_SELFTRIG) {
				mod = head->type & 0xFFFF;
				chan = (head->type >> 16) & 0xFF;
				if (chan >=64 || mod > MAXWFD) {
					Log("Channel (%d) or module (%d) out of range for selftrigger.\n", chan, mod);
					Run.ErrorCnt++;					
				} else {
					ProcessSelfTrig(mod, chan, (struct hw_rec_struct_self *)(buf + sizeof(struct rec_header_struct)));
					Run.SelfTrigCnt++;
				}
			}	// silently ignore unknown blocks - we have more block types now.
		} else {
			Log("The block length (%d) is too small.\n", head->len);
			Run.ErrorCnt++;
		}
		TThread::UnLock();
		
		Cnt++;
		if ((Run.iPlayBlocks > 0 && Cnt >= Run.iPlayBlocks) || (Run.iPlayBlocks == 0 && Cnt >= 20000)) {
			if (Run.iPlayBlocks > 0) sleep(1);
			if (PlayFile) {
				if (fsize) FileProgress(100.0*rsize/fsize, FileCnt * 100.0 / nFiles);
				Message("%s (%d/%d)", f->GetName(), FileCnt + 1, nFiles);
			}
			Cnt = 0;
		}
	}
Ret:
	if (fIn) fclose(fIn);
	if (fdIn > 0) close(fdIn);
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

void Message(const char *msg, ...)
{
	va_list ap;

	TThread::Lock();
	va_start(ap, msg);
	vsnprintf(Run.msg, sizeof(Run.msg), msg, ap);
	va_end(ap);
	TThread::UnLock();
}

void FileProgress(double inFile, double inNumber)
{
	TThread::Lock();
	Run.inFile = inFile;
	Run.inNumber = inNumber;
	TThread::UnLock();
}

