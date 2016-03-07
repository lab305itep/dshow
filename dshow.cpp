//		Simple data analysis and show for DANSS
//	I.G. Alekseev & D.N. Svirida, ITEP, Moscow, 2016

#define _FILE_OFFSET_BITS 64
#include <TApplication.h>
#include <TGClient.h>
#include <TCanvas.h>
#include <TColor.h>
#include <TGaxis.h>
#include <TBox.h>
#include <TGButton.h>
#include <TGButtonGroup.h>
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
const char *RateName[] = {"All", "NO veto"};

/*	Class dshowMainFrame - the main window		*/
/*	Main window Constructor				*/
dshowMainFrame::dshowMainFrame(const TGWindow *p, UInt_t w, UInt_t h) : TGMainFrame(p, w, h) 
{
	char strs[64];
	char strl[1024];
	Int_t StatusBarParts[] = {10, 10, 10, 10, 10, 50};	// status bar parts
	Double_t RGB_r[]    = {0., 0.0, 1.0, 1.0, 1.0};
   	Double_t RGB_g[]    = {0., 0.0, 0.0, 1.0, 1.0};
   	Double_t RGB_b[]    = {0., 1.0, 0.0, 0.0, 1.0};
   	Double_t RGB_stop[] = {0., .25, .50, .75, 1.0};
	int i;
	TGLabel *lb;
	
	// Create our data structures and histogramms
	CommonData = (struct common_data_struct *) malloc(sizeof(struct common_data_struct));
	memset(CommonData, 0, sizeof(struct common_data_struct));

	gStyle->SetOptStat("e");
	gStyle->SetOptFit(111);
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
//	TLegend
	TimeLegend = new TLegend(0.76, 0.8, 0.97, 0.88);
	TimeLegend->AddEntry(CommonData->hTimeA[0], "All events", "L");
	TimeLegend->AddEntry(CommonData->hTimeB[0], "Amplitude above threshold", "L");

//	TH1D *hSiPMsum[2];	// SiPM summa, all/no veto
	CommonData->hSiPMsum[0] = new TH1D("hSiPMsum0", "Sum of all SiPM counts, MeV", 200, 0, 10.0);
	CommonData->hSiPMsum[1] = new TH1D("hSiPMsum1", "Sum of all SiPM counts, no veto, MeV", 200, 0, 10.0);
	CommonData->hSiPMsum[1]->SetLineColor(kBlue);
//	TH1D *hSiPMhits[2];	// SiPM Hits, all/no veto
	CommonData->hSiPMhits[0] = new TH1D("hSiPMhits0", "Number of SiPM hits per event", 200, 0, 200);
	CommonData->hSiPMhits[1] = new TH1D("hSiPMhits1", "Number of SiPM hits per event, no veto", 200, 0, 200);
	CommonData->hSiPMhits[1]->SetLineColor(kBlue);
//	TH1D *hPMTsum[2];	// PMT summa, all/no veto
	CommonData->hPMTsum[0] = new TH1D("hPMTsum0", "Sum of all PMT counts, MeV", 200, 0, 10.0);
	CommonData->hPMTsum[1] = new TH1D("hPMTsum1", "Sum of all PMT counts, no veto, MeV", 200, 0, 10.0);
	CommonData->hPMTsum[1]->SetLineColor(kBlue);
//	TH1D *hPMThits[2];	// PMT Hits, all/no veto
	CommonData->hPMThits[0] = new TH1D("hPMThits0", "Number of PMT hits per event", 50, 0, 50);
	CommonData->hPMThits[1] = new TH1D("hPMThits1", "Number of PMT hits per event, no veto", 50, 0, 50);
	CommonData->hPMThits[1]->SetLineColor(kBlue);
//	TH1D *hSPRatio[2];	// SiPM sum to PTMT sum ratio, all/no veto
	CommonData->hSPRatio[0] = new TH1D("hSPRatio0", "SiPM to PMT ratio", 100, 0, 5.0);
	CommonData->hSPRatio[1] = new TH1D("hSPRatio1", "SiPM to PMT ratio, no veto", 100, 0, 5.0);
	CommonData->hSPRatio[1]->SetLineColor(kBlue);
//	TLegend
	SummaLegend = new TLegend(0.6, 0.1, 0.9, 0.3);
	SummaLegend->AddEntry(CommonData->hSiPMsum[0], "All events", "L");
	SummaLegend->AddEntry(CommonData->hSiPMsum[1], "NO veto", "L");

//	Rate
	CommonData->hRate = new TH1D("hRate", "Trigger rate, Hz", RATELEN, 0, RATELEN);
	CommonData->hRate->SetStats(0);
	CommonData->hRate->SetMarkerStyle(20);
	CommonData->hRate->SetMarkerColor(kBlue);
	CommonData->fRate = (float *) malloc(RATELEN * sizeof(float));
	memset(CommonData->fRate, 0, RATELEN * sizeof(float));
	CommonData->iRatePos = 0;
	CommonData->gLastTime = -1;

	PlayFile = new TGFileInfo();
	PlayFile->fFileTypes = DataFileTypes;
	PlayFile->fIniDir = strdup(".");
	PlayFile->SetMultipleSelection(1);

   	PaletteStart = TColor::CreateGradientColorTable(sizeof(RGB_stop) / sizeof(RGB_stop[0]), RGB_stop, RGB_r, RGB_g, RGB_b, MAXRGB);
	gStyle->SetPalette();

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
	CreateSummaTab(tab);
	CreateRateTab(tab);
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

	TGGroupFrame *grPlay = new TGGroupFrame(vframe, "Play a file", kVerticalFrame);
	
   	TGTextButton *open = new TGTextButton(grPlay,"&Open");
	open->Connect("Clicked()", "dshowMainFrame", this, "PlayFileDialog()");
   	grPlay->AddFrame(open, new TGLayoutHints(kLHintsCenterX | kLHintsTop, 5, 5, 3, 4));

   	TGTextButton *stop = new TGTextButton(grPlay,"&Stop");
	stop->Connect("Clicked()", "dshowMainFrame", this, "PlayFileStop()");
   	grPlay->AddFrame(stop, new TGLayoutHints(kLHintsCenterX | kLHintsTop, 5, 5, 3, 4));

	lb = new TGLabel(grPlay, "Blocks/s:");
   	grPlay->AddFrame(lb, new TGLayoutHints(kLHintsLeft | kLHintsTop, 5, 5, 3, 4));
	nPlayBlocks = new TGNumberEntry(grPlay, 0, 8, -1, TGNumberFormat::kNESInteger, TGNumberFormat::kNEANonNegative);
	nPlayBlocks->SetIntNumber(0);
   	grPlay->AddFrame(nPlayBlocks, new TGLayoutHints(kLHintsCenterX  | kLHintsTop, 5, 5, 3, 4));

	PlayProgress = new TGHProgressBar(grPlay, TGProgressBar::kStandard, 100);
	PlayProgress->ShowPosition();
	PlayProgress->SetBarColor("green");
	PlayProgress->SetRange(0.0, 100.0);
	grPlay->AddFrame(PlayProgress, new TGLayoutHints(kLHintsExpandX  | kLHintsTop, 5, 5, 3, 1));

   	vframe->AddFrame(grPlay, new TGLayoutHints(kLHintsCenterX  | kLHintsTop, 5, 5, 3, 4));

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
	DataThread = NULL;
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
	if (DataThread) DataThread->Join();
	Cleanup();
//		all memory will be freed without us anyway...
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
   	hframe->AddFrame(nPMTThreshold, new TGLayoutHints(kLHintsLeft | kLHintsCenterY, 5, 5, 3, 4));

	lb = new TGLabel(hframe, "SiPM Threshold:");
   	hframe->AddFrame(lb, new TGLayoutHints(kLHintsLeft | kLHintsCenterY, 5, 5, 3, 4));		
	nSiPMThreshold = new TGNumberEntry(hframe, 0, 5, -1, TGNumberFormat::kNESInteger, TGNumberFormat::kNEANonNegative);
	nSiPMThreshold->SetIntNumber(10);
   	hframe->AddFrame(nSiPMThreshold, new TGLayoutHints(kLHintsLeft | kLHintsCenterY, 5, 5, 3, 4));

	lb = new TGLabel(hframe, "PMT Sum Threshold:");
   	hframe->AddFrame(lb, new TGLayoutHints(kLHintsLeft | kLHintsCenterY, 5, 5, 3, 4));		
	nPMTSumThreshold = new TGNumberEntry(hframe, 0, 5, -1, TGNumberFormat::kNESInteger, TGNumberFormat::kNEANonNegative);
	nPMTSumThreshold->SetIntNumber(100);
   	hframe->AddFrame(nPMTSumThreshold, new TGLayoutHints(kLHintsLeft | kLHintsCenterY, 5, 5, 3, 4));

	lb = new TGLabel(hframe, "SiPM Sum Threshold:");
   	hframe->AddFrame(lb, new TGLayoutHints(kLHintsLeft | kLHintsCenterY, 5, 5, 3, 4));		
	nSiPMSumThreshold = new TGNumberEntry(hframe, 0, 5, -1, TGNumberFormat::kNESInteger, TGNumberFormat::kNEANonNegative);
	nSiPMSumThreshold->SetIntNumber(100);
   	hframe->AddFrame(nSiPMSumThreshold, new TGLayoutHints(kLHintsLeft | kLHintsCenterY, 5, 5, 3, 4));

    	me->AddFrame(hframe, new TGLayoutHints(kLHintsBottom | kLHintsExpandX, 2, 2, 2, 2));
}

/*	Create event summa tab				*/
void dshowMainFrame::CreateSummaTab(TGTab *tab)
{
	TGLabel *lb;

	TGCompositeFrame *me = tab->AddTab("Summa");

	fSummaCanvas = new TRootEmbeddedCanvas ("SummaCanvas", me, 1600, 800);
   	me->AddFrame(fSummaCanvas, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 10, 10, 10, 1));

   	TGHorizontalFrame *hframe=new TGHorizontalFrame(me);

	lb = new TGLabel(hframe, "SiPM Threshold:");
   	hframe->AddFrame(lb, new TGLayoutHints(kLHintsLeft | kLHintsCenterY, 5, 5, 3, 4));		
	SummaSiPMThreshold = new TGNumberEntry(hframe, 0, 5, -1, TGNumberFormat::kNESInteger, TGNumberFormat::kNEANonNegative);
	CommonData->SummaSiPMThreshold = 30;
	SummaSiPMThreshold->SetIntNumber(CommonData->SummaSiPMThreshold);
	SummaSiPMThreshold->Connect("ValueChanged(Long_t)", "dshowMainFrame", this, "ChangeSummaPars()");
   	hframe->AddFrame(SummaSiPMThreshold, new TGLayoutHints(kLHintsLeft | kLHintsCenterY, 5, 5, 3, 4));

	lb = new TGLabel(hframe, "SiPM Time range, ns:");
   	hframe->AddFrame(lb, new TGLayoutHints(kLHintsLeft | kLHintsCenterY, 15, 5, 3, 4));
	lbSiPMtime[0] = new TGLabel(hframe, "160.0");
   	hframe->AddFrame(lbSiPMtime[0], new TGLayoutHints(kLHintsLeft | kLHintsCenterY, 5, 5, 3, 4));
	SummaSiPMtime = new TGDoubleHSlider(hframe, 200, kDoubleScaleNo);
	SummaSiPMtime->SetRange(0, 500);
	CommonData->SummaSiPMtime[0] = 160.0;
	CommonData->SummaSiPMtime[1] = 240.0;
	SummaSiPMtime->SetPosition(CommonData->SummaSiPMtime[0], CommonData->SummaSiPMtime[1]);
	SummaSiPMtime->Connect("PositionChanged()", "dshowMainFrame", this, "ChangeSummaPars()");
   	hframe->AddFrame(SummaSiPMtime, new TGLayoutHints(kLHintsLeft | kLHintsCenterY, 5, 5, 3, 4));
	lbSiPMtime[1] = new TGLabel(hframe, "240.0");
   	hframe->AddFrame(lbSiPMtime[1], new TGLayoutHints(kLHintsLeft | kLHintsCenterY, 5, 5, 3, 4));

   	TGTextButton *reset = new TGTextButton(hframe,"&Reset");
	reset->Connect("Clicked()", "dshowMainFrame", this, "ResetSummaHists()");
   	hframe->AddFrame(reset, new TGLayoutHints(kLHintsCenterY | kLHintsRight, 5, 5, 3, 4));

    	me->AddFrame(hframe, new TGLayoutHints(kLHintsBottom | kLHintsExpandX, 2, 2, 2, 2));
}

/*	Create event rate tab				*/
void dshowMainFrame::CreateRateTab(TGTab *tab)
{
	TGCompositeFrame *me = tab->AddTab("Rates");

	fRateCanvas = new TRootEmbeddedCanvas ("RateCanvas", me, 1600, 800);
   	me->AddFrame(fRateCanvas, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 10, 10, 10, 1));
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
	int i, j;

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
		sprintf(str, "Module: %d. Amplitude versus channel number.", mod + 1);
		CommonData->hAmpS[mod]->SetTitle(str);
		CommonData->hAmpS[mod]->Draw("color");
	} else if (rSelfChan->IsOn()) {
		sprintf(str, "Module: %d. Channel occupancy.", mod + 1);
		CommonData->hAmpS[mod]->ProjectionX()->SetTitle(str);
		CommonData->hAmpS[mod]->ProjectionX()->Draw();	
	} else if (rSelfAll->IsOn()) {
		sprintf(str, "Module: %d. Amplitude distribution for all channels.", mod + 1);
		CommonData->hAmpS[mod]->ProjectionY()->SetTitle(str);
		CommonData->hAmpS[mod]->ProjectionY()->Draw();
	} else if (rSelfSingle->IsOn()) {
		sprintf(str, "Module: %d. Amplitude distribution for channel %d.", mod + 1, chan);
		CommonData->hAmpS[mod]->ProjectionY("_py", chan + 1, chan + 1)->SetTitle(str);
		CommonData->hAmpS[mod]->ProjectionY("_py", chan + 1, chan + 1)->Draw();
	}
   	fCanvas->Update();

/*		Spectrum			*/
   	fCanvas = fSpectrumCanvas->GetCanvas();
   	fCanvas->cd();
	if (rSpect2D->IsOn()) {
		sprintf(str, "Module: %d. Amplitude versus channel number.", mod + 1);
		CommonData->hAmpE[mod]->SetTitle(str);
		CommonData->hAmpE[mod]->Draw("color");
	} else if (rSpectChan->IsOn()) {
		sprintf(str, "Module: %d. Channel occupancy.", mod + 1);
		CommonData->hAmpE[mod]->ProjectionX()->SetTitle(str);
		CommonData->hAmpE[mod]->ProjectionX()->Draw();	
	} else if (rSpectAll->IsOn()) {
		sprintf(str, "Module: %d. Amplitude distribution for all channels.", mod + 1);
		CommonData->hAmpE[mod]->ProjectionY()->SetTitle(str);
		CommonData->hAmpE[mod]->ProjectionY()->Draw();
	} else if (rSpectSingle->IsOn()) {
		sprintf(str, "Module: %d. Amplitude distribution for channel %d.", mod + 1, chan);
		CommonData->hAmpE[mod]->ProjectionY("_py", chan + 1, chan + 1)->SetTitle(str);
		CommonData->hAmpE[mod]->ProjectionY("_py", chan + 1, chan + 1)->Draw();
	}
   	fCanvas->Update();

/*		Times				*/
   	fCanvas = fTimeCanvas->GetCanvas();
   	fCanvas->cd();
	if (rTimeAll->IsOn()) {
		sprintf(str, "Module: %d. Time distribution for all channels.", mod + 1);
		CommonData->hTimeA[mod]->ProjectionY()->SetTitle(str);
		CommonData->hTimeA[mod]->ProjectionY()->Draw();
		CommonData->hTimeB[mod]->ProjectionY()->Draw("same");
	} else if (rTimeSingle->IsOn()) {
		sprintf(str, "Module: %d. Time distribution for channel %d.", mod + 1, chan);
		CommonData->hTimeA[mod]->ProjectionY("_py", chan + 1, chan + 1)->SetTitle(str);
		CommonData->hTimeA[mod]->ProjectionY("_py", chan + 1, chan + 1)->Draw();
		CommonData->hTimeB[mod]->ProjectionY("_py", chan + 1, chan + 1)->Draw("same");
	}
	TimeLegend->Draw();
   	fCanvas->Update();

/*		Event display			*/
   	fCanvas = fEventCanvas->GetCanvas();
   	fCanvas->cd();
	DrawEvent(fCanvas);
   	fCanvas->Update();

/*		Summa				*/
   	fCanvas = fSummaCanvas->GetCanvas();
   	fCanvas->cd();
	fCanvas->Clear();
	fCanvas->Divide(3, 2);
//	TH1D *hSiPMsum[2];	// SiPM summa, all/no veto
   	fCanvas->cd(1);
	CommonData->hSiPMsum[0]->Draw();
	CommonData->hSiPMsum[1]->Draw("same");
//	TH1D *hSiPMhits[2];	// SiPM Hits, all/no veto
   	fCanvas->cd(2);
	CommonData->hSiPMhits[0]->Draw();
	CommonData->hSiPMhits[1]->Draw("same");
//	TH1D *hPMTsum[2];	// PMT summa, all/no veto
   	fCanvas->cd(4);
	CommonData->hPMTsum[0]->Draw();
	CommonData->hPMTsum[1]->Draw("same");
//	TH1D *hPMThits[2];	// PMT Hits, all/no veto
   	fCanvas->cd(5);
	CommonData->hPMThits[0]->Draw();
	CommonData->hPMThits[1]->Draw("same");
//	TH1D *hSPRatio[2];	// SiPM sum to PTMT sum ratio, all/no veto
   	fCanvas->cd(3);
	CommonData->hSPRatio[0]->Draw();
	CommonData->hSPRatio[1]->Draw("same");
//	TLegend
   	fCanvas->cd(6);
	SummaLegend->Draw();	
   	fCanvas->Update();

/*		Rates				*/
   	fCanvas = fRateCanvas->GetCanvas();
   	fCanvas->cd();
	for (j=0; j<RATELEN; j++) CommonData->hRate->SetBinContent(j+1, CommonData->fRate[(CommonData->iRatePos + j) % RATELEN]);
	CommonData->hRate->Draw("p");
   	fCanvas->Update();

	TThread::UnLock();
}

/*	Draw a singlwe event					*/
void dshowMainFrame::DrawEvent(TCanvas *cv)
{
	TBox PmtBox;
	TBox SipmBox;
	TGaxis ax;
	int i, j, k, n;
	int thSiPM, thPMT;
	const struct vpos_struct {
		double x1;
		double y1;
		double x2;
		double y2;
	} vpos[64] = {
//		Top
		{0.51, 0.94, 0.63, 0.96}, {0.51, 0.91, 0.63, 0.93}, {0.63, 0.94, 0.75, 0.96}, {0.63, 0.91, 0.75, 0.93}, 
		{0.75, 0.94, 0.87, 0.96}, {0.75, 0.91, 0.87, 0.93}, {0.87, 0.94, 0.99, 0.96}, {0.87, 0.91, 0.99, 0.93}, 
//		Reserved
		{-1.0, -1.0, -1.0, -1.0}, {-1.0, -1.0, -1.0, -1.0}, {-1.0, -1.0, -1.0, -1.0}, {-1.0, -1.0, -1.0, -1.0},
		{-1.0, -1.0, -1.0, -1.0}, {-1.0, -1.0, -1.0, -1.0}, {-1.0, -1.0, -1.0, -1.0}, {-1.0, -1.0, -1.0, -1.0},
//		Outer front X, back closer to the cube
		{0.475, 0.1, 0.48, 0.9}, {0.48, 0.1, 0.485, 0.9}, {0.485, 0.1, 0.49, 0.9},
//		Outer front Y, back closer to the cube
		{0.52, 0.1, 0.525, 0.9}, {0.515, 0.1, 0.52, 0.9}, {0.51, 0.1, 0.515, 0.9},
//		Outer back X, back closer to the cube
		{0.01, 0.1, 0.015, 0.9}, {0.015, 0.1, 0.02, 0.9}, {0.02, 0.1, 0.025, 0.9},
//		Outer back Y, back closer to the cube
		{0.975, 0.1, 0.98, 0.9}, {0.98, 0.1, 0.985, 0.9}, {0.985, 0.1, 0.99, 0.9},
//		Inner front X, back closer to the cube
		{0.455, 0.1, 0.46, 0.9}, {0.46, 0.1, 0.465, 0.9}, {0.465, 0.1, 0.47, 0.9},
//		Inner front Y, back closer to the cube
		{0.54, 0.1, 0.545, 0.9}, {0.535, 0.1, 0.54, 0.9}, {0.53, 0.1, 0.535, 0.9},
//		Inner back X, back closer to the cube
		{0.03, 0.1, 0.035, 0.9}, {0.035, 0.1, 0.04, 0.9}, {0.04, 0.1, 0.045, 0.9},
//		Inner back Y, back closer to the cube
		{0.955, 0.1, 0.96, 0.9}, {0.96, 0.1, 0.965, 0.9}, {0.965, 0.1, 0.97, 0.9},
//		Reserved
		{-1.0, -1.0, -1.0, -1.0}, {-1.0, -1.0, -1.0, -1.0}, {-1.0, -1.0, -1.0, -1.0}, {-1.0, -1.0, -1.0, -1.0},
		{-1.0, -1.0, -1.0, -1.0}, {-1.0, -1.0, -1.0, -1.0}, {-1.0, -1.0, -1.0, -1.0}, {-1.0, -1.0, -1.0, -1.0},
		{-1.0, -1.0, -1.0, -1.0}, {-1.0, -1.0, -1.0, -1.0}, {-1.0, -1.0, -1.0, -1.0}, {-1.0, -1.0, -1.0, -1.0},
		{-1.0, -1.0, -1.0, -1.0}, {-1.0, -1.0, -1.0, -1.0}, {-1.0, -1.0, -1.0, -1.0}, {-1.0, -1.0, -1.0, -1.0},
		{-1.0, -1.0, -1.0, -1.0}, {-1.0, -1.0, -1.0, -1.0}, {-1.0, -1.0, -1.0, -1.0}, {-1.0, -1.0, -1.0, -1.0},
		{-1.0, -1.0, -1.0, -1.0}, {-1.0, -1.0, -1.0, -1.0}, {-1.0, -1.0, -1.0, -1.0}, {-1.0, -1.0, -1.0, -1.0},
	};

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
	for (i=0; i<64; i++) if (vpos[i].x1 >= 0) SipmBox.DrawBox(vpos[i].x1, vpos[i].y1, vpos[i].x2, vpos[i].y2);

//		Draw Hits in PMT
	thPMT = nPMTThreshold->GetIntNumber();
	thSiPM = nSiPMThreshold->GetIntNumber();
	PmtBox.SetLineWidth(5);
	for (n=0; n<CommonData->EventHits; n++) if (CommonData->Event[n].type == TYPE_PMT && CommonData->Event[n].amp > thPMT) {
		i = CommonData->Event[n].xy;
		j = CommonData->Event[n].z / 2;
		k = CommonData->Event[n].z & 1;
		if (!k) i = 4 - i;
		PmtBox.SetLineColor(PaletteStart + CommonData->Event[n].amp*MAXRGB/MAXADC);
		PmtBox.DrawBox(0.05 + 0.5*k + 0.08*i, 0.1 + 0.16*j, 0.13 + 0.5*k + 0.08*i, 0.26 +0.16*j);
	}
//		Draw Hits in SiPM
	SipmBox.SetFillStyle(1000);
	for (n=0; n<CommonData->EventHits; n++) if (CommonData->Event[n].type == TYPE_SIPM && CommonData->Event[n].amp > thSiPM) {
		i = CommonData->Event[n].xy;
		j = CommonData->Event[n].z / 2;
		k = CommonData->Event[n].z & 1;
		if (!k) i = 24 - i;
		SipmBox.SetFillColor(PaletteStart + CommonData->Event[n].amp*MAXRGB/MAXADC);
		SipmBox.DrawBox(0.05 + 0.5*k + 0.016*i, 0.1 + 0.008*k + 0.016*j, 0.066 + 0.5*k + 0.016*i, 0.108 + 0.008*k + 0.016*j);
	}
//		Draw Hits in VETO
	for (n=0; n<CommonData->EventHits; n++) if (CommonData->Event[n].type == TYPE_VETO && CommonData->Event[n].amp > thPMT) {
		SipmBox.SetFillColor(PaletteStart + CommonData->Event[n].amp*MAXRGB/MAXADC);
		i = CommonData->Event[n].xy;
		if (vpos[i].x1 < 0) continue;
		SipmBox.DrawBox(vpos[i].x1, vpos[i].y1, vpos[i].x2, vpos[i].y2);
	}
//		Draw palette
	for(i=0; i<MAXRGB; i++) {
		SipmBox.SetFillColor(PaletteStart + i);
		SipmBox.DrawBox(0.05 + 0.9*i/MAXRGB, 0.05, 0.05 + 0.9*(i+1)/MAXRGB, 0.09);
	}

	ax.DrawAxis(0.05, 0.05, 0.95, 0.05, 0.0, (double) MAXADC);

	CommonData->EventHits = 0;
}

/*	show file open dialog and start thread to play to play selected data file(s)	*/
void dshowMainFrame::PlayFileDialog(void)
{
	TString *cmd;

	PlayFileStop();
	new TGFileDialog(gClient->GetRoot(), this, kFDOpen, PlayFile);

	if (PlayFile->fFileNamesList && PlayFile->fFileNamesList->First()) {
		CommonData->iStop = 0;
		DataThread = new TThread("DataThread", DataThreadFunction, (void *) this);
		DataThread->Run();
	}
}

void dshowMainFrame::PlayFileStop(void)
{
	CommonData->iStop = 1;
	PlayProgress->SetPosition(0.0);
	if (DataThread) DataThread->Join();
	DataThread = NULL;
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
	int sumPMT, sumSiPM, sumSiPMc;
	int nSiPM, nSiPMc, nPMT, nVeto;
	long long gTime;
	int gTrig;

	sumPMT = 0;
	sumSiPM = 0;
	nSiPM = 0;
	sumSiPMc = 0;
	nSiPMc = 0;
	nPMT = 0;
	nVeto = 0;
	gTime = -1;

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
			t = NSPERCHAN * FindHalfTime(&rec->d[1], rec->len - 2, amp);
			switch (Map[mod-1][chan].type) {
			case TYPE_SIPM:
				sumSiPM += amp;
				nSiPM++;
				if (amp > CommonData->SummaSiPMThreshold && t > CommonData->SummaSiPMtime[0] && t < CommonData->SummaSiPMtime[1]) {
					sumSiPMc += amp;
					nSiPMc++;					
				}
				break;
			case TYPE_PMT:
				sumPMT += amp;
				nPMT++;
				break;
			case TYPE_VETO:
				nVeto++;
				break;
			}
			if (!CommonData->hWaveForm && rWaveTrig->IsOn() && nWFD->GetIntNumber() == mod && nChan->GetIntNumber() == chan 
				&& amp > nWaveThreshold->GetIntNumber()) {
				sprintf(strl, "Event WaveForm for %d.%2.2d", mod, chan);
				CommonData->hWaveForm = new TH1D("hWave", strl, rec->len - 2, 0, NSPERCHAN * (rec->len - 2));
				CommonData->hWaveForm->SetStats(0);
				for (i = 0; i < rec->len - 2; i++) CommonData->hWaveForm->SetBinContent(i + 1, (double) rec->d[i+1]);
			}
			CommonData->hAmpE[mod-1]->Fill((double)chan, (double)amp);
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
				CommonData->hWaveForm->SetStats(0);
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
		CommonData->ErrorCnt++;
		return;
	}
//	check if a second passed
	if (CommonData->gLastTime < 0) {
		CommonData->gLastTime = gTime;
		CommonData->gTime = gTime;
		CommonData->gTrig = gTrig;
	} else if (gTime - CommonData->gLastTime >= ONESECOND) {
		CommonData->fRate[CommonData->iRatePos] = ((float) ONESECOND) * (gTrig - CommonData->gTrig) / (gTime - CommonData->gTime);
		CommonData->iRatePos = (CommonData->iRatePos + 1) % RATELEN;
		CommonData->gLastTime += ONESECOND;
		CommonData->gTime = gTime;
		CommonData->gTrig = gTrig;
	}
//
	if (evt_fill && (nPMTSumThreshold->GetIntNumber() > sumPMT || nSiPMSumThreshold->GetIntNumber() > sumSiPM)) CommonData->EventHits = 0;
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
	if (!nVeto) {
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

void dshowMainFrame::ResetSummaHists(void)
{
	int i;

	TThread::Lock();
	for (i=0; i<2; i++) {
		CommonData->hSiPMsum[i]->Reset();
		CommonData->hSiPMhits[i]->Reset();
		CommonData->hPMTsum[i]->Reset();
		CommonData->hPMThits[i]->Reset();
		CommonData->hSPRatio[i]->Reset();
	}
	TThread::UnLock();
}

/*	Change TimeB threshold				*/
void dshowMainFrame::ChangeTimeBThr(void)
{
	CommonData->TimeBThreshold = nTimeBThreshold->GetIntNumber();
}

/*	Change Summa parameters				*/
void dshowMainFrame::ChangeSummaPars(void)
{
	char str[64];

	CommonData->SummaSiPMThreshold = SummaSiPMThreshold->GetIntNumber();
	CommonData->SummaSiPMtime[0] = SummaSiPMtime->GetMinPosition();
	CommonData->SummaSiPMtime[1] = SummaSiPMtime->GetMaxPosition();
	sprintf(str, "%5.1f", CommonData->SummaSiPMtime[0]);
	lbSiPMtime[0]->SetTitle(str);
	sprintf(str, "%5.1f", CommonData->SummaSiPMtime[1]);
	lbSiPMtime[1]->SetTitle(str);
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
			for (j=0; j<64; j++) {
				Map[i][j].xy = j;
				Map[i][j].z = 0;
			}			
			break;
		}
	}

	config_destroy(&cnf);
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
	struct timeval tm;
	FILE *fIn;
	off_t fsize;
	off_t rsize;
	int iNum;
	int Cnt;
	TObject *f;
	
	Main = (dshowMainFrame *)ptr;
	CommonData = Main->CommonData;
	CommonData->iError = 0;
	
	buf = (char *)malloc(BSIZE);
	if (!buf) {
		CommonData->iError = 10;
		goto Ret;
	}
	head = (struct rec_header_struct *) buf;
	
	f = Main->PlayFile->fFileNamesList->First();
	iNum = Main->nPlayBlocks->GetIntNumber();
	Main->fStatusBar->SetText(f->GetName(), 5);
	Main->PlayProgress->SetPosition(0);
	fIn = fopen(f->GetName(), "rb");
	if (!fIn) {
		CommonData->iError = 15;
		goto Ret;
	}
	fseek(fIn, 0, SEEK_END);
	fsize = ftello(fIn);
	fseek(fIn, 0, SEEK_SET);
	rsize = 0;
	Cnt = 0;
	
	while (!CommonData->iStop) {
		irc = fread(buf, sizeof(int), 1, fIn);
		if (irc < 0 || irc > 1) {
			CommonData->iError = 20;
			break;
		} else if (irc == 0) {
			//	check if we have file after
			if (Main->PlayFile->fFileNamesList->After(f)) {
				f = Main->PlayFile->fFileNamesList->After(f);
				Main->fStatusBar->SetText(f->GetName(), 5);
				Main->PlayProgress->SetPosition(0);
				fIn = fopen(f->GetName(), "rb");
				if (!fIn) {
					CommonData->iError = 25;
					goto Ret;
				}
				fseek(fIn, 0, SEEK_END);
				fsize = ftello(fIn);
				fseek(fIn, 0, SEEK_SET);
				rsize = 0;
				Cnt = 0;
			} else {
				tm.tv_sec = 0;		// 0.1 s
				tm.tv_usec = 100000;
				select(FD_SETSIZE, NULL, NULL, NULL, &tm);
			}
			continue;
		}

		if (head->len > BSIZE || head->len < sizeof(int)) {
			CommonData->iError = 30;
			break;
		}
		irc = fread(&buf[sizeof(int)], head->len - sizeof(int), 1, fIn);
		if (irc != 1) {
			CommonData->iError = 40;
			break;
		}
		
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
		
		Cnt++;
		if ((iNum > 0 && Cnt >= iNum) || (iNum == 0 && Cnt >= 20000)) {
			if (iNum > 0) sleep(1);
			iNum = Main->nPlayBlocks->GetIntNumber();
			if (fsize) Main->PlayProgress->SetPosition(100.0*rsize/fsize);
			Main->fStatusBar->SetText(f->GetName(), 5);
			Cnt = 0;
		}	
	}
Ret:
	Main->fStatusBar->SetText("", 5);
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

