#ifndef DRATE_H
#define DRATE_H

class TGraph;

class Drate {
private:
	double *tmS;	// array of time stamps
	double *dataS;	// array of data
	double *tmL;	// array of time stamps
	double *dataL;	// array of data
	int ptrL;	// current pointer
	int ptrS;	// current pointer
	int len;	// array length
	long long gt0;	// 125 MHz time of the last point start
	long long period;	// period in 125 MHz clock
	int cntS;	// current counter
	int cntL;
	long long tmSum;
	int LS;
	int cntLS;
public:
	Drate(long long cPeriod, int cLength, int cLongShort);	// period, length, ratio
	~Drate(void);
	TGraph *GetGraphShort(void);		// get the graph - short period
	TGraph *GetGraphLong(void);		// long period
	double GetLast(void);			// get the last measurement
	double GetAverage(void);
	void Push(long long gTime, int uTime, int IsMe);	// push and event at time gTime. Signature IsMe indicates if this event should be counted or only time indicated
};

#endif /* DRATE_H */

