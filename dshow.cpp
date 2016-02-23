#include <TApplication.h>
#include <TGClient.h>
#include <TCanvas.h>
#include <TGButton.h>
#include <TGComboBox.h>
#include <TGLabel.h>
#include <TGNumberEntry.h>
#include <TGStatusBar.h>
#include <TGraph.h>
#include <TH2D.h>
#include <TRootEmbeddedCanvas.h>
#include <TStyle.h>
#include <TThread.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "dshow.h"
#include "recformat.h"

/*	Class dshowMainFrame - the main window		*/
/*	Main window Constructor				*/
dshowMainFrame::dshowMainFrame(const TGWindow *p, UInt_t w, UInt_t h) : TGMainFrame(p, w, h) 
{
	char strs[64];
	char strl[1024];
	int i;
	Int_t StatusBarParts[] = {10, 10, 10, 10, 10, 50};	// status bar parts
	const char DrawTypeList[][30] = {"Any  WaveForm", "Self WaveForm", "Event WaveForm", "Raw WaveForm", "Hist WaveForm",
		"Module versus RecType", "RecType", "Modules", "Records in Module",
		"Amplitude vs chan (self)", "Amplitudes (self)", "Channels (self)", "Amplitude for chan (self)",
		"Amplitude vs chan (event)", "Amplitudes (event)", "Channels (event)", "Amplitude for chan (event)"};
	const int DrawTypeNum[] = {10, 11, 12, 13, 14, 20, 21, 22, 23, 30, 31, 32, 33, 40, 41, 42, 43};
	TGLabel *lb;
	
	// Create our data structures and histogramms
	CommonData = (struct common_data_struct *) malloc(sizeof(struct common_data_struct));
	memset(CommonData, 0, sizeof(struct common_data_struct));

	gStyle->SetOptStat(0);
	CommonData->hWFD = new TH2D("hWFD", "Record Types versus modules", 8, 0, 8, MAXWFD, 1, MAXWFD + 1);
	for(i=0; i<MAXWFD; i++) {
		snprintf(strs, sizeof(strs), "hAmpS%2.2d", i+1);
		snprintf(strl, sizeof(strl), "Module %2.2d: amplitude versus channels number (self)", i+1);		
		CommonData->hAmpS[i] = new TH2D(strs, strl, 64, 0, 64, 128, 0, 128);
		snprintf(strs, sizeof(strs), "hAmpE%2.2d", i+1);
		snprintf(strl, sizeof(strl), "Module %2.2d: amplitude versus channels number (event)", i+1);
		CommonData->hAmpE[i] = new TH2D(strs, strl, 64, 0, 64, 248, 128, 4096);
	}
	
	// Creates widgets of the example
	fEcanvas = new TRootEmbeddedCanvas ("Ecanvas", this, w, h);
   	AddFrame(fEcanvas, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 10, 10, 10, 1));
   	TGHorizontalFrame *hframe=new TGHorizontalFrame(this, w, 40);

   	TGComboBox *lbox = new TGComboBox(hframe);
	for(i=0; i<sizeof(DrawTypeList)/sizeof(DrawTypeList[0]); i++) lbox->AddEntry(DrawTypeList[i], DrawTypeNum[i]);
	lbox->Connect("Selected(Int_t)", "dshowMainFrame", this, "DoListBox(Int_t)");
	lbox->Resize(150, 30);
	lbox->Select(20);
   	hframe->AddFrame(lbox, new TGLayoutHints(kLHintsLeft | kLHintsCenterY, 5, 5, 3, 4));

	lb = new TGLabel(hframe, "WFD:");
   	hframe->AddFrame(lb, new TGLayoutHints(kLHintsLeft | kLHintsCenterY, 5, 5, 3, 4));		
	nWFD = new TGNumberEntry(hframe, 1, 2, -1, TGNumberFormat::kNESInteger, TGNumberFormat::kNEANonNegative, 
		TGNumberFormat::kNELLimitMinMax, 1, MAXWFD);
	nWFD->Connect("ValueSet(Long_t)", "dshowMainFrame", this, "DoNWFD()");
	CommonData->DrawMod = 1;
	nWFD->SetIntNumber(CommonData->DrawMod);
   	hframe->AddFrame(nWFD, new TGLayoutHints(kLHintsLeft | kLHintsCenterY, 5, 5, 3, 4));	

	lb = new TGLabel(hframe, "Channel:");
   	hframe->AddFrame(lb, new TGLayoutHints(kLHintsLeft | kLHintsCenterY, 5, 5, 3, 4));		
	nChan = new TGNumberEntry(hframe, 0, 2, -1, TGNumberFormat::kNESInteger, TGNumberFormat::kNEANonNegative, 
		TGNumberFormat::kNELLimitMinMax, 0, 63);
	nChan->Connect("ValueSet(Long_t)", "dshowMainFrame", this, "DoNChan()");
	nChan->SetIntNumber(0);
   	hframe->AddFrame(nChan, new TGLayoutHints(kLHintsLeft  | kLHintsCenterY, 5, 5, 3, 4));	
 
	TGFrame *spacer1 = new TGFrame(hframe, w, 1, kFitWidth | kFitHeight);
	hframe->AddFrame(spacer1, new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 5, 5, 3, 4));

   	TGTextButton *exit = new TGTextButton(hframe,"&Exit ");
	exit->Connect("Clicked()", "dshowMainFrame", this, "SendCloseMessage()");
   	hframe->AddFrame(exit, new TGLayoutHints(kLHintsRight | kLHintsCenterY, 5, 5, 3, 4));

   	AddFrame(hframe, new TGLayoutHints(kLHintsCenterX, 2, 2, 2, 2));

	fStatusBar = new TGStatusBar(this, 50, 10, kHorizontalFrame);
	fStatusBar->SetParts(StatusBarParts, sizeof(StatusBarParts) / sizeof(Int_t));
	AddFrame(fStatusBar, new TGLayoutHints(kLHintsBottom | kLHintsLeft | kLHintsExpandX, 0, 0, 2, 0));

   	// Sets window name and shows the main frame
   	SetWindowName("DANSS Show");
   	MapSubwindows();
   	Resize(GetDefaultSize());
   	MapWindow();

	// Start thread and Timer
	DataThread = new TThread("DataThread", DataThreadFunction, (void *) this);
	DataThread->Run();
	OneSecond = new TTimer(1000);
	OneSecond->Connect("Timeout()", "dshowMainFrame", this, "DoDraw()");
	OneSecond->Start();
}

/*	Main Window destructor				*/
dshowMainFrame::~dshowMainFrame(void) 
{
	CommonData->iStop = 1;
	delete OneSecond;
	DataThread->Join();
	Cleanup();
}

/*	When main window is closed			*/
void dshowMainFrame::CloseWindow(void)
{
	gApplication->Terminate(0);
}

/*	Process setting channel number			*/
void dshowMainFrame::DoNChan(void)
{
	CommonData->DrawChan = nChan->GetIntNumber();
}

/*	Process setting module number			*/
void dshowMainFrame::DoNWFD(void)
{
	CommonData->DrawMod = nWFD->GetIntNumber();
}

/*	Main drawing function. Called by timer		*/
void dshowMainFrame::DoDraw(void)
{
	char str[1024];

	TThread::Lock();
   	TCanvas *fCanvas = fEcanvas->GetCanvas();
   	fCanvas->cd();
	snprintf(str, sizeof(str), "Thread status: %d", CommonData->iError);
	fStatusBar->SetText(str ,0);
	snprintf(str, sizeof(str), "Blocks: %d", CommonData->BlockCnt);
	fStatusBar->SetText(str, 1);
	snprintf(str, sizeof(str), "Events: %d", CommonData->EventCnt);
	fStatusBar->SetText(str, 2);
	snprintf(str, sizeof(str), "SelfTriggers: %d", CommonData->SelfTrigCnt);
	fStatusBar->SetText(str, 3);
	snprintf(str, sizeof(str), "Errors: %d", CommonData->ErrorCnt);
	fStatusBar->SetText(str, 4);
	snprintf(str, sizeof(str), "Type= %d Mod = %d Chan = %d", CommonData->DrawType, CommonData->DrawMod, CommonData->DrawChan); 
	fStatusBar->SetText(str, 5);
	switch(CommonData->DrawType) {
	case 10:	// any wave form
	case 11:	// self trigger wave form
	case 12:	// event wave form
	case 13:	// raw wave form
	case 14:	// history waveform
		if (CommonData->gWaveForm && CommonData->WaveFormReady) {
			CommonData->gWaveForm->Draw("A*");
			CommonData->WaveFormReady = 0;
		}
		break;
	case 20:	// Modules versus record types 2-dim plot
		CommonData->hWFD->Draw("color");
		break;
	case 21:	// Types (any modules) 1-dim plot (projection)
		CommonData->hWFD->ProjectionX()->Draw();
		break;
	case 22:	// Module (any type) 1-dim plot (projection)
		CommonData->hWFD->ProjectionY()->Draw();
		break;
	case 23:	// Types in a given module 1-dim plot (slice)
		CommonData->hWFD->ProjectionX("_px", CommonData->DrawMod, CommonData->DrawMod)->Draw();
		break;
	case 30:	// Noise Amplitude versus channel 2-dim plot
		CommonData->hAmpS[CommonData->DrawMod-1]->Draw("color");
		break;
	case 31:	// Noise Amplitudes for all channels 1-dim plot (projection)
		CommonData->hAmpS[CommonData->DrawMod-1]->ProjectionY()->Draw();
		break;
	case 32:	// Noise Channel distribution 1-dim plot (projection)
		CommonData->hAmpS[CommonData->DrawMod-1]->ProjectionX()->Draw();
		break;
	case 33:	// Noise Amplitudes for Channel 1-dim plot (slice)
		CommonData->hAmpS[CommonData->DrawMod-1]->ProjectionY("_py", CommonData->DrawChan+1, CommonData->DrawChan+1)->Draw();
		break;
	case 40:	// Event Amplitude versus channel 2-dim plot
		CommonData->hAmpE[CommonData->DrawMod-1]->Draw("color");
		break;
	case 41:	// Event Amplitudes for all channels 1-dim plot (projection)
		CommonData->hAmpE[CommonData->DrawMod-1]->ProjectionY()->Draw();
		break;
	case 42:	// Event Channel distribution 1-dim plot (projection)
		CommonData->hAmpE[CommonData->DrawMod-1]->ProjectionX()->Draw();
		break;
	case 43:	// Event Amplitudes for Channel 1-dim plot (slice)
		CommonData->hAmpE[CommonData->DrawMod-1]->ProjectionY("_py", CommonData->DrawChan+1, CommonData->DrawChan+1)->Draw();
		break;
	}
   	fCanvas->Update();
	TThread::UnLock();
}

/*	Process an event					*/
void dshowMainFrame::ProcessEvent(char *data)
{
	struct hw_rec_struct *rec;
	struct rec_header_struct *head;
	int i, iptr, len;
	double *x;
	double *y;

	head = (struct rec_header_struct *)data;
	for (iptr = sizeof(struct rec_header_struct); iptr < head->len; iptr += len) {
		rec = (struct hw_rec_struct *) &data[iptr];
		len = (rec->len + 1) * sizeof(short);
		if (iptr + len > head->len) {
			CommonData->ErrorCnt++;
			return;
		}
		if (rec->module < 1 || rec->module > MAXWFD) {
			CommonData->ErrorCnt++;
			continue;
		}
//		WFD(TYPE)
		CommonData->hWFD->Fill((double)rec->type, (double)rec->module);
		switch(rec->type) {
		case TYPE_MASTER:
			for (i = 0; i < rec->len - 2; i++) if (rec->d[i+1] & 0x4000) rec->d[i+1] |= 0x8000;
			if (!CommonData->WaveFormReady && (CommonData->DrawType == 10 || CommonData->DrawType == 12) &&
				CommonData->DrawMod == rec->module && CommonData->DrawChan == rec->chan) {
				x = (double *) malloc((rec->len - 2) * sizeof(double));
				y = (double *) malloc((rec->len - 2) * sizeof(double));
				if (x && y) {
					for (i = 0; i < rec->len - 2; i++) {
						x[i] = NSPERCHAN * i;
						y[i] = rec->d[i+1];
					}
					delete CommonData->gWaveForm;
					CommonData->gWaveForm = new TGraph(rec->len - 2, x, y);
					CommonData->WaveFormReady = 1;
				}
				if (x) free(x);
				if (y) free(y);
			}
//			WFD->MAX(Chan) (self)
			CommonData->hAmpE[rec->module-1]->Fill((double)rec->chan, (double)FindMaxofShort(&rec->d[1], rec->len - 2));
			break;
		case TYPE_RAW:
			if (!CommonData->WaveFormReady && (CommonData->DrawType == 10 || CommonData->DrawType == 13) &&
				CommonData->DrawMod == rec->module && CommonData->DrawChan == rec->chan) {
				x = (double *) malloc((rec->len - 2) * sizeof(double));
				y = (double *) malloc((rec->len - 2) * sizeof(double));
				if (x && y) {
					for (i = 0; i < rec->len - 2; i++) {
						x[i] = NSPERCHAN * i;
						y[i] = rec->d[i+1];
					}
					delete CommonData->gWaveForm;
					CommonData->gWaveForm = new TGraph(rec->len - 2, x, y);
					CommonData->WaveFormReady = 1;
				}
				if (x) free(x);
				if (y) free(y);
			}
			break;
		case TYPE_SUM:
			if (!CommonData->WaveFormReady && (CommonData->DrawType == 10 || CommonData->DrawType == 14) &&
				CommonData->DrawMod == rec->module && CommonData->DrawChan == rec->chan) {
				for (i = 0; i < rec->len - 2; i++) if (rec->d[i+1] & 0x4000) rec->d[i+1] |= 0x8000;
				x = (double *) malloc((rec->len - 2) * sizeof(double));
				y = (double *) malloc((rec->len - 2) * sizeof(double));
				if (x && y) {
					for (i = 0; i < rec->len - 2; i++) {
						x[i] = NSPERCHAN * i;
						y[i] = rec->d[i+1];
					}
					delete CommonData->gWaveForm;
					CommonData->gWaveForm = new TGraph(rec->len - 2, x, y);
					CommonData->WaveFormReady = 1;
				}
				if (x) free(x);
				if (y) free(y);
			}
			break;
		}
	}
}

/*	Process SelfTrigger					*/
void dshowMainFrame::ProcessSelfTrig(int mod, int chan, struct hw_rec_struct_self *data)
{
	double *x;
	double *y;
	int i;
//	Extend sign
	for (i = 0; i < data->len - 2; i++) if (data->d[i] & 0x4000) data->d[i] |= 0x8000;
//	Graph
	if (!CommonData->WaveFormReady && (CommonData->DrawType == 10 || CommonData->DrawType == 11) &&
		CommonData->DrawMod == mod && CommonData->DrawChan == chan) {
		x = (double *) malloc((data->len - 2) * sizeof(double));
		y = (double *) malloc((data->len - 2) * sizeof(double));
		if (x && y) {
			for (i = 0; i < data->len - 2; i++) {
				x[i] = NSPERCHAN * i;
				y[i] = data->d[i];
			}
			delete CommonData->gWaveForm;
			CommonData->gWaveForm = new TGraph(data->len - 2, x, y);
			CommonData->WaveFormReady = 1;
		}
		if (x) free(x);
		if (y) free(y);
	}
//	WFD(TYPE)
	CommonData->hWFD->Fill((double)TYPE_SELF, (double)mod);
//	WFD->MAX(Chan) (self)
	CommonData->hAmpS[mod-1]->Fill((double)chan, (double)FindMaxofShort(data->d, data->len - 2));
}

/*	General functions				*/
/*	Create receiving udp socket			*/
int bind_my_socket(void)
{
	int fd;
	struct sockaddr_in name;
	
	fd = socket(PF_INET, SOCK_DGRAM, 0);
	if (fd < 0) return fd;
	name.sin_family = AF_INET;
	name.sin_port = htons(UDPPORT);
	name.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(fd, (struct sockaddr *)&name, sizeof(name)) < 0) {
		close(fd);
		return -1;
	}
	return fd;
}

/*	Data analysis thread				*/
void *DataThreadFunction(void *ptr)
{
	struct common_data_struct *CommonData;
	dshowMainFrame *Main;
	int udpSocket;
	char *buf;
	struct rec_header_struct *head;
	int irc;
	int mod, chan;
	fd_set set;
	struct timeval tm;
	
	Main = (dshowMainFrame *)ptr;
	CommonData = Main->CommonData;
	
	buf = (char *)malloc(BSIZE);
	if (!buf) {
		CommonData->iError = 10;
		goto Ret;
	}
	head = (struct rec_header_struct *) buf;

	udpSocket = bind_my_socket();
	if (udpSocket < 0) {
		CommonData->iError = 20;
		goto Ret;
	}	

	while (!CommonData->iStop) {
		tm.tv_sec = 0;		// 0.1 s
		tm.tv_usec = 100000;
		FD_ZERO(&set);
		FD_SET(udpSocket, &set);
		irc = select(FD_SETSIZE, &set, NULL, NULL, &tm);
		if (irc <= 0 || !FD_ISSET(udpSocket, &set)) continue;
		irc = read(udpSocket, buf, BSIZE);
		TThread::Lock();
		if (irc >= sizeof(struct rec_header_struct) && (head->len == irc)) {
			CommonData->BlockCnt++;
			if (head->type & REC_EVENT) {
				Main->ProcessEvent(buf);
				CommonData->EventCnt++;
			} else if ((head->type & 0xFF000000) == REC_SELFTRIG) {
				mod = head->type & 0xFFFF;
				chan = (head->type >> 16) & 0xFF;
				if (chan >=64 || mod > MAXWFD) {
					CommonData->ErrorCnt++;					
				} else {
					Main->ProcessSelfTrig(mod, chan, (struct hw_rec_struct_self *)(buf + sizeof(struct rec_header_struct)));
					CommonData->SelfTrigCnt++;
				}
			} else {
				CommonData->ErrorCnt++;	
			}
		} else {
			CommonData->ErrorCnt++;
		}
		TThread::UnLock();
	}
Ret:
	close(udpSocket);
	if (buf) free(buf);
	return NULL;
}

int FindMaxofShort(short int *data, int cnt)
{
	int i, M;
	M = -32000;		// nearly minimum 16-bit signed value
	for (i=0; i<cnt; i++) if (data[i] > M) M = data[i];
	return M;
}

/*	the main						*/
int main(int argc, char **argv) {
   	TApplication theApp("DANSS_SHOW", &argc, argv);
   	new dshowMainFrame(gClient->GetRoot(), 2000, 1000);
   	theApp.Run();
   	return 0;
}

