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

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <math.h>
#include <sys/time.h>
#include <string.h>
#include <libgen.h>             /* dirname */
#include <assert.h>
#include <stdint.h>


#include <sys/queue.h>
#include "snapfind_consts.h"
#include "lib_results.h"
#include "lib_sfimage.h"
#include "rgb.h"
#include "gui_thread.h"
#include "sfind_tools.h"

/*
 ********************************************************************** */

static pthread_mutex_t ih_mutex = PTHREAD_MUTEX_INITIALIZER;


image_hooks_t  *
ih_new_ref(RGBImage * img, ls_obj_handle_t ohandle)
{
	image_hooks_t  *ptr = (image_hooks_t *) calloc(1, sizeof(image_hooks_t));
	assert(ptr);
	ptr->refcount = 1;

	ptr->img = img;
	ptr->ohandle = ohandle;

	return ptr;
}

void
ih_get_ref(image_hooks_t * ptr)
{
	pthread_mutex_lock(&ih_mutex);
	ptr->refcount++;
	pthread_mutex_unlock(&ih_mutex);
}

void
ih_drop_ref(image_hooks_t * ptr)
{

	pthread_mutex_lock(&ih_mutex);
	assert(ptr->refcount > 0);
	ptr->refcount--;
	if (ptr->refcount == 0) {
		free(ptr->img);
		ls_release_object(NULL, ptr->ohandle);
		free(ptr);
	}
	pthread_mutex_unlock(&ih_mutex);
}
