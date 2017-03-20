#include <TGraph.h>
#include "drate.h"
#include "dshow_global.h"

Drate::Drate(long long cPeriod, int cLength)	// period and length
{
	len = 0;
	
	tm = (double *) malloc(cLength * sizeof(double));
	data = (double *) malloc(cLength * sizeof(double));

	if (!tm || !data) return;	// no memory

	len = cLength;
	period = cPeriod;
	ptr = 0;
	gt0 = -1;
	cnt = 0;
}

Drate::~Drate(void)
{
	if (tm) free(tm);
	if (data) free(data);
}

TGraph *Drate::GetGraph(void)			// get the graph
{
	if (!ptr) return NULL;			// nothing to draw
	return new TGraph(ptr, tm, data);
}

double Drate::GetLast(void)			// get the last measurement
{
	if (!ptr) return -1;
	return data[ptr-1];
}

void Drate::Push(long long gTime, int uTime, int IsMe)	// push and event at time gTime. Signature IsMe indicates if this event should be counted or only time indicated
{
	double val;

	if (!len) return;	
	if (gt0 < 0 || gt0 > gTime || gTime > gt0 + 2*period) {
		gt0 = gTime;
		cnt = 0;
	} else if (gt0 + period < gTime) {
		gt0 += period;
		val = 1.0E9 * cnt / (period * NSPERCHAN);
		if (ptr == len) {
			memmove(tm, &tm[1], (len - 1) * sizeof(double));
			memmove(data, &data[1], (len - 1) * sizeof(double));
			tm[len-1] = uTime;
			data[len - 1] = val;
		} else {
			tm[ptr] = uTime;
			data[ptr] = val;
			ptr++;
		}
		cnt = 0;
	}
	if (IsMe) cnt++;
}

