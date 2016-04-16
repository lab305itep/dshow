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
#include <TLegend.h>
#include <TLine.h>
#include <TROOT.h>
#include <TRootEmbeddedCanvas.h>
#include <TString.h>
#include <TStyle.h>
#include <TSystem.h>
#include <TText.h>
#include <TThread.h>
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

const char *DataFileTypes[] = {"Data files", "*.dat*", "All files", "*", 0, 0};
const char *SaveFileTypes[] = {"Root files", "*.root", "PDF", "*.pdf", "PostScript", "*.ps", "JPEG", "*.jpg", "GIF", "*.gif", "PNG", "*.png", "All files", "*", 0, 0};
const char *RateName[] = {"All", "NO veto"};
const char *CutName[] = {"NONE", "VETO", "POSITRON", "NEUTRON", "NOSITIME", "NONX", "NONY", "NONZ", "NMULT", "NENERGY", "NFRACT", "PMULT", "PENERGY", "PFRACT"};

/*	Class dshowMainFrame - the main window		*/
/*	Main window Constructor				*/
dshowMainFrame::dshowMainFrame(const TGWindow *p, UInt_t w, UInt_t h, const char *cfgname) : TGMainFrame(p, w, h) 
{
	char strs[64];
	char strl[1024];
	Int_t StatusBarParts[] = {10, 10, 10, 10, 10, 50};	// status bar parts
	Double_t RGB_r[]    = {0., 0.0, 1.0, 1.0, 1.0};
   	Double_t RGB_g[]    = {0., 0.0, 0.0, 1.0, 1.0};
   	Double_t RGB_b[]    = {0., 1.0, 0.0, 0.0, 1.0};
   	Double_t RGB_stop[] = {0., .25, .50, .75, 1.0};
	int i, n;
	TGLabel *lb;
	
	ReadConfig(cfgname);
	// Create our data structures and histogramms
	CommonData = (struct common_data_struct *) malloc(sizeof(struct common_data_struct));
	memset(CommonData, 0, sizeof(struct common_data_struct));
	Event = NULL;
	EventLength = 0;
	CleanEvent = NULL;
	CleanLength = 0;
	Recent[0].gTime = -ONESECOND;
	Recent[1].gTime = -ONESECOND;
	Recent[2].gTime = -ONESECOND;

	gROOT->SetStyle("Plain");
	gStyle->SetOptStat("e");
	gStyle->SetOptFit(11);
	for(i=0; i<MAXWFD; i++) {
		snprintf(strs, sizeof(strs), "hAmpS%2.2d", i+1);
		snprintf(strl, sizeof(strl), "Module %2.2d: amplitude versus channels number (self)", i+1);		
		CommonData->hAmpS[i] = new TH2D(strs, strl, 64, 0, 64, 128, 0, 128);
		snprintf(strs, sizeof(strs), "hAmpE%2.2d", i+1);
		snprintf(strl, sizeof(strl), "Module %2.2d: amplitude versus channels number (event)", i+1);
		CommonData->hAmpE[i] = new TH2D(strs, strl, 64, 0, 64, 248, 128, 4096);
		snprintf(strs, sizeof(strs), "hTimeA%2.2d", i+1);
		snprintf(strl, sizeof(strl), "Module %2.2d: time versus channels number, all hits", i+1);
		CommonData->hTimeA[i] = new TH2D(strs, strl, 64, 0, 64, 100, 0, 512);
		snprintf(strs, sizeof(strs), "hTimeB%2.2d", i+1);
		snprintf(strl, sizeof(strl), "Module %2.2d: time versus channels number, amplitude above threshold", i+1);
		CommonData->hTimeB[i] = new TH2D(strs, strl, 64, 0, 64, 100, 0, 512);
		CommonData->hTimeB[i]->SetLineColor(kBlue);
		snprintf(strs, sizeof(strs), "hTimeC%2.2d", i+1);
		snprintf(strl, sizeof(strl), "Module %2.2d: time versus channels number relative to common SiPM time, amplitude above threshold", i+1);
		CommonData->hTimeC[i] = new TH2D(strs, strl, 64, 0, 64, 400, -200, 200);
	}
//	TLegend
	TimeLegend = new TLegend(0.65, 0.8, 0.99, 0.89);
	TimeLegend->AddEntry(CommonData->hTimeA[0], "All events", "L");
	TimeLegend->AddEntry(CommonData->hTimeB[0], "Amplitude above threshold", "L");
	TimeLegend->SetFillColor(kWhite);

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
	SummaLegend = new TLegend(0.6, 0.75, 0.9, 0.9);
	SummaLegend->AddEntry(CommonData->hSiPMsum[0], "All events", "L");
	SummaLegend->AddEntry(CommonData->hSiPMsum[1], "NO veto", "L");
	SummaLegend->SetFillColor(kWhite);
//	Cuts
	n = sizeof(CutName) / sizeof(CutName[0]);
	CommonData->hCuts = new TH1D("hCuts", "Various cuts", n, 0, n);
	for (i=0; i<n; i++) CommonData->hCuts->GetXaxis()->SetBinLabel(i+1, CutName[i]);
//	Taged events distributions
	CommonData->hTagEnergy[0] = new TH1D("hEPositron", "Energy distribution of positron-like events;MeV", 100, 0, 10);
	CommonData->hTagEnergy[1] = new TH1D("hENeutron", "Energy distribution of neutron-like events;MeV", 100, 0, 10);
	CommonData->hTagXY[0] = new TH2D("hXYPositron", "XY-distribution of positron-like events;cm;cm", 25, 0, 100, 25, 0, 100);
	CommonData->hTagXY[1] = new TH2D("hXYNeutron", "XY-distribution of neutron-like events;cm;cm", 25, 0, 100, 25, 0, 100);
	CommonData->hTagZ[0] = new TH1D("hZPositron", "Z-distribution of positron-like events;cm", 100, 0, 100);
	CommonData->hTagZ[1] = new TH1D("hZNeutron", "Z-distribution of neutron-like events;cm", 100, 0, 100);	
//	Rate
	CommonData->hRate = new TH1D("hRate", "Trigger rate, Hz;s", RATELEN, -RATEPER * RATELEN, 0);
	CommonData->hRate->SetStats(0);
	CommonData->hRate->SetMarkerStyle(20);
	CommonData->hRate->SetMarkerColor(kBlue);
	CommonData->fRate = (float *) malloc(RATELEN * sizeof(float));
	memset(CommonData->fRate, 0, RATELEN * sizeof(float));
	CommonData->iRatePos = 0;
	CommonData->gLastTime = -1;
//	Neutrino events
	CommonData->hNeutrinoEnergy[0] = new TH1D("hEPositronN", "Energy distribution of positrons in neutrino-like events;MeV", 100, 0, 10);
	CommonData->hNeutrinoEnergy[1] = new TH1D("hENeutronN", "Energy distribution of neutron capture in neutrino-like events;MeV", 100, 0, 10);
	CommonData->hCaptureTime = new TH1D("hCaptureTime", "Time distribution of neutron capture in neutrino-like events;us", 25, 0, 50);
	CommonData->hNeutronPath = new TH1D("hNeutronPath", "Distance from positron cluster to neutron cluster;cm", 50, 0, 100);
//	Muon capture events
	CommonData->hMesoEnergy[0] = new TH1D("hEPositronM", "Energy distribution of positrons in mesoatom-like events;MeV", 100, 0, 10);
	CommonData->hMesoEnergy[1] = new TH1D("hENeutronM", "Energy distribution of neutron capture in mesoatom-like events;MeV", 100, 0, 10);
	CommonData->hMesoTime[0] = new TH1D("hMesoTime", "Time distribution of mesoatom decay in mesoatom-like events;us", 25, 0, 5);
	CommonData->hMesoTime[1] = new TH1D("hCaptureTimeM", "Time distribution of neutron capture in mesoatom-like events;us", 25, 0, 50);

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
	CreateSummaTab(tab);
	CreateRateTab(tab);
	CreateTagTab(tab);
	CreateNeutrinoTab(tab);
	CreateMuonTab(tab);
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

   	TGTextButton *save = new TGTextButton(hframe,"&Save");
	save->Connect("Clicked()", "dshowMainFrame", this, "SaveWaveForm()");
   	hframe->AddFrame(save, new TGLayoutHints(kLHintsCenterY | kLHintsLeft, 5, 5, 3, 4));

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
	CommonData->TimeBThreshold = 50;
	nTimeBThreshold->SetIntNumber(CommonData->TimeBThreshold);
	nTimeBThreshold->Connect("ValueSet(Long_t)", "dshowMainFrame", this, "ChangeTimeBThr()");
   	hframe->AddFrame(nTimeBThreshold, new TGLayoutHints(kLHintsLeft  | kLHintsCenterY, 5, 5, 3, 4));

	lb = new TGLabel(hframe, "SiPM +- Time window, ns:");
   	hframe->AddFrame(lb, new TGLayoutHints(kLHintsLeft | kLHintsCenterY, 15, 5, 3, 4));
	nSiPMWindow = new TGNumberEntry(hframe, 0, 5, -1, TGNumberFormat::kNESRealTwo, TGNumberFormat::kNEANonNegative);
	CommonData->SiPMWindow = 20;	// ns
	nSiPMWindow->SetNumber(CommonData->SiPMWindow);
	nSiPMWindow->Connect("ValueSet(Long_t)", "dshowMainFrame", this, "ChangeSummaPars()");
   	hframe->AddFrame(nSiPMWindow, new TGLayoutHints(kLHintsLeft  | kLHintsCenterY, 5, 5, 3, 4));

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

   	TGTextButton *save = new TGTextButton(hframe,"&Save");
	save->Connect("Clicked()", "dshowMainFrame", this, "SaveEvent()");
   	hframe->AddFrame(save, new TGLayoutHints(kLHintsCenterY | kLHintsLeft, 5, 5, 3, 4));

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

	TGButtonGroup *bg = new TGButtonGroup(hframe, "Tagged as", kHorizontalFrame);
	rEvtAll      = new TGRadioButton(bg, "&Any       ");
	rEvtNone     = new TGRadioButton(bg, "&None      ");
	rEvtVeto     = new TGRadioButton(bg, "&Veto      ");
	rEvtPositron = new TGRadioButton(bg, "&Positron  ");
	rEvtNeutron  = new TGRadioButton(bg, "&Neutron   ");
	rEvtAll->SetState(kButtonDown);
   	hframe->AddFrame(bg, new TGLayoutHints(kLHintsLeft | kLHintsCenterY, 5, 5, 3, 4));

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
	SummaSiPMThreshold->Connect("ValueSet(Long_t)", "dshowMainFrame", this, "ChangeSummaPars()");
   	hframe->AddFrame(SummaSiPMThreshold, new TGLayoutHints(kLHintsLeft | kLHintsCenterY, 5, 5, 3, 4));

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

/*	Create Tagged event parameters tab				*/
void dshowMainFrame::CreateTagTab(TGTab *tab)
{
	TGCompositeFrame *me = tab->AddTab("Positron & Neutron");

	fTagCanvas = new TRootEmbeddedCanvas ("TagCanvas", me, 1600, 800);
   	me->AddFrame(fTagCanvas, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 10, 10, 10, 1));

   	TGHorizontalFrame *hframe=new TGHorizontalFrame(me);
   	TGTextButton *reset = new TGTextButton(hframe,"&Reset");
	reset->Connect("Clicked()", "dshowMainFrame", this, "ResetTagHists()");
   	hframe->AddFrame(reset, new TGLayoutHints(kLHintsCenterY | kLHintsRight, 5, 5, 3, 4));

    	me->AddFrame(hframe, new TGLayoutHints(kLHintsBottom | kLHintsExpandX, 2, 2, 2, 2));
}

/*	Create Neutrino event parameters tab				*/
void dshowMainFrame::CreateNeutrinoTab(TGTab *tab)
{
	TGCompositeFrame *me = tab->AddTab("Neutrino");

	fNeutrinoCanvas = new TRootEmbeddedCanvas ("NeutrinoCanvas", me, 1600, 800);
   	me->AddFrame(fNeutrinoCanvas, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 10, 10, 10, 1));

   	TGHorizontalFrame *hframe=new TGHorizontalFrame(me);
   	TGTextButton *reset = new TGTextButton(hframe,"&Reset");
	reset->Connect("Clicked()", "dshowMainFrame", this, "ResetNeutrinoHists()");
   	hframe->AddFrame(reset, new TGLayoutHints(kLHintsCenterY | kLHintsRight, 5, 5, 3, 4));

    	me->AddFrame(hframe, new TGLayoutHints(kLHintsBottom | kLHintsExpandX, 2, 2, 2, 2));
}

/*	Create Mesoatom event parameters tab				*/
void dshowMainFrame::CreateMuonTab(TGTab *tab)
{
	TGCompositeFrame *me = tab->AddTab("Mesoatom");

	fMuonCanvas = new TRootEmbeddedCanvas ("MuonCanvas", me, 1600, 800);
   	me->AddFrame(fMuonCanvas, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 10, 10, 10, 1));

   	TGHorizontalFrame *hframe=new TGHorizontalFrame(me);
   	TGTextButton *reset = new TGTextButton(hframe,"&Reset");
	reset->Connect("Clicked()", "dshowMainFrame", this, "ResetMuonHists()");
   	hframe->AddFrame(reset, new TGLayoutHints(kLHintsCenterY | kLHintsRight, 5, 5, 3, 4));

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
   	TText txt;
   	TLine ln;
	int mod, chan;
	int i, j;
	double h;
	TH1D *hProj;

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
		CommonData->hWaveForm->SetMinimum(CommonData->hWaveForm->GetMinimum() - 2);
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
	fCanvas->Clear();
	fCanvas->Divide(2, 1);
   	fCanvas->cd(1);
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
   	fCanvas->cd(2);
	if (rTimeAll->IsOn()) {
		sprintf(str, "Module: %d. Time versus common SiPM time for all channels.", mod + 1);
		hProj = CommonData->hTimeC[mod]->ProjectionY();
	} else if (rTimeSingle->IsOn()) {
		sprintf(str, "Module: %d. Time versus common SiPM time for channel %d.", mod + 1, chan);
		hProj = CommonData->hTimeC[mod]->ProjectionY("_py", chan + 1, chan + 1);
	}
	hProj->SetTitle(str);
	if (hProj->Integral() > 1.0) {
		hProj->Fit("gaus", "Q");
	} else {
		hProj->Draw();
	}
	h = hProj->GetMaximum() / 2;
	if (Map[mod][0].type == TYPE_SIPM) {
		ln.SetLineColor(kRed);
		ln.SetLineWidth(2);
		ln.DrawLine(-CommonData->SiPMWindow, 0, -CommonData->SiPMWindow, h);
		ln.DrawLine( CommonData->SiPMWindow, 0,  CommonData->SiPMWindow, h);
	}
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
	SummaLegend->Draw();	
//	cuts
   	fCanvas->cd(6);
	CommonData->hCuts->Draw();
   	fCanvas->Update();
   	
/*		Rates				*/
   	fCanvas = fRateCanvas->GetCanvas();
   	fCanvas->cd();
	fCanvas->Clear();
	for (j=0; j<RATELEN; j++) CommonData->hRate->SetBinContent(j+1, CommonData->fRate[(CommonData->iRatePos + j) % RATELEN]);
   	if (CommonData->hRate->Integral() > 1) {
		CommonData->hRate->Fit("pol0", "Q", "p");
		sprintf(str, "%6.1f Hz", CommonData->hRate->GetFunction("pol0")->GetParameter(0));
		txt.DrawTextNDC(0.75, 0.91, str);
	} else {
		CommonData->hRate->Draw("p");
	}
   	fCanvas->Update();

/*		Tagged events			*/
   	fCanvas = fTagCanvas->GetCanvas();
   	fCanvas->cd();
	fCanvas->Clear();
	fCanvas->Divide(3, 2);
	for (i=0; i<2; i++) {
	   	fCanvas->cd(3 * i + 1);
		CommonData->hTagEnergy[i]->Draw();
	   	fCanvas->cd(3 * i + 2);
		CommonData->hTagXY[i]->Draw("color");
	   	fCanvas->cd(3 * i + 3);
		CommonData->hTagZ[i]->Draw();
	}
   	fCanvas->Update();

/*		Neutrino events			*/
   	fCanvas = fNeutrinoCanvas->GetCanvas();
   	fCanvas->cd();
	fCanvas->Clear();
	fCanvas->Divide(2,2);
	fCanvas->cd(1);
	CommonData->hNeutrinoEnergy[0]->Draw();
	fCanvas->cd(2);
	CommonData->hNeutrinoEnergy[1]->Draw();
	fCanvas->cd(3);
	CommonData->hCaptureTime->Draw();
	fCanvas->cd(4);
	CommonData->hNeutronPath->Draw();
   	fCanvas->Update();

/*		Mesoatom events			*/
   	fCanvas = fMuonCanvas->GetCanvas();
   	fCanvas->cd();
	fCanvas->Clear();
	fCanvas->Divide(2, 2);
	for (i=0; i<2; i++) {
	   	fCanvas->cd(2 * i + 1);
		CommonData->hMesoEnergy[i]->Draw();
	   	fCanvas->cd(2* i + 2);
		CommonData->hMesoTime[i]->Draw();
	}
   	fCanvas->Update();

	TThread::UnLock();
}

/*	Draw a singlwe event					*/
void dshowMainFrame::DrawEvent(TCanvas *cv)
{
	TBox PmtBox;
	TBox SipmBox;
	TGaxis ax;
	TText txt;
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
	const char *tagname[] = {"NONE", "VETO", "POSITRON", "NEUTRON"};

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
//		Draw TAG
	txt.DrawText(0.2, 0.95, tagname[CommonData->EventTag]);

	CommonData->EventHits = 0;
}

/*	show file open dialog and start thread to play to play selected data file(s)	*/
void dshowMainFrame::PlayFileDialog(void)
{
	Follow->SetState(kButtonUp);
	PlayFileStop();
	new TGFileDialog(gClient->GetRoot(), this, kFDOpen, PlayFile);

	if (PlayFile->fFileNamesList && PlayFile->fFileNamesList->First()) {
		CommonData->iStop = 0;
		DataThread = new TThread("DataThread", DataThreadFunction, (void *) this);
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
			if (CommonData->hWaveForm) CommonData->hWaveForm->Write();	// Waveform to show
			for (i=0; i<MAXWFD; i++) {
				CommonData->hAmpS[i]->Write();	// amplitude versus channel - self trigger
				CommonData->hAmpE[i]->Write();	// amplitude versus channel - events
				CommonData->hTimeA[i]->Write();	// time versus channel - events, no threshold
				CommonData->hTimeB[i]->Write();	// time versus channel - events, with fixed threshold
				CommonData->hTimeC[i]->Write();	// channel time - common SiPM time, TimeB thresholds
			}
			for(i=0; i<2; i++) {
				CommonData->hSiPMsum[i]->Write();	// SiPM summa, all/no veto
				CommonData->hSiPMhits[i]->Write();	// SiPM Hits, all/no veto
				CommonData->hPMTsum[i]->Write();	// PMT summa, all/no veto
				CommonData->hPMThits[i]->Write();	// PMT Hits, all/no veto
				CommonData->hSPRatio[i]->Write();	// SiPM sum to PTMT sum ratio, all/no veto
				CommonData->hTagEnergy[i]->Write();	// Computed enegry for tagged events
				CommonData->hTagXY[i]->Write();	// XY-distribution for tagged events
				CommonData->hTagZ[i]->Write();	// Z-distribution for tagged events
				CommonData->hNeutrinoEnergy[i]->Write();	// Selected events positron and neutron energy
				CommonData->hMesoEnergy[i]->Write();	// Selected meso-events positron and neutron energy
				CommonData->hMesoTime[i]->Write();	// Meso-events meso-decay and neutron capture time
			}
			CommonData->hCaptureTime->Write();	// Neutron decay time
			CommonData->hNeutronPath->Write();	// distance from positron to neutron
			CommonData->hCuts->Write();		// Cut criteria
			CommonData->hRate->Write();		// Rate
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
			fSummaCanvas->GetCanvas()->Print(str, (Option_t *)name_end);
			fRateCanvas->GetCanvas()->Print(str, (Option_t *)name_end);
			fTagCanvas->GetCanvas()->Print(str, (Option_t *)name_end);
			fNeutrinoCanvas->GetCanvas()->Print(str, (Option_t *)name_end);
			fMuonCanvas->GetCanvas()->Print(str, (Option_t *)name_end);
			sprintf(str, "%s]", SaveFile->fFilename);
			fMuonCanvas->GetCanvas()->Print(str, (Option_t *)name_end);			
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
			sprintf(str, "%s.sum.%s", name_beg, name_end);
			fSummaCanvas->GetCanvas()->Print(str, (Option_t *)name_end);
			sprintf(str, "%s.rate.%s", name_beg, name_end);
			fRateCanvas->GetCanvas()->Print(str, (Option_t *)name_end);
			sprintf(str, "%s.tag.%s", name_beg, name_end);
			fTagCanvas->GetCanvas()->Print(str, (Option_t *)name_end);
			sprintf(str, "%s.neut.%s", name_beg, name_end);
			fNeutrinoCanvas->GetCanvas()->Print(str, (Option_t *)name_end);
			sprintf(str, "%s.muon.%s", name_beg, name_end);
			fMuonCanvas->GetCanvas()->Print(str, (Option_t *)name_end);
		}
		free(name_beg);
		TThread::UnLock();
	}
}

void dshowMainFrame::PlayFileStop(void)
{
	CommonData->iStop = 1;
	PlayProgress->SetPosition(0.0);
	if (DataThread) DataThread->Join();
	DataThread = NULL;
}

void dshowMainFrame::SaveEvent(void)
{
	char str[1024];
	sprintf(str, "%s/event_%d.jpg", SaveFile->fIniDir, CommonData->thisEventCnt);
	fEventCanvas->GetCanvas()->Print(str);	
}

void dshowMainFrame::SaveWaveForm(void)
{
	char str[1024];
	sprintf(str, "%s/wave_%d.jpg", SaveFile->fIniDir, CommonData->thisWaveFormCnt);
	fWaveCanvas->GetCanvas()->Print(str);	
}

/*	Process an event					*/
void dshowMainFrame::ProcessEvent(char *data)
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
	int nSiPM, nSiPMc, nPMT, nVeto;
	long long gTime;
	int gTrig;
	int nHits;
	int cHits;
	float dtM, dtN, dr;

	sumPMT = 0;
	sumSiPM = 0;
	nSiPM = 0;
	sumSiPMc = 0;
	nSiPMc = 0;
	nPMT = 0;
	nVeto = 0;
	gTime = -1;
	nHits = 0;
	cHits = 0;

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
			t = NSPERCHAN * (FindHalfTime(&rec->d[1], rec->len - 2, amp) - rec->d[0] / 6.0);
			if (!CommonData->hWaveForm && rWaveTrig->IsOn() && nWFD->GetIntNumber() == mod && nChan->GetIntNumber() == chan 
				&& amp > nWaveThreshold->GetIntNumber()) {
				sprintf(strl, "Event WaveForm for %d.%2.2d", mod, chan);
				CommonData->hWaveForm = new TH1D("hWave", strl, rec->len - 2, 0, NSPERCHAN * (rec->len - 2));
				CommonData->hWaveForm->SetStats(0);
				for (i = 0; i < rec->len - 2; i++) CommonData->hWaveForm->SetBinContent(i + 1, (double) rec->d[i+1]);
				CommonData->thisWaveFormCnt = CommonData->EventCnt;
			}
			CommonData->hAmpE[mod-1]->Fill((double)chan, (double)amp);
			CommonData->hTimeA[mod-1]->Fill((double)chan, t);
			if (amp > CommonData->TimeBThreshold) CommonData->hTimeB[mod-1]->Fill((double)chan, t);
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
//		Clean the event
	if (CleanLength < EventLength) {
		ptr = realloc(CleanEvent, EventLength * sizeof(struct evt_disp_struct));
		if (!ptr) return;
		CleanEvent = (struct evt_disp_struct *) ptr;
		CleanLength = EventLength;
	}
	for (i=0; i<nHits; i++) {
		if (Event[i].type == TYPE_SIPM && ((!nat) || fabs(Event[i].time - at) > CommonData->SiPMWindow)) continue;
		memcpy(&CleanEvent[cHits], &Event[i], sizeof(struct evt_disp_struct));
		cHits++;
	}
	CalculateTags(cHits, gTime);
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
		case TYPE_VETO:
			nVeto++;
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
//		Copy event for drawing
	if ((!CommonData->EventHits) && sumPMT >= nPMTSumThreshold->GetIntNumber() && sumSiPM >= nSiPMSumThreshold->GetIntNumber() && 
		(rEvtAll->IsOn() || (rEvtNone->IsOn() && EventTag == TAG_NONE) || (rEvtVeto->IsOn() && EventTag == TAG_VETO) 
		|| (rEvtPositron->IsOn() && EventTag == TAG_POSITRON) || (rEvtNeutron->IsOn() && EventTag == TAG_NEUTRON))) {
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
		CommonData->EventTag = EventTag;
		CommonData->EventEnergy = EventEnergy;
		CommonData->thisEventCnt = CommonData->EventCnt;
	}
//		try to get time correlations
	if (EventTag == TAG_NEUTRON) {
		dtM = (Recent[1].gTime - Recent[0].gTime) * NSPERCHAN / 1000.0;	// to us
		dtN = (Recent[2].gTime - Recent[1].gTime) * NSPERCHAN / 1000.0;	// to us
		dr = 0;
		for (i=0; i<3; i++) if (Recent[2].Vertex[i] >= 0 && Recent[1].Vertex[i] >= 0) dr += (Recent[2].Vertex[i] - Recent[1].Vertex[i]) * (Recent[2].Vertex[i] - Recent[1].Vertex[i]);
		dr = sqrt(dr);
		if (dtN < Pars.tNeutronCapture && dr < Pars.NeutronPath) {
			if (dtM > Pars.tMesoDecay)  {	// Neutrino
				CommonData->hNeutrinoEnergy[0]->Fill(Recent[1].Energy);
				CommonData->hNeutrinoEnergy[1]->Fill(Recent[2].Energy);
				CommonData->hCaptureTime->Fill(dtN);
			} else if (dtM > 0 && dtM < Pars.tMesoDecay) {	// Mesoatom
				CommonData->hMesoEnergy[0]->Fill(Recent[1].Energy);
				CommonData->hMesoEnergy[1]->Fill(Recent[2].Energy);
				CommonData->hMesoTime[0]->Fill(dtM);
				CommonData->hMesoTime[1]->Fill(dtN);
			}
			CommonData->hNeutronPath->Fill(dr);
			Recent[0].gTime = -ONESECOND;
			Recent[1].gTime = -ONESECOND;
			Recent[2].gTime = -ONESECOND;
		}
	}
}

/*	Check event for veto/positron/neutron signature		*/
void dshowMainFrame::CalculateTags(int cHits, long long gtime)
{
	float xsum, exsum, ysum, eysum, zsum;
	int ncHits;
	float eall, eclust, r2, xy0;
	float E;
	int i, j;

	eall = 0;
	for (i=0; i<cHits; i++) if (CleanEvent[i].type == TYPE_SIPM) eall += CleanEvent[i].amp;
	eall /= SIPM2MEV;

	EventTag = TAG_NONE;
	EventEnergy = eall;

//	VETO
	for (i=0; i<cHits; i++) if (CleanEvent[i].type == TYPE_VETO) {
		EventTag = TAG_VETO;
		Recent[0].gTime = gtime;
		Recent[0].Energy = eall;
		// don't fill vertex
		CommonData->hCuts->Fill(CUT_VETO);
		return;
	}
//	Check for neutron
//		Calculate parameters
//		Energy center
	xsum = 0;
	exsum = 0;
	ysum = 0;
	eysum = 0;
	zsum = 0;
	
	for (i=0; i<cHits; i++) if (CleanEvent[i].type == TYPE_SIPM && CleanEvent[i].amp > Pars.nHitMin * SIPM2MEV) {
		if (CleanEvent[i].z & 1) {	// X
			exsum += CleanEvent[i].amp;
			xsum += CleanEvent[i].amp * CleanEvent[i].xy;
		} else {			// Y
			eysum += CleanEvent[i].amp;
			ysum += CleanEvent[i].amp * CleanEvent[i].xy;
		}
		zsum += CleanEvent[i].amp * CleanEvent[i].z;
	}
	if (exsum > 0) {
		xsum /= exsum;
	} else {
		xsum = -1;
		CommonData->hCuts->Fill(CUT_NONX);
	}
	if (eysum > 0) {
		ysum /= eysum;
	} else {
		ysum = -1;
		CommonData->hCuts->Fill(CUT_NONY);
	}
	if (exsum + eysum > 0) {
		zsum /= exsum + eysum;
	} else {
		zsum = -1;
		CommonData->hCuts->Fill(CUT_NONZ);
	}
//		Count clusters and neutron energy
	ncHits = 0;
	eclust = 0;
	for (i=0; i<cHits; i++) if (CleanEvent[i].type == TYPE_SIPM) {
		xy0 = (CleanEvent[i].z & 1) ? xsum : ysum;
		if (xy0 > 0) {
			r2 = (CleanEvent[i].xy - xy0) * (CleanEvent[i].xy - xy0) * WIDTH * WIDTH + (CleanEvent[i].z - zsum) * (CleanEvent[i].z - zsum) * THICK * THICK;
			if (r2 > Pars.rMax * Pars.rMax) continue;
		}
		eclust += CleanEvent[i].amp;
		if (CleanEvent[i].amp > Pars.nHitMin * SIPM2MEV) ncHits++;
	}
	eclust /= SIPM2MEV;
//		Implement criteria
	if (ncHits >= Pars.nMin && eclust > Pars.eNMin && eclust < Pars.eNMax && eclust / eall > Pars.eNFraction) {
		EventTag = TAG_NEUTRON;
		EventEnergy = eclust;
		Recent[2].Energy = eclust;
		Recent[2].gTime = gtime;
		Recent[2].Vertex[0] = xsum * WIDTH;
		Recent[2].Vertex[1] = ysum * WIDTH;
		Recent[2].Vertex[2] = zsum * THICK;
		CommonData->hTagEnergy[1]->Fill(Recent[2].Energy);
		CommonData->hTagXY[1]->Fill(Recent[2].Vertex[0], Recent[2].Vertex[1]);
		CommonData->hTagZ[1]->Fill(Recent[2].Vertex[2]);
		CommonData->hCuts->Fill(CUT_NEUTRON);
		return;
	}
	if (ncHits < Pars.nMin) CommonData->hCuts->Fill(CUT_NMULT);
	if (eclust <= Pars.eNMin || eclust >= Pars.eNMax) CommonData->hCuts->Fill(CUT_NENERGY);
	if (eclust / eall <= Pars.eNFraction) CommonData->hCuts->Fill(CUT_NFRACTION);
	
//		Check for pisitron
//		Find consequtive cluster near EMAX strip
	E = 0;
	for (i=0; i<cHits; i++) if (CleanEvent[i].type == TYPE_SIPM) if (CleanEvent[i].amp > E) {
		E = CleanEvent[i].amp;
		j = i;
	}
	ncHits = 0;
	eclust = 0;
	xsum = 0;
	exsum = 0;
	ysum = 0;
	eysum = 0;
	for (i=0; i<cHits; i++) if (CleanEvent[i].type == TYPE_SIPM && neighbors(CleanEvent[i].xy, CleanEvent[j].xy, CleanEvent[i].z, CleanEvent[j].z)) {
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
	}
	eclust /= SIPM2MEV;
	E /= SIPM2MEV;
	if (exsum > 0) {
		xsum /= exsum;
	} else {
		xsum = -1;
	}
	if (eysum > 0) {
		ysum /= eysum;
	} else {
		ysum = -1;
	}
	if (exsum + eysum > 0) {
		zsum /= exsum + eysum;
	} else {
		zsum = -1;
	}
//		Implement criteria
	if (E > Pars.eHitMin && ncHits <= Pars.nClustMax && eclust > Pars.ePosMin && eclust < Pars.ePosMax && eclust / eall > Pars.ePosFraction) {
		EventTag = TAG_POSITRON;
		EventEnergy = eclust;
		Recent[1].Energy = eclust;
		Recent[1].gTime = gtime;
		Recent[1].Vertex[0] = xsum * WIDTH;
		Recent[1].Vertex[1] = ysum * WIDTH;
		Recent[1].Vertex[2] = zsum * THICK;
		CommonData->hTagEnergy[0]->Fill(Recent[1].Energy);
		CommonData->hTagXY[0]->Fill(Recent[1].Vertex[0], Recent[1].Vertex[1]);
		CommonData->hTagZ[0]->Fill(Recent[1].Vertex[2]);
		CommonData->hCuts->Fill(CUT_POSITRON);
		return;		
	}
	if (ncHits > Pars.nClustMax) CommonData->hCuts->Fill(CUT_PMULT);
	if (E <= Pars.eHitMin || eclust <= Pars.ePosMin || eclust >= Pars.ePosMax) CommonData->hCuts->Fill(CUT_PENERGY);
	if (eclust / eall <= Pars.ePosFraction) CommonData->hCuts->Fill(CUT_PFRACTION);
	CommonData->hCuts->Fill(CUT_NONE);
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
void dshowMainFrame::Reset(void)
{
	ResetSelfHists();
	ResetSpectrumHists();
	ResetTimeHists();
	ResetSummaHists();
	ResetTagHists();
	ResetNeutrinoHists();
	ResetMuonHists();
}

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
		CommonData->hTimeC[i]->Reset();
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
	CommonData->hCuts->Reset();
	TThread::UnLock();
}

void dshowMainFrame::ResetTagHists(void)
{
	int i;

	TThread::Lock();
	for (i=0; i<2; i++) {
		CommonData->hTagEnergy[i]->Reset();
		CommonData->hTagXY[i]->Reset();
		CommonData->hTagZ[i]->Reset();
	}
	TThread::UnLock();
}

void dshowMainFrame::ResetNeutrinoHists(void)
{
	int i;

	TThread::Lock();
	for (i=0; i<2; i++) CommonData->hNeutrinoEnergy[i]->Reset();
	CommonData->hCaptureTime->Reset();
	CommonData->hNeutronPath->Reset();
	TThread::UnLock();
}

void dshowMainFrame::ResetMuonHists(void)
{
	int i;

	TThread::Lock();
	for (i=0; i<2; i++) {
		CommonData->hMesoEnergy[i]->Reset();
		CommonData->hMesoTime[i]->Reset();
	}
	TThread::UnLock();
}

void dshowMainFrame::Connect2Online(void)
{
	PlayFileStop();
	if (Follow->IsDown()) {
		CommonData->iStop = 0;
		DataThread = new TThread("DataThread", DataThreadFunction, (void *) this);
		DataThread->Run();
	}
}

/*	Change TimeB threshold				*/
void dshowMainFrame::ChangeTimeBThr(void)
{
	CommonData->TimeBThreshold = nTimeBThreshold->GetIntNumber();
}

/*	Change Summa parameters				*/
void dshowMainFrame::ChangeSummaPars(void)
{
	CommonData->SummaSiPMThreshold = SummaSiPMThreshold->GetIntNumber();
	CommonData->SiPMWindow = nSiPMWindow->GetNumber();
}

void dshowMainFrame::ReadConfig(const char *fname)
{
	config_t cnf;
#ifdef LIBCONFIG_VER_MAJOR
	int tmp;
#else
	long tmp;
#endif
	double dtmp;
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
	
//		Analysis parameters
//	float eHitMin;
	Pars.eHitMin = (config_lookup_float(&cnf, "Dshow.eHitMin", &dtmp)) ? dtmp : 0.4;
//	Positron specific
//	float ePosMin;
	Pars.ePosMin = (config_lookup_float(&cnf, "Dshow.ePosMin", &dtmp)) ? dtmp : 1.0;
//	float ePosMax;
	Pars.ePosMax = (config_lookup_float(&cnf, "Dshow.ePosMax", &dtmp)) ? dtmp : 6.0;
//	float ePosFraction;
	Pars.ePosFraction = (config_lookup_float(&cnf, "Dshow.ePosFraction", &dtmp)) ? dtmp : 0.5;
//	int nClustMax;
	Pars.nClustMax = (config_lookup_int(&cnf, "Dshow.nClustMax", &tmp)) ? tmp : 4;
//	Neutron cuts
//	float nHitMin;
	Pars.nHitMin = (config_lookup_float(&cnf, "Dshow.nHitMin", &dtmp)) ? dtmp : 0.4;
//	int nMin;
	Pars.nMin = (config_lookup_int(&cnf, "Dshow.nMin", &tmp)) ? tmp : 5;
//	float eNMin;
	Pars.eNMin = (config_lookup_float(&cnf, "Dshow.eNMin", &dtmp)) ? dtmp : 3.0;
//	float eNMax;
	Pars.eNMax = (config_lookup_float(&cnf, "Dshow.eNMax", &dtmp)) ? dtmp : 10.0;
//	float rMax;
	Pars.rMax = (config_lookup_float(&cnf, "Dshow.rMax", &dtmp)) ? dtmp : 20.0;
//	float rMax;
	Pars.eNFraction = (config_lookup_float(&cnf, "Dshow.eNFraction", &dtmp)) ? dtmp : 0.8;
//	tNeutronCapture = 50;	// neutron capture time, us
	Pars.tNeutronCapture = (config_lookup_float(&cnf, "Dshow.tNeutronCapture", &dtmp)) ? dtmp : 50;
//	tMesoDecay = 5;		// Meso atom decay time, us
	Pars.tMesoDecay = (config_lookup_float(&cnf, "Dshow.tMesoDecay", &dtmp)) ? dtmp : 5;
//	NeutronPath = 30;		// Meso atom decay time, us
	Pars.NeutronPath = (config_lookup_float(&cnf, "Dshow.NeutronPath", &dtmp)) ? dtmp : 30;
//	int OutPort;			// Out port 0xB230
	Pars.HostPort = (config_lookup_int(&cnf, "Sink.OutPort", &tmp)) ? tmp : 0xB230;
//	char MyName[MAXSTR];		// The server host name
	strncpy(Pars.HostName, (config_lookup_string(&cnf, "Sink.MyName", (const char **) &stmp)) ? stmp : "dserver.danss.local", sizeof(Pars.HostName));

	config_destroy(&cnf);
}

/*	Return 1 if strips are neighbors.		*/
int dshowMainFrame::neighbors(int xy1, int xy2, int z1, int z2)
{
	if (abs(z1-z2) == 1) return 1;
	if (z1 == z2 && abs(xy1 - xy2) <= 1) return 1;
	return 0;
}

/*	Read one record							*/
/*	Return 1 if OK, negative on error, 0 on the EOF			*/
/*	If iFollow wait till full record and doesn't return on EOF,	*/
/*	otherwise partial record will produce an error.			*/
/*	Watch for iStop. 						*/
int GetRecord(char *buf, int buf_size, FILE *f, int iFollow, int *iStop)
{
	int Cnt;
	int irc;
	int len;
	struct timeval tm;
//		Read length
	Cnt = 0;
	while (!(*iStop) && Cnt < sizeof(int)) {
		irc = fread(&buf[Cnt], 1, sizeof(int) - Cnt, f);
		if (irc < 0) return irc;	// Error
		Cnt += irc;
		if (Cnt < sizeof(int) && iFollow) {
			tm.tv_sec = 0;		// do some sleep and try to get more data
			tm.tv_usec = 200000;
			select(FD_SETSIZE, NULL, NULL, NULL, &tm);			
			continue;
		}
		if (!Cnt) return 0;		// EOF
		if (Cnt != sizeof(int)) return -1;	// Strange amount read
	}

	if (*iStop) return 0;

	len = *((int *)buf);
	if (len < sizeof(int) || len > buf_size) return -2;	// strange length

//		Read the rest
	while (!(*iStop) && Cnt < len) {
		irc = fread(&buf[Cnt], 1, len - Cnt, f);
		if (irc < 0) return irc;	// Error
		Cnt += irc;
		if (Cnt < len && iFollow) {
			tm.tv_sec = 0;		// do some sleep and try to get more data
			tm.tv_usec = 200000;
			select(FD_SETSIZE, NULL, NULL, NULL, &tm);			
			continue;
		}
		if (Cnt != len) return -3;	// Strange amount read
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
FILE* OpenTCP(const char *hname, int port) 
{
	FILE *f;
	struct hostent *host;
	struct sockaddr_in hostname;
	int fd, flags, irc;
	
	host = gethostbyname(hname);
	if (!host) return NULL;
	
	hostname.sin_family = AF_INET;
	hostname.sin_port = htons(port);
	hostname.sin_addr = *(struct in_addr *) host->h_addr;

	fd = socket(PF_INET, SOCK_STREAM, 0);
	if (fd < 0) return NULL;
	
	flags = 1;
	irc = setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &flags, sizeof(flags));
	if (irc) {
		close(fd);
		return NULL;
	}

	if (connect(fd, (struct sockaddr *)&hostname, sizeof(hostname))) {
		close(fd);
		return NULL;
	}
        flags = fcntl (fd, F_GETFL, 0);
	if (flags == -1) {
		close(fd);
		return NULL;

	}
        flags |= O_NONBLOCK;
        irc = fcntl(fd, F_SETFL, flags);
	if (irc == -1) {
		close(fd);
		return NULL;
	}
	f = fdopen(fd, "rb");
	if (!f) {
		close(fd);
		return NULL;
	}
	return f;
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
	off_t fsize;
	off_t rsize;
	int iNum;
	int Cnt;
	TObject *f;
	int FileCnt;
	int nFiles;
	char str[256];
	
	Main = (dshowMainFrame *)ptr;
	CommonData = Main->CommonData;
	CommonData->iError = 0;
	
	buf = (char *)malloc(BSIZE);
	if (!buf) {
		CommonData->iError = 10;
		goto Ret;
	}
	head = (struct rec_header_struct *) buf;
	
	if (Main->Follow->IsDown()) {
		fIn = OpenTCP(Main->Pars.HostName, Main->Pars.HostPort);
		nFiles = 0;
		fsize = 0;
		snprintf(str, sizeof(str), "Online(%s:%d)", Main->Pars.HostName, Main->Pars.HostPort);
	} else {
		f = Main->PlayFile->fFileNamesList->First();
		fIn = OpenDataFile(f->GetName(), &fsize);
		nFiles = Main->PlayFile->fFileNamesList->GetEntries();
		snprintf(str, sizeof(str), "%s (%d/%d)", f->GetName(), FileCnt + 1, nFiles);
	}
	if (!fIn) {
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
		irc = GetRecord(buf, BSIZE, fIn, Main->Follow->IsDown(), &CommonData->iStop);
		if (irc < 0) {
			CommonData->iError = -irc;
			break;
		} else if (CommonData->iStop) {
			break;
		} else if (irc == 0) {
			if (Main->Follow->IsDown()) continue;
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
					goto Ret;
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

float FindHalfTime(short int *data, int cnt, int amp)
{
	int i;
	float r;
	for (i=0; i<cnt; i++) if (data[i] > amp/2) break;
	if (!i) return 0;
	r = amp;
	r = (r/2 - data[i-1]) / (data[i] - data[i-1]);
	return i + r - 1;
}

/*	the main						*/
int main(int argc, char **argv) {
   	TApplication theApp("DANSS_SHOW", &argc, argv);
   	dshowMainFrame *frame = new dshowMainFrame(gClient->GetRoot(), 2000, 1000, (argc > 1) ? argv[1] : DEFCONFIG);
   	theApp.Run();
   	return 0;
}

