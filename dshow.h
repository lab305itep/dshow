#include <TGFrame.h>

#define UDPPORT	0xA230
#define BSIZE 0x800000
#define MAXWFD 70
#define NSPERCHAN 8.0
#define DEFCONFIG	"general.conf"
#define MAXADC	4096
#define MAXRGB	128
//	12 per pixel * 1.5 crosstalk 15 p.e./MeV ~ 250
#define SIPM2MEV 250.0	
//	20 MeV ~ 2000 channels
#define PMT2MEV 100.0
#define RATELEN	1000
//	A second in GTIME units 
#define ONESECOND 125000000
#define RATEPER	5
//	Strip sizes in cm
#define WIDTH	4.0
#define THICK	1.0

class TCanvas;
class TGCheckButton;
class TGDoubleHSlider;
class TGFileInfo;
class TGLabel;
class TGNumberEntry;
class TGHProgressBar;
class TGRadioButton;
class TGStatusBar;
class TGTab;
class TH1D;
class TH2D;
class TLegend;
class TRootEmbeddedCanvas;
class TThread;
class TTimer;

struct hw_rec_struct_self;

struct evt_disp_struct {
	float amp;
	float time;
	short int mod;
	char type;
	char xy;
	char z;
	char chan;
};

struct event_struct {
	long long gTime;
	float Energy;
	float Vertex[3];
};

struct common_data_struct {
	int iStop;		// flag our thread to stop;
	int iError;		// thread error flag
	int EventCnt;		// Count events
	int SelfTrigCnt;	// Count self triggers
	int BlockCnt;		// Dara blocks counter
	int ErrorCnt;		// count data errors
//		histogramms etc to show
	TH1D *hWaveForm;	// Waveform to show
	int thisWaveFormCnt;	// current waveform number
	TH2D *hAmpS[MAXWFD];	// amplitude versus channel - self trigger
	TH2D *hAmpE[MAXWFD];	// amplitude versus channel - events
	TH2D *hTimeA[MAXWFD];	// time versus channel - events, no threshold
	TH2D *hTimeB[MAXWFD];	// time versus channel - events, with fixed threshold
	TH2D *hTimeC[MAXWFD];	// channel time - common SiPM time, TimeB thresholds
	int TimeBThreshold;	// amplitude threshold for TimeB histogram
	TH1D *hSiPMsum[2];	// SiPM summa, all/no veto
	TH1D *hSiPMhits[2];	// SiPM Hits, all/no veto
	TH1D *hPMTsum[2];	// PMT summa, all/no veto
	TH1D *hPMThits[2];	// PMT Hits, all/no veto
	TH1D *hSPRatio[2];	// SiPM sum to PTMT sum ratio, all/no veto
	TH1D *hTagEnergy[2];	// Computed enegry for tagged events
	TH2D *hTagXY[2];	// XY-distribution for tagged events
	TH1D *hTagZ[2];		// Z-distribution for tagged events
	TH1D *hNeutrinoEnergy[2];	// Selected events positron and neutron energy
	TH1D *hCaptureTime;	// Neutron decay time
	TH1D *hNeutronPath;	// distance from positron to neutron
	TH1D *hMesoEnergy[2];	// Selected meso-events positron and neutron energy
	TH1D *hMesoTime[2];	// Meso-events meso-decay and neutron capture time
	TH1D *hCuts;		// Cut criteria
	int SummaSiPMThreshold;	// SiPM threshold for contribution in Summa histogramms
	float SiPMWindow;	// SiPM window relative to the SiPM average time
	TH1D *hRate;		// Rate
	float *fRate;		// rate array
	int iRatePos;		// current position in the fRate array
	long long gLastTime;	// position in the global time, seconds
	long long gTime;	// old global time
	int gTrig;		// old trigger number
//
	int EventHits;		// hits in event to display ready signature when not zero
	int EventLength;	// number of hits allocated
	struct evt_disp_struct *Event;	// allocated by analysis for display
	float EventEnergy;	// for display
	int EventTag;		// for display
	int thisEventCnt;	// current event number
};

struct channel_struct {
	char type;		// 0 - SiPM, 1 - PMT, 2 - Veto, -1 - None
	char xy;		// x or y: 0 - 24
	char z;			// z : 0 - 99, x - odd, y - even.
};

#define TAG_NONE	0
#define TAG_VETO	1
#define TAG_POSITRON	2
#define TAG_NEUTRON	3

#define CUT_NONE	0.5
#define CUT_VETO	1.5
#define CUT_POSITRON	2.5
#define CUT_NEUTRON	3.5
#define CUT_NOSITIME	4.5
#define CUT_NONX	5.5
#define CUT_NONY	6.5
#define CUT_NONZ	7.5
#define CUT_NMULT	8.5
#define CUT_NENERGY	9.5
#define CUT_NFRACTION	10.5
#define CUT_PMULT	11.5
#define CUT_PENERGY	12.5
#define CUT_PFRACTION	13.5

struct select_parm_struct {
//	Positron cuts
	float eHitMin;
	float ePosMin;
	float ePosMax;
	float ePosFraction;
	int nClustMax;
//	Neutron cuts
	float nHitMin;
	int nMin;
	float eNMin;
	float eNMax;
	float rMax;
	float eNFraction;
//	Time windows
	float tNeutronCapture;	// neutron capture time, us
	float tMesoDecay;		// Meso atom decay time, us
	float NeutronPath;
};

void *DataThreadFunction(void *ptr);

class dshowMainFrame : public TGMainFrame {
private:
	struct common_data_struct *CommonData;
	struct channel_struct Map[MAXWFD][64];
	struct evt_disp_struct *Event;	// event under analysis
	struct evt_disp_struct *CleanEvent;	// event under analysis, cleaned
	int EventLength;		// allocated area in hits: Event
	int CleanLength;		// allocated area in hits: CleanEvent
	int EventTag;
	float EventEnergy;
	struct select_parm_struct Pars;
	struct event_struct Recent[3];	// Recent VETO, POSITRON and NEUTRON-like events correspondingly
	
	TThread *DataThread;
	TGStatusBar *fStatusBar;
	TTimer *OneSecond;
//		WaveForm tab
	TRootEmbeddedCanvas *fWaveCanvas;
	TGNumberEntry *nWaveThreshold;
	TGRadioButton *rWaveSelf;
	TGRadioButton *rWaveTrig;
	TGRadioButton *rWaveHist;
//		Selftrigger tab
	TRootEmbeddedCanvas *fSelfCanvas;
	TGRadioButton *rSelf2D;
	TGRadioButton *rSelfChan;
	TGRadioButton *rSelfAll;
	TGRadioButton *rSelfSingle;
//		Spectrum tab
	TRootEmbeddedCanvas *fSpectrumCanvas;
	TGRadioButton *rSpect2D;
	TGRadioButton *rSpectChan;
	TGRadioButton *rSpectAll;
	TGRadioButton *rSpectSingle;
//		Time tab
	TRootEmbeddedCanvas *fTimeCanvas;
	TGRadioButton *rTimeAll;
	TGRadioButton *rTimeSingle;
	TGNumberEntry *nTimeBThreshold;
	TGNumberEntry *nSiPMWindow;
	TLegend *TimeLegend;
//		Event display tab
	TRootEmbeddedCanvas *fEventCanvas;
	TGNumberEntry *nPMTThreshold;
	TGNumberEntry *nSiPMThreshold;
	TGNumberEntry *nPMTSumThreshold;
	TGNumberEntry *nSiPMSumThreshold;
	TGRadioButton *rEvtAll;
	TGRadioButton *rEvtNone;
	TGRadioButton *rEvtVeto;
	TGRadioButton *rEvtNeutron;
	TGRadioButton *rEvtPositron;
	int PaletteStart;
//		Summa tab
	TRootEmbeddedCanvas *fSummaCanvas;
	TGNumberEntry *SummaSiPMThreshold;
	TLegend *SummaLegend;
//		Rate tab
	TRootEmbeddedCanvas *fRateCanvas;
//		Tagged tab
	TRootEmbeddedCanvas *fTagCanvas;
//		Neutrino tab
	TRootEmbeddedCanvas *fNeutrinoCanvas;
//		Muon tab
	TRootEmbeddedCanvas *fMuonCanvas;	
	
//		Right buttons
	TGCheckButton *Pause;
	TGNumberEntry *nRefresh;
	TGNumberEntry *nWFD;
	TGNumberEntry *nChan;
	TGFileInfo *PlayFile;
	TGNumberEntry *nPlayBlocks;
	TGHProgressBar *PlayProgress;
	TGHProgressBar *FileProgress;
	TGCheckButton *Follow;
	TGFileInfo *SaveFile;

	int TimerCnt;
	void ProcessEvent(char *data);
	void ProcessSelfTrig(int mod, int chan, struct hw_rec_struct_self *data);
	void CreateWaveTab(TGTab *tab);
	void CreateSelfTab(TGTab *tab);
	void CreateSpectrumTab(TGTab *tab);
	void CreateTimeTab(TGTab *tab);
	void CreateEventTab(TGTab *tab);
	void CreateSummaTab(TGTab *tab);
	void CreateRateTab(TGTab *tab);
	void CreateTagTab(TGTab *tab);
	void CreateNeutrinoTab(TGTab *tab);
	void CreateMuonTab(TGTab *tab);
	void DrawEvent(TCanvas *cv);
	void CalculateTags(int nHits, long long gtime);
	int neighbors(int xy1, int xy2, int z1, int z2);
	void ReadConfig(const char *fname);
public:
   	dshowMainFrame(const TGWindow *p, UInt_t w, UInt_t h, const char *cfgname);
   	virtual ~dshowMainFrame(void);
	virtual void CloseWindow(void);
   	void DoDraw(void);
	void Reset(void);
	void ResetSelfHists(void);
	void ResetSpectrumHists(void);
	void ResetTimeHists(void);
	void ResetSummaHists(void);
	void ResetTagHists(void);
	void ResetNeutrinoHists(void);
	void ResetMuonHists(void);
	void ChangeTimeBThr(void);
	void ChangeSummaPars(void);
	void OnTimer(void);
	void PlayFileDialog(void);
	void PlayFileStop(void);
	void SaveDialog(void);
	void SaveEvent(void);
	void SaveWaveForm(void);
	friend void *DataThreadFunction(void *ptr);
	ClassDef(dshowMainFrame, 0)
};

int FindMaxofShort(short int *data, int cnt);
float FindHalfTime(short int *data, int cnt, int ampl);

