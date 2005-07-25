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


#include <stdio.h>
#include <opencv/cv.h>
#include <limits.h>
#include <string.h>		// for memcmp

#include "rgb.h"		// for image_type_h
#include "fil_image_tools.h"
#include "fil_assert.h"

#include "readtiff.h"

/*
 * Given a ffile_t, examines the first 8 bytes to try to guess
 * whether it is a TIFF, PNM, etc.  Doesn't "read" the file per se.
 */
image_type_t
determine_image_type(const u_char* buf) {
  const u_char pbm_ascii[2]	= "P1";
  const u_char pbm_raw[2]	= "P4";
  const u_char pgm_ascii[2]	= "P2";
  const u_char pgm_raw[2] 	= "P5";
  const u_char ppm_ascii[2]	= "P3";
  const u_char ppm_raw[2]	= "P6";
  const u_char tiff_big_endian[4] = { 0x4d, 0x4d, 0x00, 0x2a };
  const u_char tiff_lit_endian[4] = { 0x49, 0x49, 0x2a, 0x00 };

  image_type_t type = IMAGE_UNKNOWN;
  if	  (0 == memcmp(buf, pbm_ascii, 2))	 { type = IMAGE_PBM; }
  else if (0 == memcmp(buf, pbm_raw, 2))	 { type = IMAGE_PBM; }
  else if (0 == memcmp(buf, pgm_ascii, 2))	 { type = IMAGE_PGM; }
  else if (0 == memcmp(buf, pgm_raw, 2))	 { type = IMAGE_PGM; }
  else if (0 == memcmp(buf, ppm_ascii, 2))	 { type = IMAGE_PPM; }
  else if (0 == memcmp(buf, ppm_raw, 2))	 { type = IMAGE_PPM; }
  else if (0 == memcmp(buf, tiff_big_endian, 4)) { type = IMAGE_TIFF; }
  else if (0 == memcmp(buf, tiff_lit_endian, 4)) { type = IMAGE_TIFF; }

  return type;
}

int
pbm_read_data(off_t dlen, u_char *buf,  RGBImage * img)
{
	int             pixels;
	int		i;

	assert(sizeof(RGBPixel) >= 4);

	pixels =  img->width * img->height;

	/* verify we have enough data */
	assert((pixels / 8) <= dlen);

	for (i=0; i < pixels; i+=8) {
	  int j;
	  for (j=0; j<8; j++) {
	    int bit = 255 * ((*buf >> (7-j)) & 0x1);
	    img->data[i+j].r = bit;
	    img->data[i+j].g = bit;
	    img->data[i+j].b = bit;
	    img->data[i+j].a = 255;
	  }
	  buf++;
	}
	return (0);
}

int
pgm_read_data(off_t dlen, u_char *buf,  RGBImage * img)
{
	int             pixels;
	int		i;

	assert(sizeof(RGBPixel) >= 4);

	pixels =  img->width * img->height;

	/* verify we have enough data */
	assert((pixels) <= dlen);

	for (i=0; i < pixels; i++) {
	    	img->data[i].r = *buf;
	    	img->data[i].g = *buf;
	    	img->data[i].b = *buf++;
	    	img->data[i].a = 255;
	}
	return (0);
}

int
ppm_read_data(off_t dlen, u_char *buf,  RGBImage * img)
{
	int             pixels;
	int		i;

	assert(sizeof(RGBPixel) >= 4);

	pixels =  img->width * img->height;

	/* verify we have enough data */
	assert((pixels * 3) <= dlen);

	for (i=0; i < pixels; i++) {
	    	img->data[i].r = *buf++;
	    	img->data[i].g = *buf++;
	    	img->data[i].b = *buf++;
	    	img->data[i].a = 255;
	}
	return (0);
}

RGBImage*
get_rgb_from_pnm(u_char* buf, off_t size, image_type_t type) {
  assert(buf);
  assert((type==IMAGE_PBM) || (type==IMAGE_PGM) || (type==IMAGE_PPM));

  int err, width, height, headerlen;
  image_type_t magic;
  err = pnm_parse_header(buf, size, &width, &height, &magic, &headerlen);
  if (err) { return NULL; }
  assert(type == magic);	// paranoia :-)

  RGBImage* rgb = rgbimg_blank_image(width, height);
  rgb->type = magic;
  switch (rgb->type) {
    case IMAGE_PBM:
      err = pbm_read_data( (size - headerlen), &buf[headerlen], rgb);
      break;
    case IMAGE_PGM:
      err = pgm_read_data( (size - headerlen), &buf[headerlen], rgb);
      break;
    case IMAGE_PPM:
      err = ppm_read_data( (size - headerlen), &buf[headerlen], rgb);
      break;
    default:
      assert( 0 && "PNM format not PBM/PGM/PPM");
      // TODO: need to exit more cleanly -- file not closed.
      break;
  }
  assert(err==0);

  return rgb;
}

RGBImage*
get_rgb_from_tiff(u_char* buf, off_t size) {
  assert(buf);

  MyTIFF mytiff;
  mytiff.offset	= 0;
  mytiff.buf	= buf;
  mytiff.bytes	= size;

  return convertTIFFtoRGBImage(&mytiff);
}

RGBImage*
get_rgb_img(lf_obj_handle_t ohandle) {
  int		err = 0;
  char *	obj_data;
  off_t		data_len;
  image_type_t	magic;

  RGBImage*	img = NULL;

  err = lf_next_block(ohandle, INT_MAX, &data_len, &obj_data);
  assert(!err);
  magic = determine_image_type(obj_data);

  if ( (magic==IMAGE_PBM) || (magic==IMAGE_PGM) || (magic==IMAGE_PPM)) {
    img = get_rgb_from_pnm(obj_data, data_len, magic);
  } else if (magic == IMAGE_TIFF) {
    img = get_rgb_from_tiff(obj_data, data_len);
  }

  return img;
}



RGBImage       *
old_get_rgb_img(lf_obj_handle_t ohandle)
{
	RGBImage       *img = NULL;
	int             err = 0;
	int             width, height, headerlen;
	off_t           bytes;
	image_type_t    magic;
	char *		obj_data;
	off_t		data_len;

	/*
	 * read the header and figure out the dimensions 
	 */
	err = lf_next_block(ohandle, INT_MAX, &data_len, &obj_data);
	assert(!err);

	err = pnm_parse_header(obj_data, data_len, &width, &height, &magic, 
		&headerlen);
	if (err) {
		return(NULL);
	}
			
	/*
	 * create image to hold the data 
	 */
	bytes = sizeof(RGBImage) + width * height * sizeof(RGBPixel);
	img = (RGBImage *)malloc(bytes);
	assert(img);
	img->nbytes = bytes;
	img->height = height;
	img->width = width;
	img->type = magic;

	/*
	 * read the data into img 
	 */
	/*
	 * this should be elsewhere... 
	 */
	switch (img->type) {
		case IMAGE_PPM:
			err = ppm_read_data((data_len - headerlen), 
				&obj_data[headerlen], img);
			assert(!err);
			break;
		case IMAGE_PGM:
			err = pgm_read_data((data_len - headerlen),
				&obj_data[headerlen], img);
			assert(!err);
			break;
		default:
			assert(0 && "unsupported image format");
			/*
			 * should close file as well XXX 
			 */
	}
	return (img);

}

RGBImage       *
get_attr_rgb_img(lf_obj_handle_t ohandle, char *attr_name)
{
	int             err = 0;
	char           *image_buf;
	off_t           bsize;
	IplImage       *srcimage;
	IplImage       *image;
	RGBImage       *rgb;

	/*
	 * assume this attr > 0 size
	 */

	bsize = 0;
	err = lf_read_attr(ohandle, attr_name, &bsize, (char *) NULL);
	if (err != ENOMEM) {
		return NULL;
	}

	image_buf = (char *)malloc(bsize);
	if (image_buf == NULL) {
		return NULL;
	}

	err = lf_read_attr( ohandle, attr_name, &bsize,
	                   (char *) image_buf);
	if (err) {
		return NULL;
	}


	srcimage = (IplImage *) image_buf;
	image = cvCreateImage(cvSize(srcimage->width, srcimage->height),
	                      srcimage->depth, srcimage->nChannels);

	memcpy(image->imageDataOrigin, image_buf + sizeof(IplImage),
	       image->imageSize);


	rgb = convert_ipl_to_rgb(image);
	cvReleaseImage(&image);
	free(image_buf);

	return (rgb);
}
