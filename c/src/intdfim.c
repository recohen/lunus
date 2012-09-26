/* INTDFIM.C - Integrate diffuse features in a diffraction image and output to
               a file.
   
   Author: Mike Wall  
   Date: 5/28/94
   Version: 1.0
   
   "intdfim <diffuse features in> <image in> <diffuse features out>"

   Input is ascii diffuse features file and input image.  Output is
   diffuse features file.

*/

#include<mwmask.h>

int main(int argc, char *argv[])
{
  FILE
    *featurein,
    *featureout,
    *imagein;
  
  char
    error_msg[LINESIZE];
  
  long   
    i,
    ii;
  
  DIFFIMAGE 
    *imdiff;
  
  /*
   * Set input line defaults:
   */
  
  featurein = stdin;
  imagein = stdin;
  featureout = stdout;
  
  /*
   * Read information from input line:
   */

  switch(argc) {
  case 4:
    if (strcmp(argv[3], "-") == 0) {
      featureout = stdout;
    }
    else {
      if ( (featureout = fopen(argv[3],"wb")) == NULL ) {
	printf("Can't open %s.",argv[3]);
	exit(0);
      }
    }
  case 3:
    if (strcmp(argv[2], "-") == 0) {
      imagein = stdin;
    }
    else {
      if ( (imagein = fopen(argv[2],"rb")) == NULL ) {
	printf("Can't open %s.",argv[2]);
	exit(0);
      }
    }
  case 2:
    if ( (featurein = fopen(argv[1],"r")) == NULL ) {
      printf("Can't open %s.",argv[1]);
      exit(0);
    }
    break;
  default:
    printf("\n Usage: intdfim <diffuse features in> "
	   "<image in> <diffuse features out> \n\n");
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
   * Read diffuse features file:
   */
  
  imdiff->infile = featurein;
  if (lreaddf(imdiff) != 0) {
    perror(imdiff->error_msg);
    goto CloseShop;
  }
  
  /*
   * Integrate diffuse features:
   */

  if (lintdfim(imdiff) != 0) {
    perror(imdiff->error_msg);
    goto CloseShop;
  }

  /*
   * Write the output diffuse features file:
   */
  
  imdiff->outfile = featureout;
  if(lwritedf(imdiff) != 0) {
    perror(imdiff->error_msg);
    goto CloseShop;
  }
  
 CloseShop:
  
  lfreeim(imdiff);
  
  /*
   * Close files:
   */

  fclose(imagein);
  fclose(featurein);
  fclose(featureout);
}

