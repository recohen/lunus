/* LIQUIDFACLT.C - Generate a liquid-like motions prefactor lattice.
   
   Author: Mike Wall
   Date: 2/28/95
   Version: 1.
   
   Usage:
   		"liquidfaclt <input lattice> <output lattice> <width>"

		Input is lattice and width.  Output is liquid-like motions reciprocal space smearing function.
   */

#include<mwmask.h>

int main(int argc, char *argv[])
{
  FILE
    *latticein,
    *latticeout;
  
  char
    error_msg[LINESIZE];

  LAT3D 
    *lat;

  float
    peak,
    width;

/*
 * Set input line defaults:
 */
	
	latticein = stdin;
	latticeout = stdout;

/*
 * Read information from input line:
 */
	switch(argc) {
	  case 4:
	  width = atof(argv[3]);
	  case 3:
	  if (strcmp(argv[2],"-") == 0) {
	    latticeout = stdout;
	  }
	  else {
	    if ((latticeout = fopen(argv[2],"wb")) == NULL) {
	      printf("\nCan't open %s.\n\n",argv[2]);
	      exit(0);
	    }
	  }
	  case 2:
	  if (strcmp(argv[1],"-") == 0) {
	    latticein = stdin;
	  }
	  else {
	    if ( (latticein = fopen(argv[1],"rb")) == NULL ) {
	      printf("\nCan't open %s.\n\n",argv[1]);
	      exit(0);
	    }
	  }
	  break;
	  default:
	  printf("\n Usage: liquidfaclt <input lattice> "
		 "<output lattice> <width>\n\n");
	  exit(0);
	}
  
  /*
   * Initialize lattices:
   */

  if ((lat = linitlt()) == NULL) {
    perror("Couldn't initialize lattice.\n\n");
    exit(0);
  }
  
  /*
   * Read in lattice:
   */

  lat->infile = latticein;
  if (lreadlt(lat) != 0) {
    perror("Couldn't read lattice.\n\n");
    exit(0);
  }

  /*
   * Calculate liquid-like motions prefactor:
   */
  
  lat->width = width;
  lliquidfaclt(lat);
  
  /*
   * Write lattice to output file:
   */
  
  lat->outfile = latticeout;
  if (lwritelt(lat) != 0) {
    perror("Couldn't write lattice.\n\n");
    exit(0);
  }

CloseShop:
  
  /*
   * Free allocated memory:
   */

  lfreelt(lat);

  /*
   * Close files:
   */
  
  fclose(latticein);
  fclose(latticeout);
}


