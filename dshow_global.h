#define BSIZE 0x800000
#define MAXWFD 50
#define MAXHITS (64 * MAXWFD)
#define NSPERCHAN 8.0
#define DEFCONFIG	"dshow.conf"
#define MAXADC	4096
#define MAXRGB	128
#define MAXPMT  30.0
#define MAXSIPM 10.0
//	Strip sizes in cm
#define WIDTH	4.0
#define THICK	1.0
#define MINAMP	5
#define MAXCLUSTITER	5

class TH1D;
class TH2D;

struct hw_rec_struct_self;

struct hit_struct {
	float energy;
	float time;
	short int mod;
	short int type;
	short int xy;
	short int z;
	short int chan;
	short int flag;
};

struct event_struct {
	long long gTime;
	int gTrig;
	float Energy;
	float ClusterEnergy;
	float VetoEnergy;
	float VertexN[3];
	float VertexP[3];
	int NHits;
	int SiPmHits;
	int PmtHits;
	int VetoHits;
	int EventCnt;
	int Flags;
	struct hit_struct *hits;
};

#define FLAG_VETO	1
#define FLAG_POSITRON	2
#define FLAG_NEUTRON	4
#define FLAG_NOFINETIME	0x100
#define FLAG_NOHITS	0x200

struct common_data_struct {
//		Counters
	volatile int EventCnt;		// Count events
	volatile int SelfTrigCnt;	// Count self triggers
	volatile int BlockCnt;		// Dara blocks counter
	volatile int ErrorCnt;		// count data errors
//		histogramms etc to show
	TH1D *hWaveForm;	// Waveform to show
	volatile int thisWaveFormCnt;	// current waveform number
	volatile int WaveFormType;
	volatile int WaveFormChan;
	volatile int WaveFormThr;
	TH2D *hAmpS[MAXWFD];	// amplitude versus channel - self trigger
	TH2D *hAmpE[MAXWFD];	// amplitude versus channel - events
	TH2D *hTimeA[MAXWFD];	// time versus channel - events, no threshold
	TH2D *hTimeB[MAXWFD];	// time versus channel - events, with fixed threshold
	TH2D *hTimeC[MAXWFD];	// channel time - common SiPM time, TimeB thresholds
//		Events
	struct event_struct *DisplayEvent;	// selected event for display
	volatile int DisplayType;
	volatile float DisplayEnergy;
	struct event_struct *Event;		// currently processed event
//		Messages etc
	char msg[100];			// Message to be displayed in the status
	volatile int iThreadStop;	// flag to stop thread
	volatile double inFile;		// progress of current file read %%
	volatile double inNumber;	// progress of files read %% of total number of files
	volatile int iPlayBlocks;	// number of blocks to play per second
};

#define WAVE_SELF	0
#define WAVE_TRIG	1
#define WAVE_HIST	2

#define DISP_ALL	0
#define DISP_NONE	1
#define DISP_VETO	2
#define DISP_POSITRON	3
#define DISP_NEUTRON	4

extern struct common_data_struct Run;

struct channel_struct {
	float Coef;
	float T0;
	char type;		// 0 - SiPM, 1 - PMT, 2 - Veto, -1 - None
	char xy;		// x or y: 0 - 24
	char z;			// z : 0 - 99, x - odd, y - even.
};

extern struct channel_struct Map[MAXWFD][64];

struct common_parameters_struct {
//	11 per pixel * 1.4 crosstalk 19 p.e./MeV ~ 290
//	#define SIPM2MEV 290.0	
	float SiPmCoef;
//	20 MeV ~ 2100 channels
//	#define PMT2MEV 105.0
	float PmtCoef;	
//	#define VETOMIN 500
	float VetoMin;
//	#define DVETOMIN 20.0
	float DVetoMin;
	float SiPmWindow;	// SiPm time selection
	float NeutronMin;	// Minimum neutron capture energy
	float PositronMin;	// Minimum cluster energy
	float NeutronMax;	// Maximum neutron capture energy
	float PositronMax;	// Maximum cluster energy
	float MaxOffClusterEnergy;	// Maximum off-cluster energy
	int TimeBThreshold;	// amplitude threshold for TimeB histogram
	float FineTimeThreshold;	// threhold for fine time calculation
	float SmallSiPmAmp;	// MeV
	int ServerPort;		// Dsink server port
	char ServerName[128];	// Dsink server name
};
extern struct common_parameters_struct Conf;

void *DataThreadFunction(void *ptr);
void ExtractHits(char *data);
void FineTimeAnalysis(void);
void CleanByPmtConfirmation(void);
void CalculateParameters(void);
void CalculateTags(void);
void FillHists(void);
void Event2Draw(void);
void ProcessEvent(char *data);
void ProcessSelfTrig(int mod, int chan, struct hw_rec_struct_self *data);

int FindMaxofShort(short int *data, int cnt);
float FindHalfTime(short int *data, int cnt, int ampl);
int IsNeighbors(int xy1, int xy2, int z1, int z2);

#define LOGFILENAME "dshow-debug.log"
void Log(const char *msg, ...);
void Message(const char *msg, ...);
void FileProgress(double inFile, double inNumber);

