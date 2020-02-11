// dbglog.cpp
#include "dbglog.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#ifdef _DEBUG
const char *log_file="c:\\apps\\dbglog.txt";
#endif

void dbglog(const char* fmt,...) {
#ifdef _DEBUG
	va_list v; va_start(v,fmt);
	dbglogv(fmt,v);
#endif
}
void dbglogv(const char* fmt,va_list v) {
#ifdef _DEBUG
	FILE *f; enum { buf_size=80 }; char buf[buf_size];
	time_t now; struct tm tstruct;
	
	f=fopen(log_file,"a+"); if (!f) return;
	now=time(0); tstruct=*localtime(&now);
    strftime(buf, buf_size, "%Y-%m-%d.%X", &tstruct);
	fprintf(f,"%s: ",buf);
	vfprintf(f,fmt,v);
	fprintf(f,"\n",buf);
	fclose(f);
#endif
}
