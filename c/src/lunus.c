/* LUNUS.C - Diffuse scattering analysis and modeling.
   
   Author: Mike Wall  
   Date: 6/13/2017
   Version: 1.
   Date: 4/17/2019
   Version: 1.1 (refactored to create lprocimlt() without MPI inside)
   
   "lunus <input_deck>"

   Apply lunus diffuse scattering methods according to instructions in input_deck
   
   */

#include<lunus.h>

int lsetprocmodelt(LAT3D *lat,const int mode)
{
  if (mode == 0 || mode == 1) {
    lat->procmode = mode;
  } else {
    perror("Mode must be 0 or 1\n");
    exit(1);
  }
}

int main(int argc, char *argv[])
{
  FILE
    *deckin,
    *imagein,
    *lunusout,
    *scaleout;
  
  char
    inputdeck[10000],
    *imageinpath = NULL,
    *lunusoutpath = NULL,
    *scaleoutpath = NULL,
    *deck = NULL,    
    *amatrix_format = NULL,
    *amatrix_path = NULL,
    *xvectors_path = NULL,
    *writevtk_str = NULL,
    *lattice_dir = NULL,
    *diffuse_lattice_prefix = NULL,
    *spacegroup = NULL,
    *imagelist_name = NULL,
    *jsonlist_name = NULL,
    *imagelist[20000],
    *bkglist[20000],
    **matched_pair,
    *jsonlist[20000],
    error_msg[LINESIZE];

  void *buf;

  int
    writevtk;

  struct xyzcoords *xvectors_cctbx = NULL,*xvectors = NULL, *Hlist, *dHlist;

  IJKCOORDS_DATA *ilist, *jlist, *klist;

  struct xyzmatrix at[20000];

  size_t
    index,
    i,
    j,
    num_images;

  IJKCOORDS_DATA
    i0, j0, k0;

  float
    normim_tilt_x=0.0,
    normim_tilt_y=0.0;

  DIFFIMAGE 
    *imdiff = NULL, *imdiff_bkg = NULL, *imdiff_corrected = NULL, *imdiff_scale = NULL, *imdiff_scale_ref = NULL;

  LAT3D
    *lat;

  size_t 
    str_length,
    num_read;

  MPIVARS
    *mpiv;


  // Initialize MPI


  mpiv = (MPIVARS *)malloc(sizeof(MPIVARS));
  mpiv->argc = argc;
  mpiv->argv = argv;

  linitMPI(mpiv);

  /*
   * Read information from input line:
   */
  switch(argc) {
  case 2:
    strcpy(inputdeck,argv[1]);
    break;
  default:
    printf("\n Usage: lunus <input deck>\n\n");
    lfinalMPI(mpiv);
    exit(0);
  }

#ifdef DEBUG
  //	printf("LUNUS: reading input deck\n");
#endif
  // Read input deck into buffer

  if (mpiv->my_id == 0) {
    num_read = lreadbuf((void **)&deck,inputdeck);
  }

  // Broadcast num_read and the input deck

  lbcastBufMPI((void *)&num_read,(int)sizeof(size_t),0,mpiv);

  if (mpiv->my_id != 0) {
    deck = (char *)calloc(num_read+1,sizeof(char));
  }

  lbcastBufMPI((void *)deck,(int)num_read,0,mpiv);

#ifdef DEBUG
  //	printf("Length of input deck = %ld\n",num_read);
#endif
  // Parse input deck

  if (strstr(deck,"\nwritevtk") == NULL) {
    writevtk_str = (char *)malloc(strlen("False")+1);
    strcpy(writevtk_str,"False");
  } else {
    writevtk_str = lgettag(deck,"\nwritevtk");
  }

  if (strcmp(writevtk_str,"True")==0) {
    writevtk=1;
  } else {
    writevtk=0;
  }

  if (strstr(deck,"\nimagelist_name") != NULL) {
    imagelist_name=lgettag(deck,"\nimagelist_name");
  }

  if (strstr(deck,"\njsonlist_name") != NULL) {
    jsonlist_name=lgettag(deck,"\njsonlist_name");
  }

  if (strstr(deck,"\nspacegroup") != NULL) {
    spacegroup=lgettag(deck,"\nspacegroup");
  }

  if (strstr(deck,"\nxvectors_path") == NULL) {
    perror("Must provide xvectors_path for integration\n");
    exit(1);
  } else {
    xvectors_path=lgettag(deck,"\nxvectors_path");
  }

  if (strstr(deck,"\namatrix_format") != NULL) {
    amatrix_format=lgettag(deck,"\namatrix_format");
  }

  if (strstr(deck,"\nlattice_dir") == NULL) {
    lattice_dir = (char *)malloc(strlen(".")+1);
    strcpy(lattice_dir,".");
  } else {
    lattice_dir=lgettag(deck,"\nlattice_dir");
  }

  if (strstr(deck,"\ndiffuse_lattice_dir") != NULL) {
    lattice_dir = lgettag(deck,"\ndiffuse_lattice_dir");
  }

  if (strstr(deck,"\ndiffuse_lattice_prefix") == NULL) {
    diffuse_lattice_prefix = (char *)malloc(strlen("diffuse_lunus")+1);
    strcpy(diffuse_lattice_prefix,"diffuse_lunus");
  } else {
    diffuse_lattice_prefix=lgettag(deck,"\ndiffuse_lattice_prefix");
  }

  // Get file lists and A matrices

  if (mpiv->my_id == 0) {

    if (imagelist_name == NULL && jsonlist_name == NULL) {
	    
      perror("Can't generate image list due to NULL value of imagelist_name or jsonlist_name.\n");
	    
    } else if (jsonlist_name != NULL) {

      // Obtain image filenames and crystal orientations from .json files

      FILE *f;

      if ((f = fopen(jsonlist_name,"r")) == NULL) {
	printf("Can't open %s.\n",jsonlist_name);
	exit(1);
      }
	  
      size_t bufsize = LINESIZE;
      char *json_name;

      i = 0;

      json_name = (char *)calloc(LINESIZE+1,sizeof(char));

      int chars_read;

      while ((chars_read = getline(&json_name,&bufsize,f)) != -1) {

	json_name[chars_read-1]=0;

	if ((readExptJSON(&at[i],&imagelist[i],&bkglist[i],json_name)) != 0) {
	  printf("Skipping %s, unable to read\n",buf);
	} else {
	  //	      printf("%s,%s\n",imagelist[i],bkglist[i]);
	  i++;
	}

      }

      num_images = i;

    } else {

      // Read image list and A matrix files

      FILE *f;

      if ((f = fopen(imagelist_name,"r")) == NULL) {
	printf("Can't open %s.\n",imagelist_name);
	exit(1);
      }
	  
      size_t bufsize = LINESIZE;
      char *buf;

      i = 0;

      buf = (char *)malloc(LINESIZE+1);

      int chars_read;

      while ((chars_read = getline(&buf,&bufsize,f)) != -1) {
	    
	// Read amatrix
	    
	if ((readAmatrix(&at[i],amatrix_format,i+1) == -1)) {
	  printf("Missing amatrix file %s. Skipping frame %d.\n",amatrix_path,i);
	} else {
	      
	      
	  buf[chars_read-1]=0;
	      
	  str_length = snprintf(NULL,0,"%s",buf);
	      
	  imagelist[i] = (char *)malloc(str_length+1);
	      
	  sprintf(imagelist[i],"%s",buf);
	      
	  //	    printf("%s\n",imagelist[i]);
	      
	  i++;
	}
	    
      }
      num_images = i;
    }
  }

  // Broadcast file lists and A matrices from rank 0 to other MPI ranks

  lbcastBufMPI((void *)&num_images,sizeof(size_t),0,mpiv);
  int il_sz[num_images];
  int bl_sz[num_images];
  if (mpiv->my_id == 0) {
    for (i=0;i<num_images;i++) {
      il_sz[i] = strlen(imagelist[i])+1;
      bl_sz[i] = strlen(bkglist[i])+1;
    }
  }
  lbcastBufMPI((void *)&il_sz,sizeof(int)*num_images,0,mpiv);
  lbcastBufMPI((void *)&bl_sz,sizeof(int)*num_images,0,mpiv);
  for (i=0;i<num_images;i++) {
    if (mpiv->my_id != 0) {
      imagelist[i] = (char *)malloc(il_sz[i]);
      bkglist[i] = (char *)malloc(bl_sz[i]);
    }
    lbcastBufMPI((void *)imagelist[i],il_sz[i],0,mpiv);
    lbcastBufMPI((void *)bkglist[i],bl_sz[i],0,mpiv);
  }

  // Broadcast the list of A matrices to all MPI ranks

  lbcastBufMPI((void *)&at,sizeof(struct xyzmatrix)*num_images,0,mpiv);	

  // Initialize the 3D dataset for this MPI rank

  lat = linitlt();

  // Get lattice params from input deck

  lat->params = deck;

  lsetparamslt(lat);

  /*
   * Initialize diffraction image:
   */


  if ((imdiff = linitim()) == NULL) {
    perror("Couldn't initialize diffraction image.\n\n");
    exit(0);
  }

  // Define parameters from input deck

  imdiff->params = deck;

  lsetparamsim(imdiff);

  // Set mpi variables for imdiff

  imdiff->mpiv = mpiv;

  // Read the xvectors on rank 0
  if (imdiff->mpiv->my_id == 0) {
    num_read = lreadbuf((void **)&xvectors_cctbx,xvectors_path);
    if (num_read != 3*imdiff->image_length*sizeof(float)) {
      perror("LUNUS: Number of xvectors differs from number of pixels in image.\n");
      exit(1);
    }
#ifdef DEBUG		
    printf("SAMPLES\n");
    for (j=50000;j<50010;j++) {
      printf("(%f, %f, %f): %d\n",xvectors_cctbx[j].x,xvectors_cctbx[j].y,xvectors_cctbx[j].z, imdiff_corrected->image[j]);
    }
    printf("\n");
#endif		
    // Reorder the xvectors (transpose)

    index = 0;

    if (xvectors != NULL) free(xvectors);

    xvectors = (struct xyzcoords *)malloc(num_read);		

    if (num_read != sizeof(struct xyzcoords)*imdiff->image_length) {
      perror("Number of xvectors not equal to image length. Exiting.\n");
      exit(1);;
    }
		   
    size_t k;

    for (j=0; j<imdiff->vpixels; j++) {
      for (k=0; k<imdiff->hpixels; k++) {
	// The following conditional is needed to prevent a segfault-inducing Intel 18.X optimization error:
	//		    if (index == 0) printf("");
	xvectors[index] = xvectors_cctbx[k*imdiff->vpixels + j];
	index++;
      }
    }
  }

  // Broadcast the xvectors to other ranks
  lbcastBufMPI((void *)&num_read,sizeof(size_t),0,imdiff->mpiv);
  if (imdiff->mpiv->my_id != 0) {
    if (xvectors != NULL) free(xvectors);
    xvectors = (struct xyzcoords *)malloc(num_read);
  }
  lbcastBufMPI((void *)xvectors,num_read,0,imdiff->mpiv);

  imdiff->xvectors = xvectors;

  // Process all of the images

  int ct=0;

  // Set up the processing using a reference image

  // Process the images on this rank yielding a partial sum for the 3D dataset

  for (i=mpiv->my_id+1;i<=num_images;i=i+mpiv->num_procs) {

    /*
     * Read diffraction image:
     */
	  
    if ( (imagein = fopen(imagelist[i-1],"rb")) == NULL ) {
      printf("Can't open %s.",imagelist[i-1]);
      exit(0);
    }

    imdiff->infile = imagein;
    if (lreadim(imdiff) != 0) {
      perror(imdiff->error_msg);
      exit(0);
    }

    fclose(imagein);

    // Associate an A matrix with this image

    imdiff->amatrix = at[i-1];

    // Define image parameters from input deck

    imdiff->params = deck;

    lsetparamsim(imdiff);

    // Subtract background image if available:

    int needs_bkgsub = 0;

    if (strlen(bkglist[i-1]) > 0) {

#ifdef DEBUG
      printf("Matched pair:%s,%s\n",imagelist[i-1],bkglist[i-1]);
#endif
      needs_bkgsub = 1;
      if ( (imagein = fopen(bkglist[i-1],"rb")) == NULL ) {
	printf("Can't open %s.",bkglist[i-1]);
	exit(0);
      }
      imdiff_bkg->infile = imagein;
      if (lreadim(imdiff_bkg) != 0) {
	perror(imdiff_bkg->error_msg);
	exit(0);
      }
      fclose(imagein);
    }

    if (needs_bkgsub == 1) lbkgsubim(imdiff,imdiff_bkg);

    // If this is the first time through, initialize the processing and set the reference image

    if (i == mpiv->my_id + 1) {

      // First broadcast the reference image to all ranks

      DIFFIMAGE *imdiff_ref;

      imdiff_ref = linitim();
      lcloneim(imdiff_ref,imdiff);

      lbarrierMPI(imdiff->mpiv);

      // Broadcast the reference image data
      lbcastImageMPI(imdiff_ref->image,imdiff_ref->image_length,0,imdiff->mpiv);
      
      // Broadcast the pedestal as well -- this is critical
      lbcastBufMPI((void *)&imdiff_ref->value_offset,sizeof(IMAGE_DATA_TYPE),0,imdiff->mpiv);

      // Run the processing method in initialization mode (0) supplying the reference image

      lsetprocmodelt(lat,0);
      lat->imdiff = imdiff_ref;
      lprocimlt(lat);

    }

    // Run the processing method in accumulation mode (1)

    lsetprocmodelt(lat,1);    
    lat->imdiff = imdiff;
    lprocimlt(lat);

  }

  // Merge the data and counts

  LATTICE_DATA_TYPE *latsum;
  size_t *latctsum;
	  
  latsum = (LATTICE_DATA_TYPE *)calloc(lat->lattice_length,sizeof(LATTICE_DATA_TYPE));
  latctsum = (size_t *)calloc(lat->lattice_length,sizeof(size_t));
	  
  lreduceSumLatticeMPI(lat->lattice,latsum,lat->lattice_length,0,mpiv);
  lreduceSumLatctMPI(lat->latct,latctsum,lat->lattice_length,0,mpiv);
	  
  // Calculate the mean on the root rank and output the result
	  
  if (mpiv->my_id == 0) {
    for (j=0; j<lat->lattice_length; j++) {
      if (latctsum[j] != 0) {
	lat->lattice[j] = latsum[j]/(float)latctsum[j];
	if isnan(lat->lattice[j]) lat->lattice[j] = lat->mask_tag;
      } else {
	lat->lattice[j] = lat->mask_tag;
      }
    }
	    
    // output the result
	    
    str_length = snprintf(NULL,0,"%s/%s.lat",lattice_dir,diffuse_lattice_prefix);
	    
    char *latticeoutpath;
	    
    latticeoutpath = (char *)malloc(str_length+1);
	    
    sprintf(latticeoutpath,"%s/%s.lat",lattice_dir,diffuse_lattice_prefix);
	    
    //	  printf("imageinpath = %s\n",imageinpath);
	    
    FILE *latticeout;
	    
    if ( (latticeout = fopen(latticeoutpath,"wb")) == NULL ) {
      printf("Can't open %s.\n",latticeoutpath);
      exit(1);
    }
    lat->outfile=latticeout;
    lwritelt(lat);
    fclose(latticeout);
    // Write .vtk if requested

    if (writevtk != 0) {

      str_length = snprintf(NULL,0,"%s/%s.vtk",lattice_dir,diffuse_lattice_prefix);
	    
      char *vtkoutpath;
	      
      vtkoutpath = (char *)malloc(str_length+1);
	      
      sprintf(vtkoutpath,"%s/%s.vtk",lattice_dir,diffuse_lattice_prefix);
	      
      //	  printf("imageinpath = %s\n",imageinpath);
	      
      FILE *vtkout;
	      
      if ( (vtkout = fopen(vtkoutpath,"w")) == NULL ) {
	printf("Can't open %s.\n",vtkoutpath);
	exit(1);
      }
	      
      lat->outfile=vtkout;
      if (spacegroup != NULL) {
	strcpy(lat->space_group_str,spacegroup);
      } else {
	strcpy(lat->space_group_str,"Unknown");
      }
      lwritevtk(lat);
    }
  }
  lfinalMPI(mpiv);

}

