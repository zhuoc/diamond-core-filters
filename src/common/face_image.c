/*
 *
 *
 *                          Diamond 1.0
 * 
 *            Copyright (c) 2002-2004, Intel Corporation
 *                         All Rights Reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *
 *    * Neither the name of Intel nor the names of its contributors may
 *      be used to endorse or promote products derived from this software 
 *      without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#ifdef linux
#include <values.h>
#else
#include <limits.h>
#endif
#include <assert.h>
#include <stdint.h>
// #include "lib_searchlet.h"
// #include "lib_filter.h"
// #include "face_tools.h"
#include "face_image.h"
#include "rgb.h"

#define	RED_SCALE	205
#define	RED_BASE	50
#define	GREEN_SCALE	205
#define	GREEN_BASE	50
#define	BLUE_SCALE	205
#define	BLUE_BASE	50



#define	SLOPE_BASE	125
#define	SLOPE_SCALE	125

// #define VERBOSE



static void
write_pixel(RGBImage * img, int x, int y, u_char r, u_char g, u_char b)
{
    ssize_t         offset;
    RGBPixel       *pixel;


    if (x < 0 || y < 0 || x >= img->width || y >= img->height) {
        return;
    }
    /*
     * assert(x >= 0); 
     */
    /*
     * assert(y >= 0); 
     */
    /*
     * assert(x < img->width); 
     */
    /*
     * assert(y < img->height); 
     */
    offset = x + (y * img->width);
    assert(offset < img->width * img->height);
    pixel = &img->data[offset];

    pixel->r = r;
    pixel->g = g;
    pixel->b = b;
    pixel->a = 255;
}

static void
modify_pixel(RGBImage * img, int x, int y, RGBPixel mask, RGBPixel value)
{
    ssize_t         offset;
    RGBPixel       *pixel;

    if (x < 0 || y < 0 || x >= img->width || y >= img->height) {
        return;
    }

    /*
     * assert(x >= 0); 
     */
    /*
     * assert(y >= 0); 
     */
    /*
     * assert(x < img->width); 
     */
    /*
     * assert(y < img->height); 
     */
    offset = x + (y * img->width);
    assert(offset < img->width * img->height);
    pixel = &img->data[offset];

    if (mask.r)
        pixel->r = value.r;
    if (mask.g)
        pixel->g = value.g;
    if (mask.b)
        pixel->b = value.b;
    if (mask.a)
        pixel->a = value.a;
}


RGBImage       *
image_gen_image_scale(RGBImage * data, int scale)
{
    RGBImage       *img;
    int             x, y;
    int             i, j;
    double          cum_r, cum_g, cum_b;
    double          avg_r, avg_g, avg_b;
    double          min_color, max_color;
    double          diff;
    u_char          red, green, blue;
    int             x_scale, y_scale;

    assert(data->rows);
    assert(data->columns);

    /* XXX compute new real size */
    img = (RGBImage *) malloc(data->nbytes);
    if (img == NULL) {
        /*
         * XXX log 
         */
        printf("XXX failed to allocate image \n");
        exit(1);
    }


    /*
     * First go through all the data to cmpute the number of rows,
     * max number of columns as well as the min and max values
     * for this image.
     */

    img->columns = data->columns;
    img->rows = data->rows;
    img->nbytes = data->nbytes;
    img->type = data->type;
    min_color = 0;
    max_color = 255;

    /*
     * Compute some constants that we will need.
     */
    diff = max_color - min_color;
#ifdef VERBOSE
    printf("min color %f max %f diff %f \n", min_color, max_color, diff);
    printf("orig row %d col %d \n", img->columns, img->rows);
#endif

    /*
     * scaling
     */
    img->columns = img->columns / scale;
    img->rows = img->rows / scale;
#ifdef VERBOSE
    printf("scaled row %d col %d \n", img->columns, img->rows);
#endif
    for (y = 0; y < img->rows; y++) {
        for (x = 0; x < img->columns; x++) {
            red = green = blue = 0;
            y_scale = y * scale;
            x_scale = x * scale;

            cum_r = cum_g = cum_b = 0;
            for (i = 0; i < scale; i++) {
                for (j = 0; j < scale; j++) {
                    RGBPixel       *pixel =
                        &data->data[(y_scale + j) * data->columns +
                                    (x_scale + i)];
                    cum_r += pixel->r;
                    cum_g += pixel->g;
                    cum_b += pixel->b;
                }
            }

            avg_r = cum_r / ((double) (scale * scale));
            avg_g = cum_g / ((double) (scale * scale));
            avg_b = cum_b / ((double) (scale * scale));

            if (avg_r < min_color) {
                avg_r = min_color;
            }
            if (avg_g < min_color) {
                avg_g = min_color;
            }
            if (avg_b < min_color) {
                avg_b = min_color;
            }
            if (avg_r > max_color) {
                printf("avg too big: %f (max %f)\n", avg_r, max_color);
                exit(1);
            }
            if (avg_g > max_color) {
                printf("avg too big: %f (max %f)\n", avg_g, max_color);
                exit(1);
            }
            if (avg_b > max_color) {
                printf("avg too big: %f (max %f)\n", avg_b, max_color);
                exit(1);
            }

            write_pixel(img, x, y, (u_char) avg_r, (u_char) avg_g,
                        (u_char) avg_b);
        }
    }

#ifdef VERBOSE
    fprintf(stderr, "image_gen_image_scale\n");
#endif
    return (img);
}



/*
 * Draw the bounding box scaled to the current image.
 */
void
image_draw_bbox_scale(RGBImage * img, bbox_t * bbox, int scale,
                      RGBPixel mask, RGBPixel color)
{

    int             x, y;
    int             y_upper, y_lower;
    int             x_right, x_left;

    /*
     * Compute the pixel values, accounting for round off problems
     * if necessary.
     */
    y_upper = bbox->max_y / scale;
    if (y_upper >= img->rows) {
        y_upper = img->rows - 1;
    }
    y_lower = bbox->min_y / scale;
    x_left = bbox->min_x / scale;
    x_right = bbox->max_x / scale;
    if (x_right >= img->columns) {
        x_right = img->columns;
    }

    /* draw the upper and lower edges of the box */
    for (x = x_left; x <= x_right; x++) {
        modify_pixel(img, x, y_upper, mask, color);
        modify_pixel(img, x, y_lower, mask, color);
    }

    /* draw the left and right edges */
    for (y = y_lower; y <= y_upper; y++) {
        modify_pixel(img, x_left, y, mask, color);
        modify_pixel(img, x_right, y, mask, color);
    }
}




/*
 * Draw the bounding box scaled to the current image.
 */
void
image_fill_bbox_scale(RGBImage * img, bbox_t * bbox, int scale,
                      RGBPixel mask, RGBPixel color)
{

    int             x,
                    y;
    int             y_upper,
                    y_lower;
    int             x_right,
                    x_left;

    /*
     * Compute the pixel values, accounting for round off problems
     * if necessary.
     */
    y_upper = bbox->max_y / scale;
    if (y_upper >= img->rows) {
        y_upper = img->rows - 1;
    }
    y_lower = bbox->min_y / scale;
    x_left = bbox->min_x / scale;
    x_right = bbox->max_x / scale;
    if (x_right >= img->columns) {
        x_right = img->columns;
    }

    /*
     * draw the upper and lower edges of the box 
     */
    for (y = y_lower; y <= y_upper; y++) {
        for (x = x_left; x <= x_right; x++) {
            modify_pixel(img, x, y, mask, color);
        }
    }
}


