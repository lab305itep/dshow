//		Simple data analysis and show for DANSS
//	I.G. Alekseev & D.N. Svirida, ITEP, Moscow, 2016

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
#include <TLatex.h>
#include <TLegend.h>
#include <TLine.h>
#include <TPaveStats.h>
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
#include "drate.h"

const char *DataFileTypes[] = {"Data files", "*.dat*", "All files", "*", 0, 0};
const char *SaveFileTypes[] = {"Root files", "*.root", "PDF", "*.pdf", "PostScript", "*.ps", "JPEG", "*.jpg", "GIF", "*.gif", "PNG", "*.png", "All files", "*", 0, 0};

struct common_data_struct Run;
struct channel_struct Map[MAXWFD][64];
struct common_parameters_struct Conf;

/*	Class dshowMainFrame - the main window		*/
/*	Main window Constructor				*/
dshowMainFrame::dshowMainFrame(const TGWindow *p, UInt_t w, UInt_t h) : TGMainFrame(p, w, h) 
{
	char strs[64];
	char strl[1024];
	Int_t StatusBarParts[] = {10, 10, 10, 10, 10, 50};	// status bar parts
	Double_t RGB_r[]    = {0., 0.0, 1.0, 0.0, 1.0};
   	Double_t RGB_g[]    = {0., 0.0, 0.0, 1.0, 1.0};
   	Double_t RGB_b[]    = {0., 1.0, 0.0, 0.0, 0.0};
   	Double_t RGB_stop[] = {0., .25, .50, .75, 1.0};
	int i, n;
	TGLabel *lb;
	
	// Create our data structures and histogramms
	gROOT->SetStyle("Plain");
	gStyle->SetOptStat("e");
	gStyle->SetOptFit(11);
	InitHist();

//	TLegend
	TimeLegend = new TLegend(0.65, 0.8, 0.99, 0.89);
	TimeLegend->AddEntry(Run.hTimeA[0], "All events", "L");
	TimeLegend->AddEntry(Run.hTimeB[0], "Amplitude above threshold", "L");
	TimeLegend->SetFillColor(kWhite);

	PlayFile = new TGFileInfo();
	PlayFile->fFileTypes = DataFileTypes;
	PlayFile->fIniDir = getcwd(NULL, 0);
	PlayFile->SetMultipleSelection(1);

	SaveFile = new TGFileInfo();
	SaveFile->fFileTypes = SaveFileTypes;
	SaveFile->fIniDir = getcwd(NULL, 0);
	SaveFile->SetMultipleSelection(0);

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
	CreateRateTab(tab);
	hframe->AddFrame(tab, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 5, 5, 3, 4));
	
   	TGVerticalFrame *vframe=new TGVerticalFrame(hframe, 50, h);

	Pause = new TGCheckButton(vframe,"&Pause");
   	vframe->AddFrame(Pause, new TGLayoutHints(kLHintsCenterX | kLHintsTop, 5, 5, 3, 4));

	lb = new TGLabel(vframe, "Refresh, s");
   	vframe->AddFrame(lb, new TGLayoutHints(kLHintsLeft | kLHintsTop, 5, 5, 3, 4));		
	nRefresh = new TGNumberEntry(vframe, 1, 2, -1, TGNumberFormat::kNESInteger, TGNumberFormat::kNEANonNegative, TGNumberFormat::kNELLimitMinMax, 1, 100);
   	vframe->AddFrame(nRefresh, new TGLayoutHints(kLHintsCenterX | kLHintsTop, 5, 5, 3, 4));	

	lb = new TGLabel(vframe, "WFD:");
   	vframe->AddFrame(lb, new TGLayoutHints(kLHintsLeft | kLHintsTop, 5, 5, 3, 4));		
	nWFD = new TGNumberEntry(vframe, 1, 2, -1, TGNumberFormat::kNESInteger, TGNumberFormat::kNEANonNegative, TGNumberFormat::kNELLimitMinMax, 1, MAXWFD);
	nWFD->SetIntNumber(1);
	nWFD->Connect("ValueSet(Long_t)", "dshowMainFrame", this, "ChangeWaveFormPars()");
   	vframe->AddFrame(nWFD, new TGLayoutHints(kLHintsCenterX | kLHintsTop, 5, 5, 3, 4));	

	lb = new TGLabel(vframe, "Channel:");
   	vframe->AddFrame(lb, new TGLayoutHints(kLHintsLeft | kLHintsTop, 5, 5, 3, 4));		
	nChan = new TGNumberEntry(vframe, 0, 2, -1, TGNumberFormat::kNESInteger, TGNumberFormat::kNEANonNegative, TGNumberFormat::kNELLimitMinMax, 0, 63);
	nChan->SetIntNumber(0);
	nChan->Connect("ValueSet(Long_t)", "dshowMainFrame", this, "ChangeWaveFormPars()");
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
	nPlayBlocks->Connect("ValueSet(Long_t)", "dshowMainFrame", this, "OnPlayBlocksChanged()");
   	grPlay->AddFrame(nPlayBlocks, new TGLayoutHints(kLHintsCenterX  | kLHintsTop, 5, 5, 3, 4));

	PlayProgress = new TGHProgressBar(grPlay, TGProgressBar::kStandard, 100);
	PlayProgress->ShowPosition();
	PlayProgress->SetBarColor("green");
	PlayProgress->SetRange(0.0, 100.0);
	grPlay->AddFrame(PlayProgress, new TGLayoutHints(kLHintsExpandX  | kLHintsTop, 5, 5, 3, 1));

	FileProgress = new TGHProgressBar(grPlay, TGProgressBar::kStandard, 100);
	FileProgress->ShowPosition();
	FileProgress->SetBarColor("blue");
	FileProgress->SetRange(0.0, 100.0);
	grPlay->AddFrame(FileProgress, new TGLayoutHints(kLHintsExpandX  | kLHintsTop, 5, 5, 3, 1));

	Follow = new TGCheckButton(grPlay,"&Follow");
	Follow->Connect("Clicked()", "dshowMainFrame", this, "Connect2Online()");	
   	grPlay->AddFrame(Follow, new TGLayoutHints(kLHintsCenterX | kLHintsTop, 5, 5, 3, 4));

   	vframe->AddFrame(grPlay, new TGLayoutHints(kLHintsCenterX  | kLHintsTop, 5, 5, 3, 4));

   	TGTextButton *exit = new TGTextButton(vframe,"&Exit ");
	exit->Connect("Clicked()", "dshowMainFrame", this, "SendCloseMessage()");
   	vframe->AddFrame(exit, new TGLayoutHints(kLHintsCenterX | kLHintsBottom, 5, 5, 10, 4));

   	TGTextButton *reset = new TGTextButton(vframe,"&Reset");
	reset->Connect("Clicked()", "dshowMainFrame", this, "Reset()");
   	vframe->AddFrame(reset, new TGLayoutHints(kLHintsCenterX | kLHintsBottom, 5, 5, 3, 4));

   	TGTextButton *save = new TGTextButton(vframe,"&Save");
	save->Connect("Clicked()", "dshowMainFrame", this, "SaveDialog()");
   	vframe->AddFrame(save, new TGLayoutHints(kLHintsCenterX | kLHintsBottom, 5, 5, 3, 4));

	hframe->AddFrame(vframe, new TGLayoutHints(kLHintsRight | kLHintsExpandY, 5, 5, 3, 4));

   	AddFrame(hframe, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 2, 2, 2, 2));

	fStatusBar = new TGStatusBar(this, 50, 10, kHorizontalFrame);
	fStatusBar->SetParts(StatusBarParts, sizeof(StatusBarParts) / sizeof(Int_t));
	AddFrame(fStatusBar, new TGLayoutHints(kLHintsBottom | kLHintsExpandX, 0, 0, 2, 0));

   	// Sets window name and shows the main frame
   	SetWindowName("DANSS Show. Version of " __TIMESTAMP__ ".");
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
	Run.iThreadStop = 1;
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

   	TGTextButton *save = new TGTextButton(hframe,"&Save");
	save->Connect("Clicked()", "dshowMainFrame", this, "SaveWaveForm()");
   	hframe->AddFrame(save, new TGLayoutHints(kLHintsCenterY | kLHintsLeft, 5, 5, 3, 4));

	TGButtonGroup *bg = new TGButtonGroup(hframe, "WaveForm", kHorizontalFrame);
	rWaveSelf = new TGRadioButton(bg, "&Self   ");
	rWaveTrig = new TGRadioButton(bg, "&Event  ");
	rWaveHist = new TGRadioButton(bg, "&History");
	rWaveSelf->SetState(kButtonDown);
	rWaveSelf->Connect("Clicked()", "dshowMainFrame", this, "ChangeWaveFormPars()");
	rWaveTrig->Connect("Clicked()", "dshowMainFrame", this, "ChangeWaveFormPars()");
	rWaveHist->Connect("Clicked()", "dshowMainFrame", this, "ChangeWaveFormPars()");
   	hframe->AddFrame(bg, new TGLayoutHints(kLHintsLeft | kLHintsCenterY, 5, 5, 3, 4));

	lb = new TGLabel(hframe, "Threshold:");
   	hframe->AddFrame(lb, new TGLayoutHints(kLHintsLeft | kLHintsCenterY, 5, 5, 3, 4));		
	nWaveThreshold = new TGNumberEntry(hframe, 0, 5, -1, TGNumberFormat::kNESInteger, TGNumberFormat::kNEANonNegative);
	nWaveThreshold->SetIntNumber(0);
	nWaveThreshold->Connect("ValueSet(Long_t)", "dshowMainFrame", this, "ChangeWaveFormPars()");
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

   	TGTextButton *reset = new TGTextButton(hframe,"&Reset");
	reset->Connect("Clicked()", "dshowMainFrame", this, "ResetTimeHists()");
   	hframe->AddFrame(reset, new TGLayoutHints(kLHintsCenterY | kLHintsRight, 5, 5, 3, 4));

    	me->AddFrame(hframe, new TGLayoutHints(kLHintsBottom | kLHintsExpandX, 2, 2, 2, 2));
	fTimeCFit = new TF1("fTimeCFit", "gaus(0)+pol0(3)", -100, 100);
	fTimeCFit->SetParNames("Const.", "Mean", "Sigma", "Level");
}

/*	Create event display tab				*/
void dshowMainFrame::CreateEventTab(TGTab *tab)
{
	TGLabel *lb;
	TGButtonGroup *bg;

	TGCompositeFrame *me = tab->AddTab("Event");

	fEventCanvas = new TRootEmbeddedCanvas ("EventCanvas", me, 1600, 800);
   	me->AddFrame(fEventCanvas, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 10, 10, 10, 1));

   	TGHorizontalFrame *hframe=new TGHorizontalFrame(me);

   	TGTextButton *save = new TGTextButton(hframe,"&Save");
	save->Connect("Clicked()", "dshowMainFrame", this, "SaveEvent()");
   	hframe->AddFrame(save, new TGLayoutHints(kLHintsCenterY | kLHintsLeft, 5, 5, 3, 4));

	lb = new TGLabel(hframe, "Sum Energy Threshold:");
   	hframe->AddFrame(lb, new TGLayoutHints(kLHintsLeft | kLHintsCenterY, 5, 5, 3, 4));		
	nSumEnergyThreshold = new TGNumberEntry(hframe, 0, 5, -1, TGNumberFormat::kNESRealTwo, TGNumberFormat::kNEANonNegative);
	nSumEnergyThreshold->SetNumber(1.0);
	nSumEnergyThreshold->Connect("ValueSet(Long_t)", "dshowMainFrame", this, "ChangeDisplayPars()");
   	hframe->AddFrame(nSumEnergyThreshold, new TGLayoutHints(kLHintsLeft | kLHintsCenterY, 5, 5, 3, 4));

	bg = new TGButtonGroup(hframe, "Tagged as", kHorizontalFrame);
	rEvtAll      = new TGRadioButton(bg, "&Any       ");
	rEvtNone     = new TGRadioButton(bg, "&None      ");
	rEvtVeto     = new TGRadioButton(bg, "&Veto      ");
	rEvtPositron = new TGRadioButton(bg, "&Positron  ");
	rEvtNeutron  = new TGRadioButton(bg, "&Neutron   ");
	rEvtAll->SetState(kButtonDown);
	rEvtAll->Connect("Clicked()", "dshowMainFrame", this, "ChangeDisplayPars()");
	rEvtNone->Connect("Clicked()", "dshowMainFrame", this, "ChangeDisplayPars()");
	rEvtVeto->Connect("Clicked()", "dshowMainFrame", this, "ChangeDisplayPars()");
	rEvtPositron->Connect("Clicked()", "dshowMainFrame", this, "ChangeDisplayPars()");
	rEvtNeutron->Connect("Clicked()", "dshowMainFrame", this, "ChangeDisplayPars()");
   	hframe->AddFrame(bg, new TGLayoutHints(kLHintsLeft | kLHintsCenterY, 5, 5, 3, 4));

	bg = new TGButtonGroup(hframe, "SiPM hits", kHorizontalFrame);
	rHitAll      = new TGRadioButton(bg, "&All       ");
	rHitClean    = new TGRadioButton(bg, "&Clean     ");
	rHitCluster  = new TGRadioButton(bg, "C&luster   ");
	rHitAll->SetState(kButtonDown);
   	hframe->AddFrame(bg, new TGLayoutHints(kLHintsLeft | kLHintsCenterY, 5, 5, 3, 4));

    	me->AddFrame(hframe, new TGLayoutHints(kLHintsBottom | kLHintsExpandX, 2, 2, 2, 2));
}

/*	Create event rate tab				*/
void dshowMainFrame::CreateRateTab(TGTab *tab)
{
	int i, j;
	char str[128];
	const Color_t rateColor[6] = {kRed, kGreen, kBlue, kOrange, kCyan+4, kBlack};
	
	TGCompositeFrame *me = tab->AddTab("Rates");

	fRateCanvas = new TRootEmbeddedCanvas ("RateCanvas", me, 1600, 800);
   	me->AddFrame(fRateCanvas, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 10, 10, 10, 1));

   	TGHorizontalFrame *hframe=new TGHorizontalFrame(me);

	TGButtonGroup *bg = new TGButtonGroup(hframe, "Rate range", kHorizontalFrame);
	rRateFast      = new TGRadioButton(bg, "100-&2000  ");
	rRateMedium    = new TGRadioButton(bg, "&4-200     ");
	rRateSlow      = new TGRadioButton(bg, "&0-5       ");
	rRateFast->SetState(kButtonDown);
   	hframe->AddFrame(bg, new TGLayoutHints(kLHintsLeft | kLHintsCenterY, 5, 5, 3, 4));
    	me->AddFrame(hframe, new TGLayoutHints(kLHintsBottom | kLHintsExpandX, 2, 2, 2, 2));

	hRateTemplate[0] = new TH1D("hRateTemplateA", "Trigger rates;;Hz", 10,  0, 5000);
	hRateTemplate[1] = new TH1D("hRateTemplateB", "", 10,  0, 5000);
	for (i=0; i<2; i++) {	
		hRateTemplate[i]->SetMinimum(0);
		hRateTemplate[i]->SetMaximum(2000);
		hRateTemplate[i]->GetXaxis()->SetTimeDisplay(1);
		hRateTemplate[i]->GetXaxis()->SetTimeOffset(0);
		hRateTemplate[i]->SetStats(0);
	}
	hRateTemplate[0]->GetXaxis()->SetNdivisions(605);
	hRateTemplate[1]->GetXaxis()->SetNdivisions(504);
	hRateTemplate[0]->GetXaxis()->SetTimeFormat("%d-%b %H:%M");
	hRateTemplate[1]->GetXaxis()->SetTimeFormat("%H:%M");
	RateLegend = new TLegend(0.35, 0.85, 0.75, 0.99);
	for (j=0; j<2; j++) for (i=0; i<6; i++) {
		sprintf(str, "fRateFit%d%d", j, i);
		fRateFit[j][i] = new TF1(str, "pol0");
		fRateFit[j][i]->SetLineColor(rateColor[i]);
	}
	memset(gRateGraph, 0, sizeof(gRateGraph));
	memset(pdRate, 0, sizeof(pdRate));
}

/*	Create histogramms						*/
void dshowMainFrame::InitHist(void)
{
	int i;
	char strs[128];
	char strl[1024];

	for(i=0; i<MAXWFD; i++) {
		snprintf(strs, sizeof(strs), "hAmpS%2.2d", i+1);
		snprintf(strl, sizeof(strl), "Module %2.2d: amplitude versus channels number (self)", i+1);		
		Run.hAmpS[i] = new TH2D(strs, strl, 64, 0, 64, 128, 0, 128);
		snprintf(strs, sizeof(strs), "hAmpE%2.2d", i+1);
		snprintf(strl, sizeof(strl), "Module %2.2d: amplitude versus channels number (event)", i+1);
		Run.hAmpE[i] = new TH2D(strs, strl, 64, 0, 64, 256, 0, 4096);
		snprintf(strs, sizeof(strs), "hTimeA%2.2d", i+1);
		snprintf(strl, sizeof(strl), "Module %2.2d: time versus channels number, all hits", i+1);
		Run.hTimeA[i] = new TH2D(strs, strl, 64, 0, 64, 100, 0, 512);
		snprintf(strs, sizeof(strs), "hTimeB%2.2d", i+1);
		snprintf(strl, sizeof(strl), "Module %2.2d: time versus channels number, amplitude above threshold", i+1);
		Run.hTimeB[i] = new TH2D(strs, strl, 64, 0, 64, 100, 0, 512);
		Run.hTimeB[i]->SetLineColor(kBlue);
		snprintf(strs, sizeof(strs), "hTimeC%2.2d", i+1);
		snprintf(strl, sizeof(strl), "Module %2.2d: time versus channels number relative to common SiPM time, amplitude above threshold", i+1);
		Run.hTimeC[i] = new TH2D(strs, strl, 64, 0, 64, 400, -200, 200);
	}
	Run.Rates[0] = new Drate(5L*1.0E9/NSPERCHAN, 40000);
	Run.Rates[1] = new Drate(5L*1.0E9/NSPERCHAN, 40000);
	Run.Rates[2] = new Drate(10L*1.0E9/NSPERCHAN, 20000);
	Run.Rates[3] = new Drate(10L*1.0E9/NSPERCHAN, 20000);
	Run.Rates[4] = new Drate(10L*1.0E9/NSPERCHAN, 20000);
	Run.Rates[5] = new Drate(100L*1.0E9/NSPERCHAN, 2000);
	Run.DisplayEvent->NHits = 0;
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
	TPaveStats *stat;
   	TLatex txt;
   	TLine ln;
	int mod, chan;
	int i, j;
	double h;
	TH1D *hProj;
	const char rateName[6][20] = {"Trigger", "VETO", "Positrons", "Neutrons", ">20 MeV", ">20 MeV & !VETO"};
	const int rateMarker[6] = {20, 21, 22, 23, 24, 25};
	const Color_t rateColor[6] = {kRed, kGreen, kBlue, kOrange, kCyan+4, kBlack};
	double tm;

	mod = nWFD->GetIntNumber() - 1;
	chan = nChan->GetIntNumber();

	TThread::Lock();
/*		Progress bars		*/
	PlayProgress->SetPosition(Run.inFile);
	FileProgress->SetPosition(Run.inNumber);
/*		Status bar		*/
	snprintf(str, sizeof(str), "Blocks: %d", Run.BlockCnt);
	fStatusBar->SetText(str, 1);
	snprintf(str, sizeof(str), "Events: %d", Run.EventCnt);
	fStatusBar->SetText(str, 2);
	snprintf(str, sizeof(str), "SelfTriggers: %d", Run.SelfTrigCnt);
	fStatusBar->SetText(str, 3);
	snprintf(str, sizeof(str), "Errors: %d", Run.ErrorCnt);
	fStatusBar->SetText(str, 4);
	Run.msg[sizeof(Run.msg)-1] = '\0';
	fStatusBar->SetText(Run.msg, 5);
/*		File processing status	*/

/*		Scope			*/
   	fCanvas = fWaveCanvas->GetCanvas();
   	fCanvas->cd();
	if (Run.hWaveForm) {
		Run.hWaveForm->SetMinimum(Run.hWaveForm->GetMinimum() - 2);
		Run.hWaveForm->SetMarkerSize(2);
		Run.hWaveForm->SetMarkerColor(kBlue);
		Run.hWaveForm->SetMarkerStyle(20);
		Run.hWaveForm->DrawCopy("p");
		delete Run.hWaveForm;
		Run.hWaveForm = NULL;
	}
   	fCanvas->Update();

/*		Self			*/
   	fCanvas = fSelfCanvas->GetCanvas();
   	fCanvas->cd();
	if (rSelf2D->IsOn()) {
		sprintf(str, "Module: %d. Amplitude versus channel number.", mod + 1);
		Run.hAmpS[mod]->SetTitle(str);
		Run.hAmpS[mod]->Draw("color");
	} else if (rSelfChan->IsOn()) {
		sprintf(str, "Module: %d. Channel occupancy.", mod + 1);
		Run.hAmpS[mod]->ProjectionX()->SetTitle(str);
		Run.hAmpS[mod]->ProjectionX()->Draw();	
	} else if (rSelfAll->IsOn()) {
		sprintf(str, "Module: %d. Amplitude distribution for all channels.", mod + 1);
		Run.hAmpS[mod]->ProjectionY()->SetTitle(str);
		Run.hAmpS[mod]->ProjectionY()->Draw();
	} else if (rSelfSingle->IsOn()) {
		sprintf(str, "Module: %d. Amplitude distribution for channel %d.", mod + 1, chan);
		Run.hAmpS[mod]->ProjectionY("_py", chan + 1, chan + 1)->SetTitle(str);
		Run.hAmpS[mod]->ProjectionY("_py", chan + 1, chan + 1)->Draw();
	}
   	fCanvas->Update();

/*		Spectrum			*/
   	fCanvas = fSpectrumCanvas->GetCanvas();
   	fCanvas->cd();
	if (rSpect2D->IsOn()) {
		sprintf(str, "Module: %d. Amplitude versus channel number.", mod + 1);
		Run.hAmpE[mod]->SetTitle(str);
		Run.hAmpE[mod]->Draw("color");
	} else if (rSpectChan->IsOn()) {
		sprintf(str, "Module: %d. Channel occupancy.", mod + 1);
		Run.hAmpE[mod]->ProjectionX()->SetTitle(str);
		Run.hAmpE[mod]->ProjectionX()->Draw();	
	} else if (rSpectAll->IsOn()) {
		sprintf(str, "Module: %d. Amplitude distribution for all channels.", mod + 1);
		Run.hAmpE[mod]->ProjectionY()->SetTitle(str);
		Run.hAmpE[mod]->ProjectionY()->Draw();
	} else if (rSpectSingle->IsOn()) {
		sprintf(str, "Module: %d. Amplitude distribution for channel %d.", mod + 1, chan);
		Run.hAmpE[mod]->ProjectionY("_py", chan + 1, chan + 1)->SetTitle(str);
		Run.hAmpE[mod]->ProjectionY("_py", chan + 1, chan + 1)->Draw();
	}
   	fCanvas->Update();

/*		Times				*/
   	fCanvas = fTimeCanvas->GetCanvas();
   	fCanvas->cd();
	fCanvas->Clear();
	fCanvas->Divide(2, 1);
   	fCanvas->cd(1);
	if (rTimeAll->IsOn()) {
		sprintf(str, "Module: %d. Time distribution for all channels.", mod + 1);
		Run.hTimeA[mod]->ProjectionY()->SetTitle(str);
		Run.hTimeA[mod]->ProjectionY()->Draw();
		Run.hTimeB[mod]->ProjectionY()->Draw("same");
	} else if (rTimeSingle->IsOn()) {
		sprintf(str, "Module: %d. Time distribution for channel %d.", mod + 1, chan);
		Run.hTimeA[mod]->ProjectionY("_py", chan + 1, chan + 1)->SetTitle(str);
		Run.hTimeA[mod]->ProjectionY("_py", chan + 1, chan + 1)->Draw();
		Run.hTimeB[mod]->ProjectionY("_py", chan + 1, chan + 1)->Draw("same");
	}
	TimeLegend->Draw();
   	fCanvas->cd(2);
	if (rTimeAll->IsOn()) {
		sprintf(str, "Module: %d. Time versus common SiPM time for all channels.", mod + 1);
		hProj = Run.hTimeC[mod]->ProjectionY();
	} else if (rTimeSingle->IsOn()) {
		sprintf(str, "Module: %d. Time versus common SiPM time for channel %d.", mod + 1, chan);
		hProj = Run.hTimeC[mod]->ProjectionY("_py", chan + 1, chan + 1);
	}
	hProj->SetTitle(str);
	h = hProj->GetMaximum();
	if (hProj->Integral() > 1.0) {
		if (Map[mod][0].type == TYPE_SIPM) {
			fTimeCFit->SetParameters(h, 0, 5, 0);
			hProj->Fit("fTimeCFit", "Q", "", -2*Conf.SiPmWindow, 2*Conf.SiPmWindow);
		} else {
			hProj->Fit("gaus", "Q");
		}
	} else {
		hProj->Draw();
	}
	if (Map[mod][0].type == TYPE_SIPM) {
		ln.SetLineColor(kRed);
		ln.SetLineWidth(2);
		ln.DrawLine(-Conf.SiPmWindow, 0, -Conf.SiPmWindow, h/2);
		ln.DrawLine( Conf.SiPmWindow, 0,  Conf.SiPmWindow, h/2);
	}
	ln.SetLineColor(kGreen);
	ln.SetLineWidth(2);
	ln.DrawLine(0, 0, 0, h);
	stat = (TPaveStats *) hProj->FindObject("stats");
	if (stat) {
		stat->SetX1NDC(0.6);
		stat->SetX2NDC(0.95);
		stat->SetY1NDC(0.75);
		stat->SetY2NDC(0.93);
//		stat->Draw();
	}

   	fCanvas->Update();

/*		Event display			*/
   	fCanvas = fEventCanvas->GetCanvas();
   	fCanvas->cd();
	DrawEvent(fCanvas);
   	fCanvas->Update();

/*		Rates				*/
   	fCanvas = fRateCanvas->GetCanvas();
   	fCanvas->cd();
	fCanvas->Clear();
	tm = time(NULL);
	hRateTemplate[0]->SetBins(10, tm - 200000, tm);
	hRateTemplate[1]->SetBins(10, tm - 1800, tm);
	RateLegend->Clear();
	for (i=0; i<6; i++) {
		if (gRateGraph[i]) delete gRateGraph[i];
		gRateGraph[i] = Run.Rates[i]->GetGraph();
		if (!gRateGraph[i]) continue;
		gRateGraph[i]->SetMarkerStyle(rateMarker[i]);
		gRateGraph[i]->SetMarkerColor(rateColor[i]);
	}
	if (rRateFast->IsDown()) {
		for (i=0; i<2; i++) {
			hRateTemplate[i]->SetMinimum(100);
			hRateTemplate[i]->SetMaximum(2000);
		}
		for (j=0; j<2; j++) if (gRateGraph[j]) {
			sprintf(str, "%16s: %6.1f Hz", rateName[j], Run.Rates[j]->GetLast());
			RateLegend->AddEntry(gRateGraph[j], str, "P");
		}
	} else if (rRateMedium->IsDown()) {
		for (i=0; i<2; i++) {
			hRateTemplate[i]->SetMinimum(5);
			hRateTemplate[i]->SetMaximum(200);
		}
		for (j=2; j<5; j++) if (gRateGraph[j]) {
			sprintf(str, "%16s: %6.1f Hz", rateName[j], Run.Rates[j]->GetLast());
			RateLegend->AddEntry(gRateGraph[j], str, "P");
		}
	} else {
		for (i=0; i<2; i++) {
			hRateTemplate[i]->SetMinimum(0);
			hRateTemplate[i]->SetMaximum(5);
		}
		for (j=5; j<6; j++) if (gRateGraph[j]) {
			sprintf(str, "%16s: %6.1f Hz", rateName[j], Run.Rates[j]->GetLast());
			RateLegend->AddEntry(gRateGraph[j], str, "P");
		}
	}
	
	for (j=0; j<2; j++) {
		fCanvas->cd();
		pdRate[j] = new TPad((j) ? "pdRateB" : "pdRateA", "", 0 + 0.7*j, 0, 0.7 + 0.3*j, 1.0);
		pdRate[j]->Draw();
		pdRate[j]->cd();
		pdRate[j]->SetRightMargin(0.02);

		hRateTemplate[j]->Draw();
		stat = (TPaveStats *) hRateTemplate[j]->FindObject("stats");
		if (stat) stat->SetOptFit(0);

		for (i=0; i<6; i++) if (gRateGraph[i]) {
			gRateGraph[i]->Draw("P");
//			gRateGraph[i]->Fit(fRateFit[j][i], "Q");
//			txt.SetTextColor(rateColor[i]);
//			sprintf(str, "%16s: %6.1f #pm %6.1f [%6.1f] Hz", rateName[j], fRateFit[j][i]->GetParameter(0), 
//				fRateFit[j][i]->GetParError(0), Run.Rates[j]->GetLast());
//			txt.DrawLatex(1.05*fRateFit[j][i]->GetParameter(0), hRateTemplate[j]->GetBinCenter(3), str);
		}
		if (!j) RateLegend->Draw();
	}
	
   	fCanvas->Update();

	TThread::UnLock();
}

/*	Draw a single event					*/
void dshowMainFrame::DrawEvent(TCanvas *cv)
{
	TBox PmtBox;
	TBox SipmBox;
	TGaxis ax;
	TText txt;
	int i, j, k, n;
	int cl;
	const struct vpos_struct {
		double x1;
		double y1;
		double x2;
		double y2;
	} vpos[64] = {
//		Top
		{0.51, 0.94, 0.63, 0.96}, {0.51, 0.91, 0.63, 0.93}, {0.63, 0.94, 0.75, 0.96}, {0.63, 0.91, 0.75, 0.93}, 
		{0.75, 0.94, 0.87, 0.96}, {0.75, 0.91, 0.87, 0.93}, {0.87, 0.94, 0.99, 0.96}, {0.87, 0.91, 0.99, 0.93}, 
//		Corners
		{0.45, 0.97, 0.48, 0.98}, {0.48, 0.95, 0.49, 0.98}, {-1.0, -1.0, -1.0, -1.0}, {0.45, 0.91, 0.48, 0.92},
		{0.02, 0.91, 0.05, 0.92}, {0.01, 0.91, 0.02, 0.94}, {0.01, 0.95, 0.02, 0.98}, {0.02, 0.97, 0.05, 0.98},
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
		{0.48, 0.91, 0.49, 0.94}, {-1.0, -1.0, -1.0, -1.0}, {-1.0, -1.0, -1.0, -1.0}, {-1.0, -1.0, -1.0, -1.0},
		{-1.0, -1.0, -1.0, -1.0}, {-1.0, -1.0, -1.0, -1.0}, {-1.0, -1.0, -1.0, -1.0}, {-1.0, -1.0, -1.0, -1.0},
		{-1.0, -1.0, -1.0, -1.0}, {-1.0, -1.0, -1.0, -1.0}, {-1.0, -1.0, -1.0, -1.0}, {-1.0, -1.0, -1.0, -1.0},
		{-1.0, -1.0, -1.0, -1.0}, {-1.0, -1.0, -1.0, -1.0}, {-1.0, -1.0, -1.0, -1.0}, {-1.0, -1.0, -1.0, -1.0},
		{-1.0, -1.0, -1.0, -1.0}, {-1.0, -1.0, -1.0, -1.0}, {-1.0, -1.0, -1.0, -1.0}, {-1.0, -1.0, -1.0, -1.0},
		{-1.0, -1.0, -1.0, -1.0}, {-1.0, -1.0, -1.0, -1.0}, {-1.0, -1.0, -1.0, -1.0}, {-1.0, -1.0, -1.0, -1.0},
	};
	const char *tagname[] = {"VETO ", "POSITRON ", "NEUTRON "};
	char str[64];

	if (!Run.DisplayEvent || !Run.DisplayEvent->NHits) return;
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
	PmtBox.SetLineWidth(5);
	for (n=0; n<Run.DisplayEvent->NHits; n++) if (Run.DisplayEvent->hits[n].type == TYPE_PMT) {
		i = Run.DisplayEvent->hits[n].xy;
		j = Run.DisplayEvent->hits[n].z / 2;
		k = Run.DisplayEvent->hits[n].z & 1;
		if (!k) i = 4 - i;
		cl = Run.DisplayEvent->hits[n].energy*MAXRGB/MAXPMT;
		if (cl > MAXRGB - 1) cl = MAXRGB - 1;
		PmtBox.SetLineColor(PaletteStart + cl);
		PmtBox.DrawBox(0.05 + 0.5*k + 0.08*i, 0.1 + 0.16*j, 0.13 + 0.5*k + 0.08*i, 0.26 +0.16*j);
	}
//		Draw Hits in SiPM
	SipmBox.SetFillStyle(1000);
	for (n=0; n<Run.DisplayEvent->NHits; n++) if (Run.DisplayEvent->hits[n].type == TYPE_SIPM) {
		if (rHitCluster->IsOn() && Run.DisplayEvent->hits[n].flag < 10) continue;
		if (rHitClean->IsOn() && Run.DisplayEvent->hits[n].flag < 0) continue;
		i = Run.DisplayEvent->hits[n].xy;
		j = Run.DisplayEvent->hits[n].z / 2;
		k = Run.DisplayEvent->hits[n].z & 1;
		if (!k) i = 24 - i;
		cl = Run.DisplayEvent->hits[n].energy*MAXRGB/MAXSIPM;
		if (cl > MAXRGB - 1) cl = MAXRGB - 1;
		SipmBox.SetFillColor(PaletteStart + cl);
		SipmBox.DrawBox(0.05 + 0.5*k + 0.016*i, 0.1 + 0.008*k + 0.016*j, 0.066 + 0.5*k + 0.016*i, 0.108 + 0.008*k + 0.016*j);
	}
//		Draw Hits in VETO
	for (n=0; n<Run.DisplayEvent->NHits; n++) if (Run.DisplayEvent->hits[n].type == TYPE_VETO) {
		cl = Run.DisplayEvent->hits[n].energy*MAXRGB/MAXADC;
		if (cl > MAXRGB - 1) cl = MAXRGB - 1;
		SipmBox.SetFillColor(PaletteStart + cl);
		i = Run.DisplayEvent->hits[n].xy;
		if (vpos[i].x1 < 0) continue;
		SipmBox.DrawBox(vpos[i].x1, vpos[i].y1, vpos[i].x2, vpos[i].y2);
	}
//		Draw palette
	for(i=0; i<MAXRGB; i++) {
		SipmBox.SetFillColor(PaletteStart + i);
		SipmBox.DrawBox(0.05 + 0.9*i/MAXRGB, 0.05, 0.05 + 0.9*(i+1)/MAXRGB, 0.09);
	}

	ax.DrawAxis(0.05, 0.05, 0.95, 0.05, 0.0, (double) MAXADC);
//		Draw TAG
	str[0] = '\0';
	for (i=0; i<sizeof(tagname) / sizeof(tagname[0]); i++) if (Run.DisplayEvent->Flags & (1 << i)) strcat(str, tagname[i]);
	txt.DrawText(0.07, 0.95, str);

	Run.DisplayEvent->NHits = 0;
}

/*	show file open dialog and start thread to play to play selected data file(s)	*/
void dshowMainFrame::PlayFileDialog(void)
{
	Follow->SetState(kButtonUp);
	PlayFileStop();
	new TGFileDialog(gClient->GetRoot(), this, kFDOpen, PlayFile);

	if (PlayFile->fFileNamesList && PlayFile->fFileNamesList->First()) {
		Run.iThreadStop = 0;
		DataThread = new TThread("DataThread", DataThreadFunction, (void *) PlayFile);
		DataThread->Run();
	}
}

/*	Save histogramms to pictures/root	*/
void dshowMainFrame::SaveDialog(void)
{
	char str[1024];
	char *name_beg;
	char *name_end;
	char def_end[] = "jpg";
	TFile *f;
	int i;

	new TGFileDialog(gClient->GetRoot(), this, kFDSave, SaveFile);
		
	if (SaveFile->fFilename) {
		name_beg = strdup(SaveFile->fFilename);
		name_end = strrchr(name_beg, '.');
		if (name_end) {
			*name_end = '\0';
			name_end++;
		} else {
			name_end = def_end;
		}
		TThread::Lock();
		if (!strcmp(name_end, "root")) {
			f = new TFile(SaveFile->fFilename, "RECREATE");
			if (Run.hWaveForm) Run.hWaveForm->Write();	// Waveform to show
			for (i=0; i<MAXWFD; i++) {
				Run.hAmpS[i]->Write();	// amplitude versus channel - self trigger
				Run.hAmpE[i]->Write();	// amplitude versus channel - events
				Run.hTimeA[i]->Write();	// time versus channel - events, no threshold
				Run.hTimeB[i]->Write();	// time versus channel - events, with fixed threshold
				Run.hTimeC[i]->Write();	// channel time - common SiPM time, TimeB thresholds
			}
			f->Close();
			delete f;
		} else if (!strcmp(name_end, "ps") || !strcmp(name_end, "pdf")) {
			sprintf(str, "%s[", SaveFile->fFilename);
			fWaveCanvas->GetCanvas()->Print(str, (Option_t *)name_end);
			strcpy(str, SaveFile->fFilename);
			fWaveCanvas->GetCanvas()->Print(str, (Option_t *)name_end);
			fSelfCanvas->GetCanvas()->Print(str, (Option_t *)name_end);
			fSpectrumCanvas->GetCanvas()->Print(str, (Option_t *)name_end);
			fTimeCanvas->GetCanvas()->Print(str, (Option_t *)name_end);
			fEventCanvas->GetCanvas()->Print(str, (Option_t *)name_end);
			fRateCanvas->GetCanvas()->Print(str, (Option_t *)name_end);
			sprintf(str, "%s]", SaveFile->fFilename);
			fRateCanvas->GetCanvas()->Print(str, (Option_t *)name_end);			
		} else {
			sprintf(str, "%s.wave.%s", name_beg, name_end);
			fWaveCanvas->GetCanvas()->Print(str, (Option_t *)name_end);
			sprintf(str, "%s.self.%s", name_beg, name_end);
			fSelfCanvas->GetCanvas()->Print(str, (Option_t *)name_end);
			sprintf(str, "%s.spect.%s", name_beg, name_end);
			fSpectrumCanvas->GetCanvas()->Print(str, (Option_t *)name_end);
			sprintf(str, "%s.time.%s", name_beg, name_end);
			fTimeCanvas->GetCanvas()->Print(str, (Option_t *)name_end);
			sprintf(str, "%s.event.%s", name_beg, name_end);
			fEventCanvas->GetCanvas()->Print(str, (Option_t *)name_end);
			sprintf(str, "%s.rate.%s", name_beg, name_end);
			fRateCanvas->GetCanvas()->Print(str, (Option_t *)name_end);
		}
		free(name_beg);
		TThread::UnLock();
	}
}

void dshowMainFrame::PlayFileStop(void)
{
	Run.iThreadStop = 1;
	PlayProgress->SetPosition(0.0);
	if (DataThread) DataThread->Join();
	DataThread = NULL;
}

void dshowMainFrame::SaveEvent(void)
{
	char str[1024];
	sprintf(str, "%s/event_%d.jpg", SaveFile->fIniDir, Run.DisplayEvent->EventCnt);
	fEventCanvas->GetCanvas()->Print(str);	
}

void dshowMainFrame::SaveWaveForm(void)
{
	char str[1024];
	sprintf(str, "%s/wave_%d.jpg", SaveFile->fIniDir, Run.thisWaveFormCnt);
	fWaveCanvas->GetCanvas()->Print(str);	
}


/*	Reset histogramms				*/
void dshowMainFrame::Reset(void)
{
	ResetSelfHists();
	ResetSpectrumHists();
	ResetTimeHists();
}

void dshowMainFrame::ResetSelfHists(void)
{
	int i;

	TThread::Lock();
	for (i=0; i<MAXWFD; i++) Run.hAmpS[i]->Reset();
	TThread::UnLock();
}

void dshowMainFrame::ResetSpectrumHists(void)
{
	int i;

	TThread::Lock();
	for (i=0; i<MAXWFD; i++) Run.hAmpE[i]->Reset();
	TThread::UnLock();
}

void dshowMainFrame::ResetTimeHists(void)
{
	int i;

	TThread::Lock();
	for (i=0; i<MAXWFD; i++) {
		Run.hTimeA[i]->Reset();
		Run.hTimeB[i]->Reset();
		Run.hTimeC[i]->Reset();
	}
	TThread::UnLock();
}

void dshowMainFrame::Connect2Online(void)
{
	PlayFileStop();
	if (Follow->IsDown()) {
		Run.iThreadStop = 0;
		DataThread = new TThread("DataThread", DataThreadFunction, NULL);
		DataThread->Run();
	}
}

void dshowMainFrame::OnPlayBlocksChanged(void)
{
	Run.iPlayBlocks = nPlayBlocks->GetIntNumber();
}

void dshowMainFrame::ChangeWaveFormPars(void)
{
	TThread::Lock();
	if (rWaveSelf->IsOn()) Run.WaveFormType = WAVE_SELF;
	if (rWaveTrig->IsOn()) Run.WaveFormType = WAVE_TRIG;
	if (rWaveHist->IsOn()) Run.WaveFormType = WAVE_HIST;
	Run.WaveFormChan = 100 * nWFD->GetIntNumber() + nChan->GetIntNumber(); 
	Run.WaveFormThr = nWaveThreshold->GetIntNumber();
	TThread::UnLock();
}

void dshowMainFrame::ChangeDisplayPars(void)
{
	TThread::Lock();
	if (rEvtAll->IsOn()) Run.DisplayType = DISP_ALL;
	if (rEvtNone->IsOn()) Run.DisplayType = DISP_NONE;
	if (rEvtVeto->IsOn()) Run.DisplayType = DISP_VETO;
	if (rEvtPositron->IsOn()) Run.DisplayType = DISP_POSITRON;
	if (rEvtNeutron->IsOn()) Run.DisplayType = DISP_NEUTRON;
	Run.DisplayEnergy = nSumEnergyThreshold->GetNumber();
	TThread::UnLock();	
}

int ReadConfig(const char *fname)
{
	config_t cnf;
#ifdef LIBCONFIG_VER_MAJOR
	int tmp;
#else
	long tmp;
#endif
	double dtmp, T0;
	char *stmp;
	int i, j, k, type, xy, z;
	config_setting_t *ptr_xy;
	config_setting_t *ptr_z;
	char str[1024];

	memset(Map, -1, sizeof(Map));
	config_init(&cnf);
	if (config_read_file(&cnf, fname) != CONFIG_TRUE) {
        	printf("DSHOW: Configuration error in file %s at line %d: %s\n", fname, config_error_line(&cnf), config_error_text(&cnf));
		config_destroy(&cnf);
            	return 10;
    	}

//		Parameters
//	float SiPmCoef;
	Conf.SiPmCoef = (config_lookup_float(&cnf, "Dshow.SiPmCoef", &dtmp)) ? dtmp : 1/290.0;
//	float PmtCoef;
	Conf.PmtCoef = (config_lookup_float(&cnf, "Dshow.PmtCoef", &dtmp)) ? dtmp : 1/105.0;
//	float VetoMin;
	Conf.VetoMin = (config_lookup_float(&cnf, "Dshow.VetoMin", &dtmp)) ? dtmp : 300.0;	// veto single hit minimum amplitude
//	float DVetoMin;
	Conf.DVetoMin = (config_lookup_float(&cnf, "Dshow.DVetoMin", &dtmp)) ? dtmp : 20.0;	// veto in DANSS, MeV
//	float SiPmWindow;
	Conf.SiPmWindow = (config_lookup_float(&cnf, "Dshow.SiPmWindow", &dtmp)) ? dtmp : 20.0;
//	float NeutronMin;	// Minimum neutron capture energy
	Conf.NeutronMin = (config_lookup_float(&cnf, "Dshow.NeutronMin", &dtmp)) ? dtmp : 3.0;
//	float PositronMin;	// Minimum cluster energy
	Conf.PositronMin = (config_lookup_float(&cnf, "Dshow.PositronMin", &dtmp)) ? dtmp : 1.0;
//	float NeutronMax;	// Maximum neutron capture energy
	Conf.NeutronMax = (config_lookup_float(&cnf, "Dshow.NeutronMax", &dtmp)) ? dtmp : 12.0;
//	float PositronMax;	// Maximum cluster energy
	Conf.PositronMax = (config_lookup_float(&cnf, "Dshow.PositronMax", &dtmp)) ? dtmp : 12.0;
//	float MaxOffClusterEnergy;	// Maximum off-cluster energy
	Conf.MaxOffClusterEnergy = (config_lookup_float(&cnf, "Dshow.MaxOffClusterEnergy", &dtmp)) ? dtmp : 2.0;
//	int TimeBThreshold;	// amplitude threshold for TimeB histogram
	Conf.TimeBThreshold = (config_lookup_int(&cnf, "Dshow.TimeBThreshold", &tmp)) ? tmp : 50;
//	float FineTimeThreshold;	// fine time calculation
	Conf.FineTimeThreshold = (config_lookup_float(&cnf, "Dshow.FineTimeThreshold", &dtmp)) ? dtmp : 0.25;
//	float SmallSiPmAmp;	// small SiPm ~ 1 pixel
	Conf.SmallSiPmAmp = (config_lookup_float(&cnf, "Dshow.SmallSiPmAmp", &dtmp)) ? dtmp : 0.05;
//	int ServerPort;			// Out port 0xB230
	Conf.ServerPort = (config_lookup_int(&cnf, "Sink.Port", &tmp)) ? tmp : 0xB230;
//	char ServerName[128];		// The server host name
	strncpy(Conf.ServerName, (config_lookup_string(&cnf, "Sink.Name", (const char **) &stmp)) ? stmp : "dserver.danss.local", sizeof(Conf.ServerName));

//		DANSS map	
	for(i=0; i<MAXWFD; i++) { 
	//	int Type;
		sprintf(str, "Map.Dev%3.3d.Type", i+1);
		type = (config_lookup_int(&cnf, str, &tmp)) ? tmp : -1;
		sprintf(str, "Map.Dev%3.3d.T0", i+1);
		T0 = (config_lookup_float(&cnf, str, &dtmp)) ? dtmp : 0.0;
		if (type < 0) continue;
		sprintf(str, "Map.Dev%3.3d.XY", i+1);
		ptr_xy = config_lookup(&cnf, str);
		sprintf(str, "Map.Dev%3.3d.Z", i+1);
		ptr_z = config_lookup(&cnf, str);
		for (j=0; j<64; j++) {
			Map[i][j].type = type;
			Map[i][j].T0 = T0;
		}
		switch(type) {
		case TYPE_SIPM:
			// SiPM map: Z: even - Y, odd - X, Z = 0..99, XY=0..24
			for (j=0; j<64; j++) Map[i][j].Coef = Conf.SiPmCoef;
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
			// PMT map: Z: even - Y, odd - X, Z = 0..9, XY=0..4
			for (j=0; j<64; j++) Map[i][j].Coef = Conf.PmtCoef;
			if (!ptr_xy || !ptr_z) break;
			for (j=0; j<64; j++) {
				Map[i][j].xy = config_setting_get_int_elem(ptr_xy, j);
				Map[i][j].z = config_setting_get_int_elem(ptr_z, j);
			}
			break;
		case TYPE_VETO:
			for (j=0; j<64; j++) {
				Map[i][j].Coef = 1;	// we sum amplitude...
				Map[i][j].xy = j;
				Map[i][j].z = 0;
			}			
			break;
		}
	}
	
	config_destroy(&cnf);
	return 0;
}

/*	the main						*/
int main(int argc, char **argv) 
{
	int irc;
	
	irc = 100;
	memset(&Run, 0, sizeof(Run));
	memset(&Conf, 0, sizeof(Conf));
	Run.Event = (struct event_struct *) malloc (sizeof(struct event_struct));
	Run.DisplayEvent = (struct event_struct *) malloc (sizeof(struct event_struct));
	if (!Run.Event || !Run.DisplayEvent) {
		printf("FATAL: No memory !\n");
		return irc;
	}
	memset(Run.Event, 0, sizeof(struct event_struct));
	memset(Run.DisplayEvent, 0, sizeof(struct event_struct));
	Run.Event->hits = (struct hit_struct *) malloc (sizeof(struct hit_struct) * MAXHITS);
	Run.DisplayEvent->hits = (struct hit_struct *) malloc (sizeof(struct hit_struct) * MAXHITS);
	if (!Run.Event->hits || !Run.DisplayEvent->hits) {
		printf("FATAL: No memory !\n");
		free(Run.Event);
		free(Run.DisplayEvent);
		return irc;
	}

	irc = ReadConfig((argc > 1) ? argv[1] : DEFCONFIG);
	if (irc) {
		free(Run.Event->hits);
		free(Run.DisplayEvent->hits);
		free(Run.Event);
		free(Run.DisplayEvent);
		return irc;
	}
   	TApplication theApp("DANSS_SHOW", &argc, argv);
   	dshowMainFrame *frame = new dshowMainFrame(gClient->GetRoot(), 2000, 1000);
   	theApp.Run();

	free(Run.Event->hits);
	free(Run.DisplayEvent->hits);
	free(Run.Event);
	free(Run.DisplayEvent);
   	return 0;
}

