
#include <opencv/cv.h>
#include "texture_tools.h"
#include "face.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

// Code to support texture filters
// Derek Hoiem 2003.05.21
int
texture_test_entire_image(IplImage * img, texture_args_t *targs, bbox_list_t *blist)
{

    /*
     * first process entire image 
     */
    double          feature_val[NUM_LAP_PYR_LEVELS * TEXTURE_MAX_CHANNELS];

    double          variance[NUM_LAP_PYR_LEVELS*TEXTURE_MAX_CHANNELS];
    double          mean [NUM_LAP_PYR_LEVELS*TEXTURE_MAX_CHANNELS];

    double          ave_sample_mean_diff[NUM_LAP_PYR_LEVELS *
                                         TEXTURE_MAX_CHANNELS];
    double          mean_sample_diff[NUM_LAP_PYR_LEVELS * TEXTURE_MAX_CHANNELS];
    double          distance;
    double          min_distance;   // min distance for one window from all samples
    double          min_min_distance = -1.0;    // min distance for all
    int             passed = 0;
    int             i, s, x, y; 
    bbox_t*         bbox;
    int		    quit_on_pass = 0; //quits as soon as its known that the image passes
    int             extra_pixels_w = img->width % (1 << NUM_LAP_PYR_LEVELS);
    int             extra_pixels_h = img->height % (1 << NUM_LAP_PYR_LEVELS);
    cvSetImageROI(img, cvRect(0, 0, img->width - extra_pixels_w,
                         img->height - extra_pixels_h));

    /**
     * Find weighting for the mahalanobis distance metric for a diagonal covariance matrix
     **/
    if (targs->num_samples >= 2) {
        for (i = 0; i < NUM_LAP_PYR_LEVELS * targs->num_channels; i++) {

	  mean[i] = 0.0;
	  variance[i] = 0.0;	  

	  for (s=0; s< targs->num_samples; s++) {
	    mean[i] += targs->sample_values[s][i];
	  }
	  mean[i] = mean[i] / targs->num_samples;

	  for (s=0; s< targs->num_samples; s++) {
	    variance[i] += pow((targs->sample_values[s][i]-mean[i]), 2.0);
	  }
	  
	  // unbiased population estimation of variance
	  variance[i] = variance[i]/(targs->num_samples-1);

	  // old code below
/*             mean_sample_diff[i] = 0.0; */
/*             for (s = 0; s < targs->num_samples; s++) { */
/*                 mean_sample_diff[i] += targs->sample_values[s][i]; */
/*             } */
/*             mean_sample_diff[i] /= (double) targs->num_samples; */
/*             ave_sample_mean_diff[i] = 0; */
/*             for (s = 0; s < targs->num_samples; s++) { */
/*                 ave_sample_mean_diff[i] += */
/*                     fabs(targs->sample_values[s][i] - mean_sample_diff[i]); */
/*             } */
/*             ave_sample_mean_diff[i] /= (double) targs->num_samples; */
        }

    }

    /*
     * set default weights for if only one sample is given Note: these may
     * not be optimal.. based on grass and waves 
     */
    if (targs->num_samples <= 1) {
        for (i = 0; i < NUM_LAP_PYR_LEVELS * targs->num_channels; i++) {
            if (i / targs->num_channels == 0) {
                ave_sample_mean_diff[i] = 2.6;
            }
            if (i / targs->num_channels == 1) {
                ave_sample_mean_diff[i] = 90.0;
            }
            if (i / targs->num_channels == 2) {
                ave_sample_mean_diff[i] = 4.5;
            }
            if (i / targs->num_channels == 3) {
                ave_sample_mean_diff[i] = 90.0;
            }
            if (i / targs->num_channels == 4) {
                ave_sample_mean_diff[i] = 10.0;
            }
            if (i / targs->num_channels > 4) {
                ave_sample_mean_diff[i] = 50.0;
            }
        }
    }

    /*
     * test each subwindow 
     */
    for (x = 0; x + targs->box_width < img->width; x += targs->step) {
        for (y = 0; y + targs->box_height < img->height; y += targs->step) {
            texture_get_lap_pyr_features_from_subimage(img, targs->num_channels, x,
                                                       y, targs->box_width,
                                                       targs->box_height,
                                                       feature_val);
            distance = 0.0;
            min_distance = -1.0;
	    for (i = 0; i < NUM_LAP_PYR_LEVELS * targs->num_channels; i++) {
	      // mahalanobis distance metric
	      distance += ((feature_val[i] - mean[i])*(feature_val[i]-mean[i]) /
			   variance[i]);
	    }
	    min_distance = distance;

	    min_distance = min_distance / targs->num_channels;
	    // old code
/*             for (s = 0; s < targs->num_samples; s++) { */
/*                 distance = 0.0; */
/*                 for (i = 0; i < NUM_LAP_PYR_LEVELS * targs->num_channels; i++) { */
/*                     distance += (feature_val[i] - targs->sample_values[s][i]) * */
/*                         (feature_val[i] - targs->sample_values[s][i]) / */
/*                         (ave_sample_mean_diff[i] * ave_sample_mean_diff[i]); */
/*                 } */
/* 	    distance = sqrt(distance) / targs->num_channels; */
/* 	    if ((distance < min_distance) || (min_distance == -1.0)) { */
/* 	      min_distance = distance; */
/* 	    } */
/* 	} */

#ifdef	XXX
            if ((min_distance < min_min_distance)
                || (min_min_distance == -1.0)) {
                *min_x = x;
                *min_y = y;
                min_min_distance = min_distance;
            }
#endif


            if (min_distance <= targs->max_distance) {
                passed++;
		bbox = (bbox_t *)malloc(sizeof(*bbox));
		assert(bbox != NULL);
		bbox->min_x = x;
		bbox->min_y = y;
		bbox->max_x = x + targs->box_width;	/* XXX scale */
		bbox->max_y = y + targs->box_height; /* XXX scale */
		bbox->distance = min_distance;
		TAILQ_INSERT_TAIL(blist, bbox, link);

		/* XXX put on list of items */
                /* 		
			dst_img->imageData[dst_img->widthStep * y + x] =
			(char) (min_distance / threshold * 100);
		*/
                if (quit_on_pass && (passed >= targs->min_matches)) {
                    goto done;
                }
            }
        }
    }

done:

    cvResetImageROI(img);
    return (passed);
}

/*
 * gets features from a single subwindow 
 */
void
texture_get_lap_pyr_features_from_subimage(IplImage * img,
                                           int num_channels,
                                           int min_x,
                                           int min_y,
                                           int box_width,
                                           int box_height,
                                           double *feature_values)
{
    IplImage       *gaussianIm[NUM_LAP_PYR_LEVELS + 1];
    IplImage       *laplacianIm_tmp[NUM_LAP_PYR_LEVELS + 1];
    IplImage       *laplacianIm[NUM_LAP_PYR_LEVELS + 1];
    CvRect          roi;
    CvRect          old_roi;
    CvScalar        response;
    int             gindex;
    int             i,
                    j;
    // CvScalar rgbsum;

    old_roi = cvGetImageROI(img);
    cvSetImageROI(img, cvRect(min_x, min_y, box_width, box_height));


    /*
     * 0 is coarsest level, NUM_LAP_PYR_LEVELS is finest level 0 level is not 
     * used for features 
     */

    /*
     * form gaussian pyramid 
     */
    gaussianIm[NUM_LAP_PYR_LEVELS] = img;

    for (i = 1; i <= NUM_LAP_PYR_LEVELS; i++) {
        gindex = NUM_LAP_PYR_LEVELS - i;
        roi = cvGetImageROI(gaussianIm[gindex + 1]);
        gaussianIm[NUM_LAP_PYR_LEVELS - i] =
            cvCreateImage(cvSize(roi.width / 2, roi.height / 2), IPL_DEPTH_8U,
                          num_channels);
        cvPyrDown(gaussianIm[gindex + 1], gaussianIm[gindex],
                  CV_GAUSSIAN_5x5);
    }

    /*
     * form laplacian pyramid
     */
    laplacianIm[0] = gaussianIm[0];
    laplacianIm_tmp[0] = gaussianIm[0];
    for (i = 1; i <= NUM_LAP_PYR_LEVELS; i++) {
        roi = cvGetImageROI(laplacianIm[i - 1]);
        laplacianIm_tmp[i] =
            cvCreateImage(cvSize(roi.width * 2, roi.height * 2), IPL_DEPTH_8U,
                          num_channels);
        laplacianIm[i] =
            cvCreateImage(cvSize(roi.width * 2, roi.height * 2), IPL_DEPTH_8U,
                          num_channels);
        cvResize(laplacianIm[i - 1], laplacianIm_tmp[i], CV_INTER_NN);
        printf("comput peramid \n");
        printf("s1 ht %d s1 wd %d s1 ht %d s2 wd %d \n",
               gaussianIm[i]->height, gaussianIm[i]->width,
               laplacianIm_tmp[i]->height, laplacianIm_tmp[i]->width);

        cvAbsDiff(gaussianIm[i], laplacianIm_tmp[i], laplacianIm[i]);
        printf("after peramid \n");
    }
    /*
     * calculate response 
     */
    // printf("feature values: ");
    for (i = 1; i <= NUM_LAP_PYR_LEVELS; i++) {
        response = cvSum(laplacianIm[i]);
        for (j = 0; j < num_channels; j++) {
            feature_values[(i - 1) * num_channels + j] =
                response.val[j] / (laplacianIm[i]->width *
                                   laplacianIm[i]->height);
        }
    }
    /*
     * printf("\n"); rgbsum = cvSum(img); int num_pixels = box_width *
     * box_height; printf("rgb: (%f, %f, %f)\n", rgbsum.val[0]/num_pixels,
     * rgbsum.val[1]/num_pixels, rgbsum.val[2]/num_pixels); 
     */

    cvSetImageROI(img, old_roi);

    for (i = 0; i < NUM_LAP_PYR_LEVELS + 1; i++) {
        if (i != NUM_LAP_PYR_LEVELS) {
            cvReleaseImage(&gaussianIm[i]); // don't release original image
        }
        if (i != 0) {
            cvReleaseImage(&laplacianIm[i]);
            cvReleaseImage(&laplacianIm_tmp[i]);
        }
    }

}