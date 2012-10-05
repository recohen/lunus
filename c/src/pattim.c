/* PATTIM.C - Create an image with a pattern.
   
   Author: Mike Wall
   Date: 10/3/2012
   Version: 1.
   
   "pattim <image in> <image out>"
   
   Input is image. Output is image with pattern.

*/

#include<mwmask.h>

int main(int argc, char *argv[])
{
  FILE
	*imagein,
	*imageout;
  
  char
    error_msg[LINESIZE];

  DIFFIMAGE 
	*imdiff;

  int sz,cell_sz;

  size_t
    i,j;

/*
 * Set input line defaults:
 */
	
	imagein = stdin;
	imageout = stdout;

/*
 * Read information from input line:
 */
  switch(argc) {
  case 5:
    cell_sz = atoi(argv[4]);
  case 4:
    sz = atoi(argv[3]);
  case 3:
    if (strcmp(argv[2], "-") == 0) {
      imageout = stdout;
    }
    else {
      if ( (imageout = fopen(argv[2],"wb")) == NULL ) {
	printf("Can't open %s.",argv[2]);
	exit(0);
      }
    }
    case 2:
    if (strcmp(argv[1], "-") == 0) {
      imagein = stdin;
    }
    else {
      if ( (imagein = fopen(argv[1],"rb")) == NULL ) {
	printf("Can't open %s.",argv[1]);
	exit(0);
      }
    }
    break;
    default:
    printf("\n Usage: ovldim "
	   "<image in> <image out> \n\n");
    exit(0);
  }
  
  /*
   * Initialize diffraction image:
   */
  
  if ((imdiff = linitim()) == NULL) {
    perror("Couldn't initialize diffraction image.\n\n");
    exit(0);
  }
 
/*
 * Read diffraction image:
 */

  imdiff->infile = imagein;
  if (lreadim(imdiff) != 0) {
    perror(imdiff->error_msg);
    goto CloseShop;
  }


  /*
   * Replace image with pattern for diffraction:
   */

  size_t index = 0;
  int sgni,sgnj;
  int sgni_last = 0;

  int ic,jc,cellidx;
  int mask;

  int *cells;
  int xcells = imdiff->hpixels/cell_sz;
  int ycells = imdiff->vpixels/cell_sz;
  cells = (int *)malloc(sizeof(int)* xcells * ycells);

  for (i=0;i<xcells*ycells;i++) {
    cells[i] = rand() % 2;
  }

  for (j=0;j<imdiff->vpixels;j++) {
    jc = j/cell_sz;
    mask = 1;
    if (j%sz<sz/2) sgnj = 0; else sgnj = 1;
    for (i=0;i<imdiff->hpixels;i++) {
      ic = i/cell_sz;
      cellidx = jc*xcells+ic;
      if (i%sz<sz/2) sgni = 0; else sgni = 1;
      if ((j%sz<sz/8)||(j%sz>=7*sz/8)) mask = 0; else mask = 1;
      if ((i%sz<sz/8) || (i%sz >= 7*sz/8)) mask = 0;
      if (mask==0) {
	imdiff->image[index] = 10000;
      } else {
      if (sgnj==0) 
	imdiff->image[index] = 10000*mask*((1-sgni)*cells[cellidx] + sgni*(1-cells[cellidx]));
      else
	imdiff->image[index] = 10000*mask*(sgni*cells[cellidx]+(1-sgni)*(1-cells[cellidx]));
      }
      index++;
    }
  }
    
/*
 * Write the output image:
 */

  imdiff->outfile = imageout;
  if(lwriteim(imdiff) != 0) {
    perror(imdiff->error_msg);
    goto CloseShop;
  }

CloseShop:
  
  lfreeim(imdiff);

/*
 * Close files:
 */
  
  fclose(imagein);
  fclose(imageout);
  
}

