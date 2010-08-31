/*
 *  SnapFind
 *  An interactive image search application
 *  Version 1
 *
 *  Copyright (c) 2002-2005 Intel Corporation
 *  All Rights Reserved.
 *
 *  This software is distributed under the terms of the Eclipse Public
 *  License, Version 1.0 which can be found in the file named LICENSE.
 *  ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS SOFTWARE CONSTITUTES
 *  RECIPIENT'S ACCEPTANCE OF THIS AGREEMENT
 */

#ifndef	_SFIND_TOOLS_H_
#define	_SFIND_TOOLS_H_

#include "rgb.h"

#include "searchlet_api.h"

#ifdef __cplusplus
extern "C"
{
#endif



/* reference counted bunch of handles */
typedef struct image_hooks_t {
	int		refcount;
	RGBImage        *img;
	ls_obj_handle_t ohandle;
}
image_hooks_t;

image_hooks_t *ih_new_ref(RGBImage *img, ls_obj_handle_t ohandle);
void ih_get_ref(image_hooks_t *ptr);
void ih_drop_ref(image_hooks_t *ptr);


/* ********************************************************************** */

void img_constrain_bbox(bbox_t *bbox, RGBImage *img);


#ifdef __cplusplus
}
#endif
#endif	/* _SFIND_TOOLS_H_ */
