/*
 * 	SnapFind (Release 0.9)
 *      An interactive image search application
 *
 *      Copyright (c) 2002-2005, Intel Corporation
 *      All Rights Reserved
 *
 *  This software is distributed under the terms of the Eclipse Public
 *  License, Version 1.0 which can be found in the file named LICENSE.
 *  ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS SOFTWARE CONSTITUTES
 *  RECIPIENT'S ACCEPTANCE OF THIS AGREEMENT
 */

/*
 *  Copyright (c) 2006 Larry Huston <larry@thehustons.net>
 *
 *  This software is distributed under the terms of the Eclipse Public
 *  License, Version 1.0 which can be found in the file named LICENSE.
 *  ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS SOFTWARE CONSTITUTES
 *  RECIPIENT'S ACCEPTANCE OF THIS AGREEMENT
 */


/*
 * face detector 
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <opencv/cvaux.h>

#include "lib_filter.h"
#include "lib_log.h"
#include "fil_ocv_face.h"
#include "rgb.h"
#include "opencv_face.h"
#include "opencv_face_tools.h"
#include "lib_results.h"
#include "lib_sfimage.h"



int
f_init_opencv_fdetect(int numarg, char **args, int blob_len, void *blob_data,
 			const char *fname, void **fdatap)
{

	opencv_fdetect_t *fconfig;
	CvHaarClassifierCascade *cascade;


	fconfig = (opencv_fdetect_t *) malloc(sizeof(*fconfig));
	assert(fconfig != NULL);

	fconfig->name = strdup(args[0]);
	assert(fconfig->name != NULL);
	fconfig->scale_mult = atof(args[1]);
	fconfig->xsize = atoi(args[2]);
	fconfig->ysize = atoi(args[3]);
	fconfig->stride = atoi(args[4]);
	/*
	 * XXX skip 5 for now ?? 
	 */
	fconfig->support = atoi(args[6]);

	if (fconfig->scale_mult <= 1) {
		lf_log(LOGL_TRACE,
		       "scale multiplier must be > 1; got %f\n", fconfig->scale_mult);
		exit(1);
	}

	cascade = cvLoadHaarClassifierCascade("<default_face_cascade>",
	                                      cvSize(fconfig->xsize, fconfig->ysize));
	/* XXX check args */
	fconfig->haar_cascade = cvCreateHidHaarClassifierCascade(
	                            cascade, 0, 0, 0, 1);
	cvReleaseHaarClassifierCascade(&cascade);

	*fdatap = fconfig;
	return (0);
}


int
f_fini_opencv_fdetect(void *fdata)
{
	opencv_fdetect_t *fconfig = (opencv_fdetect_t *) fdata;

	cvReleaseHidHaarClassifierCascade(&fconfig->haar_cascade);
	free(fconfig->name);
	free(fconfig);
	return (0);
}


int
f_eval_opencv_fdetect(lf_obj_handle_t ohandle, void *fdata)
{
	int             	pass = 0;
	unsigned char *		dptr;
	RGBImage *		img;
	opencv_fdetect_t *	fconfig = (opencv_fdetect_t *) fdata;
	int             	err;
	bbox_list_t	    	blist;
	bbox_t *		cur_box;
	size_t			len;

	lf_log(LOGL_TRACE, "f_eval_opencv_fdetect: enter\n");

	/* get the img */
	err = lf_ref_attr(ohandle, RGB_IMAGE, &len, &dptr);
	assert(err == 0);
	img = (RGBImage *)dptr;


	TAILQ_INIT(&blist);
	pass = opencv_face_scan(img, &blist, fconfig);
	save_patches(ohandle, fconfig->name, &blist);

	while (!(TAILQ_EMPTY(&blist))) {
		cur_box = TAILQ_FIRST(&blist);
		TAILQ_REMOVE(&blist, cur_box, link);
		free(cur_box);
	}

	lf_log(LOGL_TRACE, "found %d faces\n", pass);
	return pass;
}




