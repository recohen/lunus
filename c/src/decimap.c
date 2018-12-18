/* DECIMAP.C - Decimate a ccp4 map.
   
   Author: Mike Wall
   Date: 5/4/2015
   Version: 1.
   
   Usage:
   		"decimap <input ccp4 map> <output ccp4 map> <decimation factor>"

		Input is a map in CCP4 format. 
			Output is a map decimated by the given factor.  
   */

#include<mwmask.h>

int main(int argc, char *argv[])
{
  FILE
    *mapin,
    *mapout,
    *latticeout;
  
  char
    error_msg[LINESIZE],filename[256];
  
  size_t
    i,
    j,
    k,
    num_read,
    num_wrote;

  int
    decimation_factor;

  CCP4MAP
    *map;
  
/*
 * Set input line defaults:
 */
	mapin = stdin;
	latticeout = stdout;

/*
 * Read information from input line:
 */
	switch(argc) {
	  case 4:
	    decimation_factor=atoi(argv[3]);
	  case 3:
	    if (strcmp(argv[2],"-") == 0) {
	      mapout = stdout;
	    }
	  else {
	    if ((mapout = fopen(argv[2],"wb")) == NULL) {
	      printf("\nCan't open %s.\n\n",argv[2]);
	      exit(0);
	    }
	  }
	  case 2:
	  if (strcmp(argv[1],"-") == 0) {
	    mapin = stdin;
	  }
	  else {
	    strcpy(filename,argv[1]);
	    if ( (mapin = fopen(argv[1],"rb")) == NULL ) {
	      printf("\nCan't open %s.\n\n",argv[1]);
	      exit(0);
	    }
	  }
	  break;
	  default:
	  printf("\n Usage: decimap <input ccp4 map> <output ccp4 map> <decimation factor>\n\n");
	  exit(0);
	}
  
  /*
   * Initialize map:
   */

  if ((map = linitmap()) == NULL) {
    perror("Couldn't initialize map.\n\n");
    exit(0);
  }
  
  /*
   * Read in map:
   */

  map->filename = filename;
  map->infile = mapin;
  if (lreadmap(map) != 0) {
    perror("Couldn't read map.\n\n");
    exit(0);
  }

  

  /*
   * Decimate the map:
   */

  map->decimation_factor=decimation_factor;
  ldecimap(map);

/*
 * Write map to output file:
 */

  map->outfile = mapout;
  if (lwritemap(map) != 0) {
    perror("Couldn't write map.\n\n");
    exit(0);
  }

CloseShop:
  
  /*
   * Free allocated memory:
   */

  //  lfreelt(lat);

  /*
   * Close files:
   */
  
  fclose(mapin);
  fclose(mapout);
}

