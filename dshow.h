#include <TGFrame.h>

#define UDPPORT	0xA230
#define BSIZE 0x800000
#define MAXWFD 70
#define NSPERCHAN 8.0
#define DEFCONFIG	"general.conf"

class TCanvas;
class TGCheckButton;
class TGNumberEntry;
class TGRadioButton;
class TGStatusBar;
class TGTab;
class TH1D;
class TH2D;
class TRootEmbeddedCanvas;
class TThread;
class TTimer;

struct hw_rec_struct_self;

struct evt_disp_struct {
	int amp;
	char type;
	char xy;
	char z;
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
	int TimeBThreshold;	// amplitude threshold for TimeB histogram
//
	int EventHits;		// hits in event to display ready signature when not zero
	int EventLength;	// number of hits allocated
	struct evt_disp_struct *Event;	// allocated by analysis
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
//		Event display tab
	TRootEmbeddedCanvas *fEventCanvas;
	TGNumberEntry *nPMTThreshold;
	TGNumberEntry *nSiPMThreshold;
	TGNumberEntry *nPMTSumThreshold;
	TGNumberEntry *nSiPMSumThreshold;
//		Right buttons
	TGCheckButton *Pause;
	TGNumberEntry *nRefresh;
	TGNumberEntry *nWFD;
	TGNumberEntry *nChan;

	int TimerCnt;
	void ProcessEvent(char *data);
	void ProcessSelfTrig(int mod, int chan, struct hw_rec_struct_self *data);
	void CreateWaveTab(TGTab *tab);
	void CreateSelfTab(TGTab *tab);
	void CreateSpectrumTab(TGTab *tab);
	void CreateTimeTab(TGTab *tab);
	void CreateEventTab(TGTab *tab);
	void DrawEvent(TCanvas *cv);
public:
   	dshowMainFrame(const TGWindow *p, UInt_t w, UInt_t h);
   	virtual ~dshowMainFrame(void);
	virtual void CloseWindow(void);
   	void DoDraw(void);
	void ResetSelfHists(void);
	void ResetSpectrumHists(void);
	void ResetTimeHists(void);
	void ChangeTimeBThr(void);
	void OnTimer(void);
	void ReadConfig(const char *fname);
	friend void *DataThreadFunction(void *ptr);
	ClassDef(dshowMainFrame, 0)
};

int FindMaxofShort(short int *data, int cnt);
int FindHalfTime(short int *data, int cnt, int ampl);

