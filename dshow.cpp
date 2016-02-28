#include <TApplication.h>
#include <TGClient.h>
#include <TCanvas.h>
#include <TColor.h>
#include <TBox.h>
#include <TGButton.h>
#include <TGButtonGroup.h>
#include <TGComboBox.h>
#include <TGFileDialog.h>
#include <TGLabel.h>
#include <TGNumberEntry.h>
#include <TGStatusBar.h>
#include <TGTab.h>
#include <TGraph.h>
#include <TH2D.h>
#include <TRootEmbeddedCanvas.h>
#include <TString.h>
#include <TStyle.h>
#include <TSystem.h>
#include <TThread.h>
#include <arpa/inet.h>
#include <libconfig.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "dshow.h"
#include "recformat.h"

const char *DataFileTypes[] = {"All files", "*", "Data files", "*.dat*", 0, 0};

/*	Class dshowMainFrame - the main window		*/
/*	Main window Constructor				*/
dshowMainFrame::dshowMainFrame(const TGWindow *p, UInt_t w, UInt_t h) : TGMainFrame(p, w, h) 
{
	char strs[64];
	char strl[1024];
	Int_t StatusBarParts[] = {10, 10, 10, 10, 10, 50};	// status bar parts
	Double_t r[]    = {0., 0.0, 1.0, 1.0, 1.0};
   	Double_t g[]    = {0., 0.0, 0.0, 1.0, 1.0};
   	Double_t b[]    = {0., 1.0, 0.0, 0.0, 1.0};
   	Double_t stop[] = {0., .25, .50, .75, 1.0};
	int i;
	TGLabel *lb;
	
	// Create our data structures and histogramms
	CommonData = (struct common_data_struct *) malloc(sizeof(struct common_data_struct));
	memset(CommonData, 0, sizeof(struct common_data_struct));

	gStyle->SetOptStat(0);
	for(i=0; i<MAXWFD; i++) {
		snprintf(strs, sizeof(strs), "hAmpS%2.2d", i+1);
		snprintf(strl, sizeof(strl), "Module %2.2d: amplitude versus channels number (self)", i+1);		
		CommonData->hAmpS[i] = new TH2D(strs, strl, 64, 0, 64, 128, 0, 128);
		snprintf(strs, sizeof(strs), "hAmpE%2.2d", i+1);
		snprintf(strl, sizeof(strl), "Module %2.2d: amplitude versus channels number (event)", i+1);
		CommonData->hAmpE[i] = new TH2D(strs, strl, 64, 0, 64, 248, 128, 4096);
		snprintf(strs, sizeof(strs), "hTimeA%2.2d", i+1);
		snprintf(strl, sizeof(strl), "Module %2.2d: time versus channels number, all hits", i+1);
		CommonData->hTimeA[i] = new TH2D(strs, strl, 64, 0, 64, 100, 0, 800);
		snprintf(strs, sizeof(strs), "hTimeB%2.2d", i+1);
		snprintf(strl, sizeof(strl), "Module %2.2d: time versus channels number, amplitude above threshold", i+1);
		CommonData->hTimeB[i] = new TH2D(strs, strl, 64, 0, 64, 100, 0, 800);
		CommonData->hTimeB[i]->SetLineColor(kBlue);
	}

	PlayFile = new TGFileInfo();
	PlayFile->fFileTypes = DataFileTypes;
	PlayFile->fIniDir = strdup(".");
	PlayFile->SetMultipleSelection(1);
	myDir = getcwd(NULL, 0);

   	PaletteStart = TColor::CreateGradientColorTable(5, stop, r, g, b, 100);

	//
	//	Create Main window structure
	//
   	TGHorizontalFrame *hframe=new TGHorizontalFrame(this);
	
	TGTab *tab = new TGTab(hframe, w-50, h);
	CreateWaveTab(tab);
	CreateSelfTab(tab);
	CreateSpectrumTab(tab);
	CreateTimeTab(tab);
	CreateEventTab(tab);
	hframe->AddFrame(tab, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 5, 5, 3, 4));
	
   	TGVerticalFrame *vframe=new TGVerticalFrame(hframe, 50, h);
	Pause = new TGCheckButton(vframe,"&Pause");
   	vframe->AddFrame(Pause, new TGLayoutHints(kLHintsCenterX | kLHintsTop, 5, 5, 3, 4));

	lb = new TGLabel(vframe, "Refresh, s");
   	vframe->AddFrame(lb, new TGLayoutHints(kLHintsLeft | kLHintsTop, 5, 5, 3, 4));		
	nRefresh = new TGNumberEntry(vframe, 1, 2, -1, TGNumberFormat::kNESInteger, TGNumberFormat::kNEANonNegative, 
		TGNumberFormat::kNELLimitMinMax, 1, 100);
   	vframe->AddFrame(nRefresh, new TGLayoutHints(kLHintsCenterX | kLHintsTop, 5, 5, 3, 4));	

	lb = new TGLabel(vframe, "WFD:");
   	vframe->AddFrame(lb, new TGLayoutHints(kLHintsLeft | kLHintsTop, 5, 5, 3, 4));		
	nWFD = new TGNumberEntry(vframe, 1, 2, -1, TGNumberFormat::kNESInteger, TGNumberFormat::kNEANonNegative, TGNumberFormat::kNELLimitMinMax, 1, MAXWFD);
	nWFD->SetIntNumber(1);
   	vframe->AddFrame(nWFD, new TGLayoutHints(kLHintsCenterX | kLHintsTop, 5, 5, 3, 4));	

	lb = new TGLabel(vframe, "Channel:");
   	vframe->AddFrame(lb, new TGLayoutHints(kLHintsLeft | kLHintsTop, 5, 5, 3, 4));		
	nChan = new TGNumberEntry(vframe, 0, 2, -1, TGNumberFormat::kNESInteger, TGNumberFormat::kNEANonNegative, TGNumberFormat::kNELLimitMinMax, 0, 63);
	nChan->SetIntNumber(0);
   	vframe->AddFrame(nChan, new TGLayoutHints(kLHintsCenterX  | kLHintsTop, 5, 5, 3, 4));

   	TGTextButton *play = new TGTextButton(vframe,"&Play file");
	play->Connect("Clicked()", "dshowMainFrame", this, "PlayFileDialog()");
   	vframe->AddFrame(play, new TGLayoutHints(kLHintsCenterX | kLHintsTop, 5, 5, 3, 4));

	lb = new TGLabel(vframe, "Blocks/s:");
   	vframe->AddFrame(lb, new TGLayoutHints(kLHintsLeft | kLHintsTop, 5, 5, 3, 4));
	nPlayBlocks = new TGNumberEntry(vframe, 0, 8, -1, TGNumberFormat::kNESInteger, TGNumberFormat::kNEANonNegative);
	nPlayBlocks->SetIntNumber(0);
   	vframe->AddFrame(nPlayBlocks, new TGLayoutHints(kLHintsCenterX  | kLHintsTop, 5, 5, 3, 4));

   	TGTextButton *exit = new TGTextButton(vframe,"&Exit ");
	exit->Connect("Clicked()", "dshowMainFrame", this, "SendCloseMessage()");
   	vframe->AddFrame(exit, new TGLayoutHints(kLHintsCenterX | kLHintsBottom, 5, 5, 3, 4));

	hframe->AddFrame(vframe, new TGLayoutHints(kLHintsRight | kLHintsExpandY, 5, 5, 3, 4));

   	AddFrame(hframe, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 2, 2, 2, 2));

	fStatusBar = new TGStatusBar(this, 50, 10, kHorizontalFrame);
	fStatusBar->SetParts(StatusBarParts, sizeof(StatusBarParts) / sizeof(Int_t));
	AddFrame(fStatusBar, new TGLayoutHints(kLHintsBottom | kLHintsExpandX, 0, 0, 2, 0));

   	// Sets window name and shows the main frame
   	SetWindowName("DANSS Show");
   	MapSubwindows();
   	Resize(GetDefaultSize());
   	MapWindow();

	// Start thread and Timer
	DataThread = new TThread("DataThread", DataThreadFunction, (void *) this);
	DataThread->Run();
	TimerCnt = 0;
	OneSecond = new TTimer(1000);
	OneSecond->Connect("Timeout()", "dshowMainFrame", this, "OnTimer()");
	OneSecond->Start();
}

/*	Main Window destructor				*/
dshowMainFrame::~dshowMainFrame(void) 
{
	CommonData->iStop = 1;
	delete OneSecond;
	DataThread->Join();
	Cleanup();
	if (CommonData->Event) free(CommonData->Event);
}

/*	When main window is closed			*/
void dshowMainFrame::CloseWindow(void)
{
	gApplication->Terminate(0);
}

/*	Create Waveform tab				*/
void dshowMainFrame::CreateWaveTab(TGTab *tab)
{
	TGLabel *lb;
	int i;
	
	TGCompositeFrame *me = tab->AddTab("Scope");

	fWaveCanvas = new TRootEmbeddedCanvas ("ScopeCanvas", me, 1600, 800);
   	me->AddFrame(fWaveCanvas, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 10, 10, 10, 1));

   	TGHorizontalFrame *hframe=new TGHorizontalFrame(me);
	TGButtonGroup *bg = new TGButtonGroup(hframe, "WaveForm", kHorizontalFrame);
	rWaveSelf = new TGRadioButton(bg, "&Self   ");
	rWaveTrig = new TGRadioButton(bg, "&Event  ");
	rWaveHist = new TGRadioButton(bg, "&History");
	rWaveSelf->SetState(kButtonDown);
   	hframe->AddFrame(bg, new TGLayoutHints(kLHintsLeft | kLHintsCenterY, 5, 5, 3, 4));

	lb = new TGLabel(hframe, "Threshold:");
   	hframe->AddFrame(lb, new TGLayoutHints(kLHintsLeft | kLHintsCenterY, 5, 5, 3, 4));		
	nWaveThreshold = new TGNumberEntry(hframe, 0, 5, -1, TGNumberFormat::kNESInteger, TGNumberFormat::kNEANonNegative);
	nWaveThreshold->SetIntNumber(100);
   	hframe->AddFrame(nWaveThreshold, new TGLayoutHints(kLHintsLeft  | kLHintsCenterY, 5, 5, 3, 4));
 
    	me->AddFrame(hframe, new TGLayoutHints(kLHintsBottom | kLHintsExpandX, 2, 2, 2, 2));
}

/*	Create Self tab				*/
void dshowMainFrame::CreateSelfTab(TGTab *tab)
{
	TGCompositeFrame *me = tab->AddTab("Self");

	fSelfCanvas = new TRootEmbeddedCanvas ("SelfCanvas", me, 1600, 800);
   	me->AddFrame(fSelfCanvas, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 10, 10, 10, 1));

   	TGHorizontalFrame *hframe=new TGHorizontalFrame(me);
	TGButtonGroup *bg = new TGButtonGroup(hframe, "Histogramms", kHorizontalFrame);
	rSelf2D =     new TGRadioButton(bg, "&2D        ");
	rSelfChan =   new TGRadioButton(bg, "&Channels  ");
	rSelfAll =    new TGRadioButton(bg, "&Average   ");
	rSelfSingle = new TGRadioButton(bg, "&Selected  ");
	rSelf2D->SetState(kButtonDown);
   	hframe->AddFrame(bg, new TGLayoutHints(kLHintsLeft | kLHintsCenterY, 5, 5, 3, 4));

   	TGTextButton *reset = new TGTextButton(hframe,"&Reset");
	reset->Connect("Clicked()", "dshowMainFrame", this, "ResetSelfHists()");
   	hframe->AddFrame(reset, new TGLayoutHints(kLHintsCenterY | kLHintsRight, 5, 5, 3, 4));

    	me->AddFrame(hframe, new TGLayoutHints(kLHintsBottom | kLHintsExpandX, 2, 2, 2, 2));
}

/*	Create event spectrum tab				*/
void dshowMainFrame::CreateSpectrumTab(TGTab *tab)
{
	TGCompositeFrame *me = tab->AddTab("Spectrum");

	fSpectrumCanvas = new TRootEmbeddedCanvas ("SpectrumCanvas", me, 1600, 800);
   	me->AddFrame(fSpectrumCanvas, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 10, 10, 10, 1));

   	TGHorizontalFrame *hframe=new TGHorizontalFrame(me);
	TGButtonGroup *bg = new TGButtonGroup(hframe, "Histogramms", kHorizontalFrame);
	rSpect2D =     new TGRadioButton(bg, "&2D        ");
	rSpectChan =   new TGRadioButton(bg, "&Channels  ");
	rSpectAll =    new TGRadioButton(bg, "&Average   ");
	rSpectSingle = new TGRadioButton(bg, "&Selected  ");
	rSpect2D->SetState(kButtonDown);
   	hframe->AddFrame(bg, new TGLayoutHints(kLHintsLeft | kLHintsCenterY, 5, 5, 3, 4));

   	TGTextButton *reset = new TGTextButton(hframe,"&Reset");
	reset->Connect("Clicked()", "dshowMainFrame", this, "ResetSpectrumHists()");
   	hframe->AddFrame(reset, new TGLayoutHints(kLHintsCenterY | kLHintsRight, 5, 5, 3, 4));

    	me->AddFrame(hframe, new TGLayoutHints(kLHintsBottom | kLHintsExpandX, 2, 2, 2, 2));
}

/*	Create time tab						*/
void dshowMainFrame::CreateTimeTab(TGTab *tab)
{
	TGCompositeFrame *me = tab->AddTab("Time");

	fTimeCanvas = new TRootEmbeddedCanvas ("TimeCanvas", me, 1600, 800);
   	me->AddFrame(fTimeCanvas, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 10, 10, 10, 1));

   	TGHorizontalFrame *hframe=new TGHorizontalFrame(me);
	TGButtonGroup *bg = new TGButtonGroup(hframe, "Histogramms", kHorizontalFrame);
	rTimeAll =    new TGRadioButton(bg, "&Average   ");
	rTimeSingle = new TGRadioButton(bg, "&Selected  ");
	rTimeAll->SetState(kButtonDown);
   	hframe->AddFrame(bg, new TGLayoutHints(kLHintsLeft | kLHintsCenterY, 5, 5, 3, 4));

	TGLabel *lb = new TGLabel(hframe, "Threshold:");
   	hframe->AddFrame(lb, new TGLayoutHints(kLHintsLeft | kLHintsCenterY, 5, 5, 3, 4));		
	nTimeBThreshold = new TGNumberEntry(hframe, 0, 5, -1, TGNumberFormat::kNESInteger, TGNumberFormat::kNEANonNegative);
	CommonData->TimeBThreshold = 100;
	nTimeBThreshold->SetIntNumber(CommonData->TimeBThreshold);
	nTimeBThreshold->Connect("ValueChanged(Long_t)", "dshowMainFrame", this, "ChangeTimeBThr()");
   	hframe->AddFrame(nTimeBThreshold, new TGLayoutHints(kLHintsLeft  | kLHintsCenterY, 5, 5, 3, 4));

   	TGTextButton *reset = new TGTextButton(hframe,"&Reset");
	reset->Connect("Clicked()", "dshowMainFrame", this, "ResetTimeHists()");
   	hframe->AddFrame(reset, new TGLayoutHints(kLHintsCenterY | kLHintsRight, 5, 5, 3, 4));

    	me->AddFrame(hframe, new TGLayoutHints(kLHintsBottom | kLHintsExpandX, 2, 2, 2, 2));
}

/*	Create event display tab				*/
void dshowMainFrame::CreateEventTab(TGTab *tab)
{
	TGLabel *lb;

	TGCompositeFrame *me = tab->AddTab("Event");

	fEventCanvas = new TRootEmbeddedCanvas ("EventCanvas", me, 1600, 800);
   	me->AddFrame(fEventCanvas, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 10, 10, 10, 1));

   	TGHorizontalFrame *hframe=new TGHorizontalFrame(me);

	lb = new TGLabel(hframe, "PMT Threshold:");
   	hframe->AddFrame(lb, new TGLayoutHints(kLHintsLeft | kLHintsCenterY, 5, 5, 3, 4));		
	nPMTThreshold = new TGNumberEntry(hframe, 0, 5, -1, TGNumberFormat::kNESInteger, TGNumberFormat::kNEANonNegative);
	nPMTThreshold->SetIntNumber(10);
   	hframe->AddFrame(nPMTThreshold, new TGLayoutHints(kLHintsLeft  | kLHintsCenterY, 5, 5, 3, 4));

	lb = new TGLabel(hframe, "SiPM Threshold:");
   	hframe->AddFrame(lb, new TGLayoutHints(kLHintsLeft | kLHintsCenterY, 5, 5, 3, 4));		
	nSiPMThreshold = new TGNumberEntry(hframe, 0, 5, -1, TGNumberFormat::kNESInteger, TGNumberFormat::kNEANonNegative);
	nSiPMThreshold->SetIntNumber(10);
   	hframe->AddFrame(nSiPMThreshold, new TGLayoutHints(kLHintsLeft  | kLHintsCenterY, 5, 5, 3, 4));

	lb = new TGLabel(hframe, "PMT Sum Threshold:");
   	hframe->AddFrame(lb, new TGLayoutHints(kLHintsLeft | kLHintsCenterY, 5, 5, 3, 4));		
	nPMTSumThreshold = new TGNumberEntry(hframe, 0, 5, -1, TGNumberFormat::kNESInteger, TGNumberFormat::kNEANonNegative);
	nPMTSumThreshold->SetIntNumber(100);
   	hframe->AddFrame(nPMTSumThreshold, new TGLayoutHints(kLHintsLeft  | kLHintsCenterY, 5, 5, 3, 4));

	lb = new TGLabel(hframe, "SiPM Sum Threshold:");
   	hframe->AddFrame(lb, new TGLayoutHints(kLHintsLeft | kLHintsCenterY, 5, 5, 3, 4));		
	nSiPMSumThreshold = new TGNumberEntry(hframe, 0, 5, -1, TGNumberFormat::kNESInteger, TGNumberFormat::kNEANonNegative);
	nSiPMSumThreshold->SetIntNumber(100);
   	hframe->AddFrame(nSiPMSumThreshold, new TGLayoutHints(kLHintsLeft  | kLHintsCenterY, 5, 5, 3, 4));

    	me->AddFrame(hframe, new TGLayoutHints(kLHintsBottom | kLHintsExpandX, 2, 2, 2, 2));
}

/*	Timer interrupt - once per second		*/
void dshowMainFrame::OnTimer(void)
{
	if (Pause->IsOn()) return;
	TimerCnt++;
	if (TimerCnt >= nRefresh->GetIntNumber()) {
		TimerCnt = 0;
		DoDraw();
	}
}

/*	Main drawing function. Called by timer		*/
void dshowMainFrame::DoDraw(void)
{
	char str[1024];
   	TCanvas *fCanvas;
	int mod, chan;

	mod = nWFD->GetIntNumber() - 1;
	chan = nChan->GetIntNumber();

	TThread::Lock();

/*		Status bar		*/
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

/*		Scope			*/
   	fCanvas = fWaveCanvas->GetCanvas();
   	fCanvas->cd();
	if (CommonData->hWaveForm) {
		CommonData->hWaveForm->SetMinimum();
		CommonData->hWaveForm->SetMarkerSize(2);
		CommonData->hWaveForm->SetMarkerColor(kBlue);
		CommonData->hWaveForm->SetMarkerStyle(20);
		CommonData->hWaveForm->DrawCopy("p");
		delete CommonData->hWaveForm;
		CommonData->hWaveForm = NULL;
	}
   	fCanvas->Update();

/*		Self			*/
   	fCanvas = fSelfCanvas->GetCanvas();
   	fCanvas->cd();
	if (rSelf2D->IsOn()) {
		CommonData->hAmpS[mod]->Draw("color");
	} else if (rSelfChan->IsOn()) {
		CommonData->hAmpS[mod]->ProjectionX()->Draw();	
	} else if (rSelfAll->IsOn()) {
		CommonData->hAmpS[mod]->ProjectionY()->Draw();
	} else if (rSelfSingle->IsOn()) {
		CommonData->hAmpS[mod]->ProjectionY("_py", chan+1, chan+1)->Draw();
	}
   	fCanvas->Update();

/*		Spectrum			*/
   	fCanvas = fSpectrumCanvas->GetCanvas();
   	fCanvas->cd();
	if (rSpect2D->IsOn()) {
		CommonData->hAmpE[mod]->Draw("color");
	} else if (rSpectChan->IsOn()) {
		CommonData->hAmpE[mod]->ProjectionX()->Draw();	
	} else if (rSpectAll->IsOn()) {
		CommonData->hAmpE[mod]->ProjectionY()->Draw();
	} else if (rSpectSingle->IsOn()) {
		CommonData->hAmpE[mod]->ProjectionY("_py", chan+1, chan+1)->Draw();
	}
   	fCanvas->Update();
/*		Times				*/
   	fCanvas = fTimeCanvas->GetCanvas();
   	fCanvas->cd();
	if (rTimeAll->IsOn()) {
		CommonData->hTimeA[mod]->ProjectionY()->Draw();
		CommonData->hTimeB[mod]->ProjectionY()->Draw("same");
	} else if (rTimeSingle->IsOn()) {
		CommonData->hTimeA[mod]->ProjectionY("_py", chan+1, chan+1)->Draw();
		CommonData->hTimeB[mod]->ProjectionY("_py", chan+1, chan+1)->Draw("same");
	}
   	fCanvas->Update();
/*		Event display			*/
   	fCanvas = fEventCanvas->GetCanvas();
   	fCanvas->cd();
	DrawEvent(fCanvas);
   	fCanvas->Update();

	TThread::UnLock();
}

/*	Draw a singlwe event					*/
void dshowMainFrame::DrawEvent(TCanvas *cv)
{
	TBox PmtBox;
	TBox SipmBox;
	int i, j, k, n;
	int thSiPM, thPMT;

	if (!CommonData->EventHits) return;
//		Draw DANSS	
	cv->Clear();
	SipmBox.SetLineColor(kGray);
	SipmBox.SetFillStyle(kNone);

	PmtBox.SetLineColor(kBlack);
	PmtBox.SetFillStyle(kNone);
	
	for (k=0; k<2; k++) for (i=0; i<5; i++) for (j=0; j<5; j++) 
		PmtBox.DrawBox(0.05 + 0.5*k + 0.08*i, 0.1 + 0.16*j, 0.13 + 0.5*k + 0.08*i, 0.26 +0.16*j);
	
	for (k=0; k<2; k++) for (i=0; i<25; i++) for (j=0; j<50; j++) 
		SipmBox.DrawBox(0.05 + 0.5*k + 0.016*i, 0.1 + 0.008*k + 0.016*j, 0.066 + 0.5*k + 0.016*i, 0.108 + 0.008*k + 0.016*j);
//		Draw Hits
	thPMT = nPMTThreshold->GetIntNumber();
	thSiPM = nSiPMThreshold->GetIntNumber();
	PmtBox.SetLineWidth(5);
	for (n=0; n<CommonData->EventHits; n++) if (CommonData->Event[n].type == TYPE_PMT && CommonData->Event[n].amp > thPMT) {
		i = CommonData->Event[n].xy;
		j = CommonData->Event[n].z / 2;
		k = CommonData->Event[n].z & 1;
		PmtBox.SetLineColor(PaletteStart + CommonData->Event[n].amp/16);
		PmtBox.DrawBox(0.05 + 0.5*k + 0.08*i, 0.1 + 0.16*j, 0.13 + 0.5*k + 0.08*i, 0.26 +0.16*j);
	}

	SipmBox.SetFillStyle(1000);
	for (n=0; n<CommonData->EventHits; n++) if (CommonData->Event[n].type == TYPE_SIPM && CommonData->Event[n].amp > thSiPM) {
		i = CommonData->Event[n].xy;
		j = CommonData->Event[n].z / 2;
		k = CommonData->Event[n].z & 1;
		SipmBox.SetFillColor(PaletteStart + CommonData->Event[n].amp/16);
		SipmBox.DrawBox(0.05 + 0.5*k + 0.016*i, 0.1 + 0.008*k + 0.016*j, 0.066 + 0.5*k + 0.016*i, 0.108 + 0.008*k + 0.016*j);
	}

	CommonData->EventHits = 0;
}

/*	show file open dialog to play data file			*/
void dshowMainFrame::PlayFileDialog(void)
{
	TString *cmd;
	TObject *ptr;
	
	new TGFileDialog(gClient->GetRoot(), this, kFDOpen, PlayFile);
	
	ptr = PlayFile->fFileNamesList->First();
	if (ptr) {
		cmd = new TString(myDir);
		*cmd += "/dplay -p ";
		*cmd += TString::Itoa(UDPPORT, 10);
		*cmd += " -s ";
		*cmd += TString::Itoa(nPlayBlocks->GetIntNumber(), 10);
		for (; ptr; ptr = PlayFile->fFileNamesList->After(ptr)) {
			*cmd += " ";
			*cmd += ptr->GetName();			
		}
		*cmd += " &";
		printf("CMD: %s\n", cmd->Data());
		system(cmd->Data());
		delete cmd;
	}
}

/*	Process an event					*/
void dshowMainFrame::ProcessEvent(char *data)
{
	struct hw_rec_struct *rec;
	struct rec_header_struct *head;
	int i, iptr, len, amp;
	double t;
	char strl[1024];
	int mod, chan;
	int evt_len, evt_fill;
	void *ptr;
	int sumPMT, sumSiPM;

	sumPMT = 0;
	sumSiPM = 0;

	if (!CommonData->EventHits) {
		evt_fill = 1;
	} else {
		evt_fill = 0;
	}

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
		mod = rec->module;
		chan = rec->chan;
		switch(rec->type) {
		case TYPE_MASTER:
			for (i = 0; i < rec->len - 2; i++) if (rec->d[i+1] & 0x4000) rec->d[i+1] |= 0x8000;
			amp = FindMaxofShort(&rec->d[1], rec->len - 2);
			if (Map[mod-1][chan].type == TYPE_SIPM) sumSiPM += amp;
			if (Map[mod-1][chan].type == TYPE_PMT) sumPMT += amp;
			if (!CommonData->hWaveForm && rWaveTrig->IsOn() && nWFD->GetIntNumber() == mod && nChan->GetIntNumber() == chan 
				&& amp > nWaveThreshold->GetIntNumber()) {
				sprintf(strl, "Event WaveForm for %d.%2.2d", mod, chan);
				CommonData->hWaveForm = new TH1D("hWave", strl, rec->len - 2, 0, NSPERCHAN * (rec->len - 2));
				for (i = 0; i < rec->len - 2; i++) CommonData->hWaveForm->SetBinContent(i + 1, (double) rec->d[i+1]);
			}
			CommonData->hAmpE[mod-1]->Fill((double)chan, (double)amp);
			t = NSPERCHAN * FindHalfTime(&rec->d[1], rec->len - 2, amp);
			CommonData->hTimeA[mod-1]->Fill((double)chan, t);
			if (amp > CommonData->TimeBThreshold) CommonData->hTimeB[mod-1]->Fill((double)chan, t);
			if (evt_fill && Map[mod-1][chan].type >= 0) {
				if (CommonData->EventHits >= CommonData->EventLength) {
					ptr = realloc(CommonData->Event, (CommonData->EventLength + 1024) * sizeof(struct evt_disp_struct));
					if (!ptr) break;
					CommonData->Event = (struct evt_disp_struct *)ptr;
					CommonData->EventLength += 1024;
				}
				CommonData->Event[CommonData->EventHits].amp = amp;
				CommonData->Event[CommonData->EventHits].type = Map[mod-1][chan].type;
				CommonData->Event[CommonData->EventHits].xy = Map[mod-1][chan].xy;
				CommonData->Event[CommonData->EventHits].z = Map[mod-1][chan].z;
				CommonData->EventHits++;
			}
			break;
		case TYPE_RAW:
			amp = FindMaxofShort(&rec->d[1], rec->len - 2);
			if (!CommonData->hWaveForm && rWaveTrig->IsOn() && nWFD->GetIntNumber() == mod && nChan->GetIntNumber() == chan
				&& amp > nWaveThreshold->GetIntNumber()) {
				sprintf(strl, "Event WaveForm for %d.%2.2d", mod, chan);
				CommonData->hWaveForm = new TH1D("hWave", strl, rec->len - 2, 0, NSPERCHAN * (rec->len - 2));
				for (i = 0; i < rec->len - 2; i++) CommonData->hWaveForm->SetBinContent(i + 1, (double) rec->d[i+1]);
			}
			break;
		case TYPE_SUM:
			for (i = 0; i < rec->len - 2; i++) if (rec->d[i+1] & 0x4000) rec->d[i+1] |= 0x8000;
			amp = FindMaxofShort(&rec->d[1], rec->len - 2);
			if (!CommonData->hWaveForm && rWaveHist->IsOn() && nWFD->GetIntNumber() == mod && nChan->GetIntNumber() == chan
				&& amp > nWaveThreshold->GetIntNumber()) {
				sprintf(strl, "Event WaveForm for %d.%2.2d", mod, chan);
				CommonData->hWaveForm = new TH1D("hWave", strl, rec->len - 2, 0, NSPERCHAN * (rec->len - 2));
				for (i = 0; i < rec->len - 2; i++) CommonData->hWaveForm->SetBinContent(i + 1, (double) rec->d[i+1]);
			}
			break;
		}
	}
	if (evt_fill && (nPMTSumThreshold->GetIntNumber() > sumPMT || nSiPMSumThreshold->GetIntNumber() > sumSiPM)) CommonData->EventHits = 0;

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

/*	Reset histogramms				*/
void dshowMainFrame::ResetSelfHists(void)
{
	int i;

	TThread::Lock();
	for (i=0; i<MAXWFD; i++) CommonData->hAmpS[i]->Reset();
	TThread::UnLock();
}

void dshowMainFrame::ResetSpectrumHists(void)
{
	int i;

	TThread::Lock();
	for (i=0; i<MAXWFD; i++) CommonData->hAmpE[i]->Reset();
	TThread::UnLock();
}

void dshowMainFrame::ResetTimeHists(void)
{
	int i;

	TThread::Lock();
	for (i=0; i<MAXWFD; i++) {
		CommonData->hTimeA[i]->Reset();
		CommonData->hTimeB[i]->Reset();
	}
	TThread::UnLock();
}

/*	Change TimeB threshold				*/
void dshowMainFrame::ChangeTimeBThr(void)
{
	CommonData->TimeBThreshold = nTimeBThreshold->GetIntNumber();
}

void dshowMainFrame::ReadConfig(const char *fname)
{
	config_t cnf;
	long tmp;
	int i, j, k, type, xy, z;
	config_setting_t *ptr_xy;
	config_setting_t *ptr_z;
	char str[1024];

	memset(Map, -1, sizeof(Map));
	config_init(&cnf);
	if (config_read_file(&cnf, fname) != CONFIG_TRUE) {
        	printf("DSHOW: Configuration error in file %s at line %d: %s\n", fname, config_error_line(&cnf), config_error_text(&cnf));
		config_destroy(&cnf);
            	return;
    	}
	
	for(i=0; i<MAXWFD; i++) { 
	//	int Type;
		sprintf(str, "Map.Dev%3.3d.Type", i+1);
		type = (config_lookup_int(&cnf, str, &tmp)) ? tmp : -1;
		if (type < 0) continue;
		sprintf(str, "Map.Dev%3.3d.XY", i+1);
		ptr_xy = config_lookup(&cnf, str);
		sprintf(str, "Map.Dev%3.3d.Z", i+1);
		ptr_z = config_lookup(&cnf, str);
		for (j=0; j<64; j++) Map[i][j].type = type;
		switch(type) {
		case TYPE_SIPM:
			if (!ptr_xy || !ptr_z) break;
			for (j=0; j<4; j++) {
				xy = config_setting_get_int_elem(ptr_xy, j);
				z = config_setting_get_int_elem(ptr_z, j);
				for (k=0; k<15; k++) {
					if (xy < 0 || ((z/2) == 16 && (k%3) == 2)) {
						Map[i][16*j+k].type = -1;
					} else {
						Map[i][16*j+k].xy = 5*xy + ((z&1) ? 4 - (k/3) : (k/3));
						Map[i][16*j+k].z = 6*(z/2) + (z&1) + 2*(k%3);
					}
				}
				Map[i][16*j+15].type = -1;
			}
			break;
		case TYPE_PMT:
			if (!ptr_xy || !ptr_z) break;
			for (j=0; j<64; j++) {
				Map[i][j].xy = config_setting_get_int_elem(ptr_xy, j);
				Map[i][j].z = config_setting_get_int_elem(ptr_z, j);
			}
			break;
		case TYPE_VETO:
			break;
		}
	}

	config_destroy(&cnf);
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

int FindHalfTime(short int *data, int cnt, int amp)
{
	int i;
	for (i=0; i<cnt; i++) if (data[i] > amp/2) break;
	return i;
}

/*	the main						*/
int main(int argc, char **argv) {
   	TApplication theApp("DANSS_SHOW", &argc, argv);
   	dshowMainFrame *frame = new dshowMainFrame(gClient->GetRoot(), 2000, 1000);
	frame->ReadConfig((argc > 1) ? argv[1] : DEFCONFIG);
   	theApp.Run();
   	return 0;
}

