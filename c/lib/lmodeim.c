/* LMODEIM.C - Remove the peaks from an image by mode filetering using
               a specified mask size.
   
   Author: Mike Wall 
   Date: 2/2/95
   Version: 1.
   
   "modeim <image in> <image out> <mask size> <bin size>"

   Input is ascii coordinates file.  Output is 16-bit 
   image of specified size (1024 x 1024 default).

   */

#include<mwmask.h>
int lmodeim(DIFFIMAGE *imdiff) 
{
  
  RCCOORDS_DATA 
    half_height,
    half_width,
    n, 
    m, 
    r, 
    c; 

  size_t
    i,
    j,
    mode_value,
    mode_ct,
    k,
    l,
    max_count=0,
    avg_max_count_count = 0,
    *count,
    *count_pointer,
    imd_index = 0,
    index = 0; 
  
  float
    avg_max_count = 0;

  IMAGE_DATA_TYPE
    *image;

  int
    return_value = 0;
  
  /* 
   * Allocate working image: 
   */ 
  
  image = (IMAGE_DATA_TYPE *)calloc(imdiff->image_length, 
				      sizeof(IMAGE_DATA_TYPE)); 
  count = (size_t *)calloc(65537,sizeof(size_t));
  count_pointer = (size_t *)calloc((imdiff->mode_height+1) *
				(imdiff->mode_width+1), 
				sizeof(size_t));
  if (!image || !count || !count_pointer) {
    sprintf(imdiff->error_msg,"\nLMODEIM:  Couldn't allocate arrays.\n\n");
    return_value = 1;
    goto CloseShop;
  }


  half_height = imdiff->mode_height / 2;
  half_width = imdiff->mode_width / 2;
  index = 0;
  for (j=0; j<imdiff->vpixels; j++) {
    for (i=0; i<imdiff->hpixels; i++) {
      if (imdiff->image[index] != imdiff->ignore_tag) {
	mode_value=0;
	max_count=0;
	l=0;
	for(n=-half_height; n<=half_height; n++) {
	  r = j + n;
	  for(m=-half_width; m<=half_width; m++) {
	    c = i + m;
	    if (!((r < 0) || (r > imdiff->vpixels) || (c < 0) ||       
		  (c > imdiff->hpixels))) {
	      imd_index = index + n*imdiff->hpixels + m;
	      if ((imdiff->image[imd_index] != imdiff->overload_tag) &&
		  (imdiff->image[imd_index] != imdiff->ignore_tag)) {
		count_pointer[l]=(imdiff->image[imd_index] - 
				    (imdiff->image[imd_index] % 
				     imdiff->mode_binsize) + 32768);
		count[count_pointer[l++]]++;
	      }
	    }
	  }
	}
	if (l==0) {
	  /*	printf("\nl=0 index=%ld\n",index);/***/
	  image[index]=imdiff->ignore_tag;
	}
	else {
	  for(k=0;k<=l-1;k++) {
	    if (count[count_pointer[k]] == max_count) {
	      mode_value = (size_t)((float)(mode_ct*mode_value +
					    count_pointer[k])/ 
				    (float)(mode_ct+1));
	      mode_ct++;
	    }
	    else if (count[count_pointer[k]] > max_count) {
	      mode_value = count_pointer[k];
	      max_count = count[count_pointer[k]];
	      mode_ct = 1;
	    }
	  }
	  for(k=0;k<=l-1;k++) count[count_pointer[k]] = 0;
	  image[index] = (IMAGE_DATA_TYPE)mode_value - 32768;
	  avg_max_count = (avg_max_count*avg_max_count_count +
			   max_count) / (float)(avg_max_count_count + 1);
	  avg_max_count_count++;
	}
      }
      else {
	image[index] = imdiff->image[index];
      }
      index++;
      /*if (index % 1000 == 0) printf("%d\n",index);/***/
    }
  }
  for(index=0;index<imdiff->image_length; index++) {
    imdiff->image[index] = image[index];
  }
  printf("avg_max_count = %f\n\n",avg_max_count);/***/
  free((size_t *)count);/***/
  free((size_t *)count_pointer);/***/
  free((IMAGE_DATA_TYPE *)image);/***/
  CloseShop:
  return(return_value);
}







