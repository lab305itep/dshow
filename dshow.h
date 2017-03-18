#include <TGFrame.h>

class TCanvas;
class TF1;
class TGCheckButton;
class TGDoubleHSlider;
class TGFileInfo;
class TGLabel;
class TGNumberEntry;
class TGHProgressBar;
class TGRadioButton;
class TGStatusBar;
class TGTab;
class TLegend;
class TRootEmbeddedCanvas;
class TThread;
class TTimer;

class dshowMainFrame : public TGMainFrame {
private:
	TThread *DataThread;
	TGStatusBar *fStatusBar;
	TTimer *OneSecond;
	int TimerCnt;
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
	TGNumberEntry *nSiPMWindow;
	TLegend *TimeLegend;
	TF1 *fTimeCFit;
//		Event display tab
	TRootEmbeddedCanvas *fEventCanvas;
	TGNumberEntry *nSumEnergyThreshold;
	TGRadioButton *rEvtAll;
	TGRadioButton *rEvtNone;
	TGRadioButton *rEvtVeto;
	TGRadioButton *rEvtNeutron;
	TGRadioButton *rEvtPositron;
	TGRadioButton *rHitAll;
	TGRadioButton *rHitClean;
	TGRadioButton *rHitCluster;
	int PaletteStart;
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
	TGHProgressBar *FileProgress;
	TGCheckButton *Follow;
	TGFileInfo *SaveFile;

	void CreateWaveTab(TGTab *tab);
	void CreateSelfTab(TGTab *tab);
	void CreateSpectrumTab(TGTab *tab);
	void CreateTimeTab(TGTab *tab);
	void CreateEventTab(TGTab *tab);
	void CreateRateTab(TGTab *tab);
	void DrawEvent(TCanvas *cv);
	void InitHist(void); 
public:
   	dshowMainFrame(const TGWindow *p, UInt_t w, UInt_t h);
   	virtual ~dshowMainFrame(void);
	virtual void CloseWindow(void);
   	void DoDraw(void);
	void Reset(void);
	void ResetSelfHists(void);
	void ResetSpectrumHists(void);
	void ResetTimeHists(void);
	void OnPlayBlocksChanged(void);
	void ChangeWaveFormPars(void);
	void ChangeDisplayPars(void);
	void Connect2Online(void);
	void OnTimer(void);
	void PlayFileDialog(void);
	void PlayFileStop(void);
	void SaveDialog(void);
	void SaveEvent(void);
	void SaveWaveForm(void);
	ClassDef(dshowMainFrame, 0)
};

int ReadConfig(const char *fname);

