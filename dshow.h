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

struct common_data_struct {
	int iStop;		// flag our thread to stop;
	int iError;		// thread error flag
	int EventCnt;		// Count events
	int SelfTrigCnt;	// Count self triggers
	int BlockCnt;		// Dara blocks counter
	int ErrorCnt;		// count data errors
//		histogramms etc to show
	TH1D *hWaveForm;	// Waveform to show
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
	int SummaSiPMThreshold;	// SiPM threshold for contribution in Summa histogramms
	float SummaSiPMtime[2];	// SiPM time range
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
};

#define TYPE_SIPM	0
#define TYPE_PMT	1
#define	TYPE_VETO	2

struct channel_struct {
	char type;		// 0 - SiPM, 1 - PMT, 2 - Veto, -1 - None
	char xy;		// x or y: 0 - 24
	char z;			// z : 0 - 99, x - odd, y - even.
};

void *DataThreadFunction(void *ptr);

class dshowMainFrame : public TGMainFrame {
private:
	struct common_data_struct *CommonData;
	struct channel_struct Map[MAXWFD][64];
	struct evt_disp_struct *Event;	// event under analysis
	int EventLength;		// allocated area in hits
	
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
	TGNumberEntry *nTimeBThreshold;
	TGRadioButton *rTimeAll;
	TGRadioButton *rTimeSingle;
	TLegend *TimeLegend;
//		Event display tab
	TRootEmbeddedCanvas *fEventCanvas;
	TGNumberEntry *nPMTThreshold;
	TGNumberEntry *nSiPMThreshold;
	TGNumberEntry *nPMTSumThreshold;
	TGNumberEntry *nSiPMSumThreshold;
	int PaletteStart;
//		Summa tab
	TRootEmbeddedCanvas *fSummaCanvas;
	TGNumberEntry *SummaSiPMThreshold;
	TGDoubleHSlider *SummaSiPMtime;
	TGLabel *lbSiPMtime[2];
	TLegend *SummaLegend;
//		Rate tab
	TRootEmbeddedCanvas *fRateCanvas;
//		Right buttons
	TGCheckButton *Pause;
	TGNumberEntry *nRefresh;
	TGNumberEntry *nWFD;
	TGNumberEntry *nChan;
	TGFileInfo *PlayFile;
	TGNumberEntry *nPlayBlocks;
	TGHProgressBar *PlayProgress;

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
	void DrawEvent(TCanvas *cv);
public:
   	dshowMainFrame(const TGWindow *p, UInt_t w, UInt_t h);
   	virtual ~dshowMainFrame(void);
	virtual void CloseWindow(void);
   	void DoDraw(void);
	void ResetSelfHists(void);
	void ResetSpectrumHists(void);
	void ResetTimeHists(void);
	void ResetSummaHists(void);
	void ChangeTimeBThr(void);
	void ChangeSummaPars(void);
	void OnTimer(void);
	void PlayFileDialog(void);
	void PlayFileStop(void);
	void ReadConfig(const char *fname);
	friend void *DataThreadFunction(void *ptr);
	ClassDef(dshowMainFrame, 0)
};

int FindMaxofShort(short int *data, int cnt);
int FindHalfTime(short int *data, int cnt, int ampl);

