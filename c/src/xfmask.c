/* XFMASK.C - Tag pixels as ignored in one image which in a second image are
		tagged as either overloads or ignored.  Positions are 
		reffered to origin.
   
   Author: Mike Wall
   Date: 4/11/94
   Version: 1.
   
   "xfmask <input image 1> <input image 2> <output image>" 

   Input two diffraction images in TIFF TV6 format.  Output is two rfiles
   with scale and offset as function of radius.

   */

#include<mwmask.h>

int main(int argc, char *argv[])
{
  FILE
	*imagein1,
	*imagein2,
	*imageout;
  
  size_t
    i,
    num_wrote,
    num_read;

  DIFFIMAGE 
	*imdiff1,
	*imdiff2;

  int
	got_r2 = 0,
	got_c2 = 0;

  struct rccoords
	origin1,
	origin2;

/*
 * Set input line defaults:
 */
	
	imagein1 = stdin;
	imagein2 = stdin;
	imageout = stdout;
        origin1.r = DEFAULT_IMAGE_ORIGIN;
	origin1.c = DEFAULT_IMAGE_ORIGIN;
	origin2.r = DEFAULT_IMAGE_ORIGIN;
	origin2.c = DEFAULT_IMAGE_ORIGIN;

/*
 * Read information from input line:
 */
	switch(argc) {
		case 8:
			if (strcmp(argv[7], "-") == 0) {
				imageout = stdout;
			}
			else {
			 if ( (imageout = fopen(argv[7],"wb")) == NULL ) {
				printf("Can't open %s.",argv[7]);
				exit(0);
			 }
			}
                case 7:
			origin2.r = (RCCOORDS_DATA)atoi(argv[6]);
			got_r2 = 1;
		case 6:
			origin2.c = (RCCOORDS_DATA)atoi(argv[5]);
			got_c2 = 1;
		case 5:
			if (strcmp(argv[4], "-") == 0) {
				imagein2 = stdin;
			}
			else {
			 if ( (imagein2 = fopen(argv[4],"rb")) == NULL ) {
				printf("Can't open %s.",argv[4]);
				exit(0);
			 }
			}
                case 4:
			origin1.r = (RCCOORDS_DATA)atoi(argv[3]);
			if (got_r2 == 0) origin2.r = origin1.r;
		case 3:
			origin1.c = (RCCOORDS_DATA)atoi(argv[2]);
			if (got_c2 == 0) origin2.c = origin1.c;

		case 2:
			if (strcmp(argv[1], "-") == 0) {
				imagein1 = stdin;
			}
			else {
			 if ( (imagein1 = fopen(argv[1],"rb")) == NULL ) {
				printf("Can't open %s.",argv[1]);
				exit(0);
			 }
			}
			break;
		default:
			printf("\n Usage: xfmask <input image 1> <x origin 1> "
				"<y origin 1> <input image 2> <x origin 2> "
				"<y origin 2> <output image>\n\n");
			exit(0);
	}
/*
 * Initialize diffraction images:
 */

  if (((imdiff1 = linitim(1)) == NULL) || ((imdiff2 = linitim(1)) == NULL)) {
    perror("Couldn't initialize diffraction images.\n\n");
    exit(0);
  }

/*
 * Set main defaults:
 */

  imdiff1->origin = origin1;
  imdiff2->origin = origin2;


/*
 * Read diffraction images:
 */

  imdiff1->infile = imagein1;
  if (lreadim(imdiff1) != 0) {
    perror(imdiff1->error_msg);
    goto CloseShop;
  }

  imdiff2->infile = imagein2;
  if (lreadim(imdiff2) != 0) {
    perror(imdiff2->error_msg);
    goto CloseShop;
  }

  if (lxfmask(imdiff1,imdiff2) != 0) {
    perror(imdiff2->error_msg);
    goto CloseShop;
  }

/*
 * Write the output image:
 */

  imdiff1->outfile = imageout;
  if(lwriteim(imdiff1) != 0) {
    perror(imdiff1->error_msg);
    goto CloseShop;
  }

 CloseShop:
  
/*
 * Free allocated memory:
 */

  lfreeim(imdiff1);
  lfreeim(imdiff2);

/*
 * Close files:
 */
  
  fclose(imagein1);
  fclose(imagein2);
  fclose(imageout);
}

