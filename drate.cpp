#include <TGraph.h>
#include "drate.h"
#include "dshow_global.h"

Drate::Drate(long long cPeriod, int cLength, int cLongShort)	// period, length, ratio
{
	len = 0;
	
	tmS = (double *) malloc(cLength * sizeof(double));
	dataS = (double *) malloc(cLength * sizeof(double));
	tmL = (double *) malloc(cLength * sizeof(double));
	dataL = (double *) malloc(cLength * sizeof(double));

	if (!tmS || !dataS || !tmL || !dataL) return;	// no memory

	len = cLength;
	period = cPeriod;
	LS = cLongShort;
	gt0 = -1;
	ptrS = 0;
	ptrL = 0;
	cntS = 0;
	cntL = 0;
	tmSum = 0;
	cntLS = 0;
}

Drate::~Drate(void)
{
	if (tmS) free(tmS);
	if (dataS) free(dataS);
	if (tmL) free(tmL);
	if (dataL) free(dataL);
}

TGraph *Drate::GetGraphShort(void)			// get the graph
{
	if (!ptrS) return NULL;			// nothing to draw
	return new TGraph(ptrS, tmS, dataS);
}

TGraph *Drate::GetGraphLong(void)			// get the graph
{
	if (!ptrL) return NULL;			// nothing to draw
	return new TGraph(ptrL, tmL, dataL);
}

double Drate::GetLast(void)			// get the last measurement
{
	if (!ptrS) return -1;
	return dataS[ptrS-1];
}

double Drate::GetAverage(void)			// get the last measurement
{
	if (!ptrL) return -1;
	return dataL[ptrL-1];
}

void Drate::Push(long long gTime, int uTime, int IsMe)	// push and event at time gTime. Signature IsMe indicates if this event should be counted or only time indicated
{
	double val;

	if (!len) return;	
	if (gt0 < 0 || gt0 > gTime || gTime > gt0 + 2*period) {
		gt0 = gTime;
		cntS = 0;
		cntL = 0;
		tmSum = 0;
	} else if (gt0 + period < gTime) {
		gt0 += period;
		val = 1.0E9 * cntS / (period * NSPERCHAN);
		if (ptrS == len) {
			memmove(tmS, &tmS[1], (len - 1) * sizeof(double));
			memmove(dataS, &dataS[1], (len - 1) * sizeof(double));
			tmS[len-1] = uTime;
			dataS[len - 1] = val;
		} else {
			tmS[ptrS] = uTime;
			dataS[ptrS] = val;
			ptrS++;
		}
		cntL += cntS;
		tmSum += uTime;
		cntLS++;
		if (cntLS == LS) {
			val = 1.0E9 * cntL / (period * NSPERCHAN * LS);
			tmSum /= LS;
			if (ptrL == len) {
				memmove(tmL, &tmL[1], (len - 1) * sizeof(double));
				memmove(dataL, &dataL[1], (len - 1) * sizeof(double));
				tmL[len-1] = uTime;
				dataL[len - 1] = val;
			} else {
				tmL[ptrL] = tmSum;
				dataL[ptrL] = val;
				ptrL++;
			}
			cntL = 0;
		}
		cntS = 0;
	}
	cntS += IsMe;
}

