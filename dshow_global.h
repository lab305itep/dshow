#define BSIZE 0x800000
#define MAXWFD 50
#define MAXHITS (64 * MAXWFD)
#define NSPERCHAN 8.0
#define DEFCONFIG	"dshow.conf"
#define MAXADC	4096
#define MAXRGB	128
//	Strip sizes in cm
#define WIDTH	4.0
#define THICK	1.0

class TH1D;
class TH2D;

struct hw_rec_struct_self;

struct hit_struct {
	float energy;
	float time;
	short int mod;
	char type;
	char xy;
	char z;
	char chan;
	char in;		// in cluster
	char flag;
};

struct event_struct {
	long long gTime;
	float Energy;
	float ClusterEnergy;
	float VertexN[3];
	float VertexP[3];
	int NHits;
	int NSiPmHits;
	int EventCnt;
	int Flags;
	struct hit_struct *hits;
};

struct common_data_struct {
//		Counters
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
	struct event_struct *DisplayEvent;	// selected event for display
	char msg[100];
};

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
//	#define VETOMINA 500
	float VetoMinA;
//	#define VETOMINB 50
	float VetoMinB;
//	#define DVETOMIN 20.0
	float DVetoMin;
	float SiPmWindow;	// SiPm time selection
	float NeutronMin;	// Minimum neutron capture energy
	float PositronMin;	// Minimum cluster energy
	float NeutronMax;	// Maximum neutron capture energy
	float PositronMax;	// Maximum cluster energy
	float MaxOffClusterEnergy;	// Maximum off-cluster energy
	int TimeBThreshold;	// amplitude threshold for TimeB histogram
	int ServerPort;		// Dsink server port
	char ServerName[128];	// Dsink server name
};
extern struct common_parameters_struct Conf;

#define FLAG_VETO	1
#define FLAG_POSITRON	2
#define FLAG_NEUTRON	4
#define FLAG_NOISE	0x100

void *DataThreadFunction(void *ptr);
void ProcessEvent(char *data);
void ProcessSelfTrig(int mod, int chan, struct hw_rec_struct_self *data);

int FindMaxofShort(short int *data, int cnt);
float FindHalfTime(short int *data, int cnt, int ampl);
int IsNeighbors(int xy1, int xy2, int z1, int z2);

#define LOGFILENAME "dshow-debug.log"
void Log(const char *msg, ...);

