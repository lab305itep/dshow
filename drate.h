#ifndef DRATE_H
#define DRATE_H

class TGraph;

class Drate {
private:
	double *tm;	// array of time stamps
	double *data;	// array of data
	int ptr;	// current pointer
	int len;	// array length
	long long gt0;	// 125 MHz time of the last point start
	long long period;	// period in 125 MHz clock
	int cnt;	// current counter
public:
	Drate(long long cPeriod, int cLength);	// period and length
	~Drate(void);
	TGraph *GetGraph(void);			// get the graph
	double GetLast(void);			// get the last measurement
	void Push(long long gTime, int uTime, int IsMe);	// push and event at time gTime. Signature IsMe indicates if this event should be counted or only time indicated
};

#endif /* DRATE_H */

