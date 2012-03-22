#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "log.h"
#include "serial.h"
#define FILENAME_LOG "log.txt"

void log_retrieve(void) {

	FILE* fh;
	unsigned char* logfile;
	int i;

	int logsize = 0xF0;
	logfile = malloc(logsize);

	//Send logfile request
	if (1 != serial_write('L')) {
		printf("Error sending log request\n");
		return;
	}
	
	for (i=0;i<logsize;) {
		if (1 == serial_read(logfile+i))
			i++;
	}

	printf("%s",logfile);
	//Output logfile to file FILENAME_LOG
	fh = fopen(FILENAME_LOG, "w");
	if (fh == 0) {
		printf("Error opening %s.\n", FILENAME_LOG);
		return;
	}
	//TODO:Limit write chunk size
	if (0 == fwrite(logfile, 1, logsize, fh)) {
		printf("Error writing to file.\n");
		return;
	}
	fclose(fh);
	
}
