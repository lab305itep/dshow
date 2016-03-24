//  Preprocess one data file
//  do simple signal analysis and remove events definitely out of 
//  correct timing.

#define _FILE_OFFSET_BITS 64
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "recformat.h"

#define BSIZE  0x800000
#define MAXHIT 500
//	50 us
#define TDIFF  6250
#define NSPERCHAN 8.0

int FindMaxofShort(short int *data, int cnt)
{
	int i, M;
	M = -32000;		// nearly minimum 16-bit signed value
	for (i=0; i<cnt; i++) if (data[i] > M) M = data[i];
	return M;
}

float FindHalfTime(short int *data, int cnt, int amp)
{
	int i;
	float r;
	for (i=0; i<cnt; i++) if (data[i] > amp/2) break;
	if (!i) return 0;
	r = amp;
	r = (r/2 - data[i-1]) / (data[i] - data[i-1]);
	return i + r - 1;
}

int FillEvent(char *buf, struct pre_event_struct *event)
{
	struct rec_header_struct *head;
	union hw_rec_union *rec;
	int ErrCnt;
	int ptr;
	int i;
	
	ErrCnt = 0;
	head = (struct rec_header_struct *)buf;
	event->systime = head->time;
	event->gtime = -125000000;
	event->nhits = 0;

	for (ptr = sizeof(struct rec_header_struct); ptr < head->len; ptr += (rec->rec.len + 1) * sizeof(short)) {
		rec = (union hw_rec_union *) &buf[ptr];
		switch(rec->rec.type) {
		case TYPE_MASTER:
			if (event->nhits < MAXHIT) {
				for (i = 0; i < rec->mast.len - 2; i++) if (rec->mast.d[i] & 0x4000) rec->mast.d[i] |= 0x8000;	// extend sign
				event->hit[event->nhits].amp = FindMaxofShort(rec->mast.d, rec->mast.len - 2);
				event->hit[event->nhits].time = NSPERCHAN * (FindHalfTime(rec->mast.d, rec->mast.len - 2, event->hit[event->nhits].amp) - rec->mast.time / 6.0);
				event->hit[event->nhits].mod = rec->mast.module;
				event->hit[event->nhits].chan = rec->mast.chan;
				event->nhits++;
			}
			break;
		case TYPE_TRIG:
			event->gtime = rec->trig.gtime[0] + (((long long) rec->trig.gtime[1]) << 15) + (((long long) rec->trig.gtime[2]) << 30);
			event->number = rec->trig.cnt[0] + (((int) rec->trig.cnt[1]) << 15);
			break;
		case TYPE_SUM:
		case TYPE_DELIM:
			break;
		default:
			printf("Wrong (unexpected) record type %d met.\n", rec->rec.type);
			ErrCnt++;
		}
	}
	if (event->gtime < 0) ErrCnt++;
	event->len = sizeof(struct pre_event_struct) + sizeof(struct pre_hit_struct) * event->nhits;
	return ErrCnt;
}

void Help(void)
{
	printf("Preprocess data.\n");
	printf("Usage: ./dana1 file.dat[.bz2|xz|gz]\n");
}

/*	Open data file either directly or via zcat etc, depending on the extension	*/
FILE* OpenDataFile(const char *fname) 
{
	char cmd[1024];
	FILE *f;

	if (strstr(fname, ".bz2")) {
		sprintf(cmd, "bzcat %s", fname);
		f = popen(cmd, "r");
	} else if (strstr(fname, ".gz")) {
		sprintf(cmd, "zcat %s", fname);
		f = popen(cmd, "r");
	} else if (strstr(fname, ".xz")) {
		sprintf(cmd, "xzcat %s", fname);
		f = popen(cmd, "r");
	} else {
		f = fopen(fname, "rb");
	}
	return f;
}

FILE* OpenOutFile(const char *fname)
{
	char outname[1024];
	char *ptr;
	
	strcpy(outname, fname);
	ptr = strchr(outname, '.');
	if (ptr) *ptr = '\0';
	strcat(outname, ".rdat");
	return fopen(outname, "wb");
}

int main(int argc, char **argv)
{
	FILE *fIn;
	FILE *fOut;
	int Cnt, EventCnt, OutCnt, ErrCnt;
	int irc;
	char *buf;
	struct pre_event_struct *event[2];
	struct rec_header_struct *head;
	int inum;
	int Written;



	fIn = NULL;
	fOut = NULL;
	buf = NULL;
	event[0] = NULL;
	event[1] = NULL;
	
	if (argc < 2) {
		Help();
		return 10;
	}

	fIn = OpenDataFile(argv[1]);
	if (!fIn) {
		printf("Can not open input file %s: %m\n", argv[1]);
		goto fin;
	}

	fOut = OpenOutFile(argv[1]);
	if (!fOut) {
		printf("Can not open output file for %s: %m\n", argv[1]);
		goto fin;
	}

	buf = (char *) malloc(BSIZE);
	head = (struct rec_header_struct *)buf;
	event[0] = (struct pre_event_struct *) malloc(sizeof(struct pre_event_struct) + MAXHIT * sizeof(struct pre_hit_struct));
	event[1] = (struct pre_event_struct *) malloc(sizeof(struct pre_event_struct) + MAXHIT * sizeof(struct pre_hit_struct));
	if (!buf || !event[0] || !event[1]) {
		printf("Allocation failure %m.\n");
		goto fin;
	}
	event[0]->gtime = -125000000;	// - 1 s.
	event[1]->gtime = -125000000;	// - 1 s.

	Cnt = 0;
	EventCnt = 0;
	OutCnt = 0;
	ErrCnt = 0;
	inum = 0;
	Written = 0;
	for(;!feof(fIn);) {
		irc = fread(buf, sizeof(int), 1, fIn);
		if (irc != 1) break;
		if (head->len > BSIZE || head->len < sizeof(struct rec_header_struct)) {
			printf("Odd block size: %d\n", head->len);
			break;
		}
		irc = fread(&buf[sizeof(int)], head->len - sizeof(int), 1, fIn);
		if (irc != 1) {
			printf("Unexpected EOF - %m\n");
			break;
		}
		Cnt++;
		if (head->type & REC_EVENT) {
			EventCnt++;
			if (FillEvent(buf, event[inum])) ErrCnt++;
			if (event[inum]->gtime - event[1-inum]->gtime < TDIFF) {
				if (!Written) fwrite(event[1-inum], event[1-inum]->len, 1, fOut);
				fwrite(event[inum], event[inum]->len, 1, fOut);
				Written = 1;
				OutCnt++;
			} else {
				Written = 0;
			}
			inum = 1 - inum;
		}
	}

	printf("%d records and %d events processed. %d events written. %d Errors found.\n", Cnt, EventCnt, OutCnt, ErrCnt);
fin:
	if (fIn) fclose(fIn);
	if (fOut) fclose(fOut);
	if (buf) free(buf);
	if (event[0]) free (event[0]);
	if (event[1]) free (event[1]);
	return 0;
}

