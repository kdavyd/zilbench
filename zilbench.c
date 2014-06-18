/* Copyright 2013, Nexenta Systems, Inc.*/


/* printf */
#include <stdio.h>
/* open */
#include <fcntl.h>
/* strtol exit rand srand malloc */
#include <sys/types.h>
#include <stdlib.h>
#include <limits.h>
/* strlen */
#include <string.h>
/* lseek fsync */
#include <unistd.h>
/* gethrtime */
#include <sys/time.h>

void usage(char *prog) 
{
	(void) printf("Use %s <blocksize in bytes> <how many blocks> <device>\n" ,
																prog) ;
	exit(-1) ;
}

int main(int argc , char **argv)
{
	char *rdata = NULL ;
	char *buffer = NULL ;
	char devicePath[1024] ;
	int blockSize = 0 ;
	uint64_t blockCount = 0 ;
	uint64_t blockCounter = 0 ;
	int fdw = -1 ;
	int fdr = -1 ;
	off_t fsz = (off_t)0 ;
	size_t bread = (size_t)0 ;
	size_t bwritten = (size_t)0 ;
	off_t maxOffset = (off_t)0 ;
	off_t roffset = (off_t)0 ;
	int j = 0 ;
	int r = 0 ;
	hrtime_t start ;
	hrtime_t stop ;
	hrtime_t elapsed = (hrtime_t)0;

	if(argc != 4) {
		usage(argv[0]) ;
	}

	if((blockSize = (uint64_t)strtol(argv[1] , NULL , 10)) <= 0 || (blockCount =
				(uint64_t)strtol(argv[2] , NULL , 10)) <= 0) {
		(void) fprintf(stderr , "conversion of block size" 
									"or block count failed.\n") ;
		exit(-1) ;
	}

	rdata = malloc(blockSize*10) ;
	if(rdata == NULL) {
		(void) fprintf(stderr , "Out of memory.\n") ;
		exit(-1) ;
	}

	if(strlen(argv[3]) >= 1024) {
		(void) fprintf(stderr , "Pathname exceeds buffer size.\n") ;
		free(rdata) ;
		exit(-1) ;
	}

	strcpy(devicePath , argv[3]) ;

	/* open /dev/urandom and read chunks of bytes to write to devicePath */

	if((fdr = open("/dev/urandom" , O_RDONLY)) == -1) {
		(void) fprintf(stderr , "Unable to open /dev/urandom for reading.\n") ;
		free(rdata) ;
		exit(-1) ;
	}

	if((bread = read(fdr , rdata , blockSize*10)) !=
			(size_t)(blockSize*10)) {
		fprintf(stderr , "Expected to read %d bytes from /dev/urandom" 
				" , got %d bytes\n",
				blockSize*10 , (uint64_t)bread) ; 
	} 

	close(fdr) ;

	if((fdw = open(devicePath , O_WRONLY)) == -1) {
		(void) fprintf(stderr , "Unable to open %s for writing.\n" 
															, devicePath) ;
		free(rdata) ;
		(void) close(fdr) ;
		exit(-1) ;
	}

	if((fsz = lseek(fdw , (off_t)0 , SEEK_END)) == (off_t)-1) {
		(void) fprintf(stderr , "Unable to get the size of %s.\n" , argv[3]) ;
		free(rdata) ;
		(void) close(fdr) ;
		(void) close(fdw) ;
		exit(-1) ;
	}

	maxOffset = fsz - (off_t)blockSize - (off_t)1 ;
	fprintf(stderr , "Disk size is %ld, maxOffset is %ld\n" , fsz , maxOffset) ;

	if((fsz = lseek(fdw , (off_t)0 , SEEK_SET)) == (off_t)-1) {
		(void) fprintf(stderr , "Unable to rewind to the beginning %s.\n" 
																, argv[3]) ;
		free(rdata) ;
		(void) close(fdr) ;
		(void) close(fdw) ;
		exit(-1) ;
	}

	srandom((unsigned)gethrtime()) ;
	//srand((unsigned)gethrtime()) ;

	for(blockCounter = 0 ; blockCounter < blockCount ; blockCounter++) {
		j = rand() ;
		buffer = (rdata + (j%(blockSize*10-blockSize))) ;
		roffset = (off_t)((random()*random())%(long)maxOffset) ;
		//roffset = (off_t)((rand()*rand())%(long)maxOffset) ;

		if((r = ((long)roffset % (long)blockSize)) != 0 && r != 0) {
			roffset -= (off_t)r ;	
		}

		start = gethrtime() ;
		if((bwritten = pwrite(fdw , buffer , blockSize , roffset))
				!= (size_t)blockSize) {
			fprintf(stderr , "Expected to write %d bytes, wrote %d bytes\n",
					blockSize , (uint64_t)bwritten) ; 
		} else {
			fsync(fdw) ;
			stop = gethrtime() ;
			elapsed += (stop-start) ;
		}

		buffer = NULL ;
	}

	free(rdata) ;
	close(fdw) ;

	printf("Wrote %d blocks in chunks of %d bytes in %.8f seconds\n" ,
			blockCounter , blockSize , (float)((float)elapsed/1000000000.0)) ;

	exit(0) ;
}

