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


#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <gtk/gtk.h>
#include <opencv/cv.h>
#include <opencv/cvaux.h>
#include "queue.h"
#include "rgb.h"
#include "common_consts.h"
//#include "histo.h"
#include "image_tools.h"
//#include "texture_tools.h"
#include "img_search.h"
//#include "gui_thread.h"
//#include "snapfind.h"
//#include "fil_tools.h"
#include "facedet.h"
#include "face_tools.h"
#include "fil_data2ii.h"
#include "search_set.h"
#include "read_config.h"

#define	MAX_DISPLAY_NAME	64

void
vj_face_init()
{
        vj_face_factory *fac;
        printf("vj_face init \n");

        fac = new vj_face_factory;

        read_config_register("vj_face_search", fac);
}


vj_face_search::vj_face_search(const char *name, char *descr)
		: window_search(name, descr)
{
	face_count = 1;
	start_stage = 0;
	end_stage = 37;
	do_merge = 1;
	overlap_val = 0.75;

	edit_window = NULL;
	count_widget = NULL;
	start_widget = NULL;
	end_widget = NULL;
	face_merge = NULL;
	merge_overlap = NULL;

	set_scale(1.20);
	set_stride(1);
	set_testx(24);
	set_testy(24);
}

vj_face_search::~vj_face_search()
{
	return;
}


void
vj_face_search::set_face_count(char *data)
{
	int		new_count = atoi(data);

	set_face_count(new_count);
	return;
}

void
vj_face_search::set_face_count(int new_count)
{
	if (new_count < 0) {
		new_count = 0;
	}

	face_count = new_count;
	return;
}

#define	VJ_FACE_MIN_STAGE	0
#define	VJ_FACE_MAX_STAGE	37

void
vj_face_search::set_start_level(char *data)
{
	int		new_level = atoi(data);

	set_start_level(new_level);
	return;
}

void
vj_face_search::set_start_level(int new_level)
{
	if (new_level < VJ_FACE_MIN_STAGE) {
		new_level = VJ_FACE_MIN_STAGE;
	}

	if (new_level > VJ_FACE_MAX_STAGE) {
		new_level = VJ_FACE_MAX_STAGE;
	}

	start_stage = new_level;
	return;
}



void
vj_face_search::set_end_level(char *data)
{
	int		new_level = atoi(data);

	set_end_level(new_level);
	return;
}

void
vj_face_search::set_end_level(int new_level)
{
	if (new_level < VJ_FACE_MIN_STAGE) {
		new_level = VJ_FACE_MIN_STAGE;
	}

	if (new_level > VJ_FACE_MAX_STAGE) {
		new_level = VJ_FACE_MAX_STAGE;
	}

	/* XXX deal with case where end start overlap ??? */
	end_stage = new_level;
	return;
}


int
vj_face_search::handle_config(config_types_t conf_type, char *data)
{
	int	err;

	switch (conf_type) {
		case NUMF_TOK:
			set_face_count(data);
			err = 0;
			break;

		case START_TOK:
			set_start_level(data);
			err = 0;
			break;

		case END_TOK:
			set_end_level(data);
			err = 0;
			break;

		case MERGE_TOK:
			do_merge = atoi(data);
			err = 0;
			break;

		case OVERLAP_TOK:
			overlap_val = atof(data);
			err = 0;
			break;

		default:
			err = window_search::handle_config(conf_type, data);
			break;
	}
	return(err);
}




static GtkWidget *
create_slider_entry(char *name, float min, float max, int dec, float initial,
                    float step, GtkObject **adjp)
{
	GtkWidget *container;
	GtkWidget *scale;
	GtkWidget *button;
	GtkWidget *label;


	container = gtk_hbox_new(FALSE, 10);

	label = gtk_label_new(name);
	gtk_box_pack_start(GTK_BOX(container), label, FALSE, FALSE, 0);

	if (max <= 1.0) {
		max += 0.1;
		*adjp = gtk_adjustment_new(min, min, max, step, 0.1, 0.1);
	} else if (max < 50) {
		max++;
		*adjp = gtk_adjustment_new(min, min, max, step, 1.0, 1.0);
	} else {
		max+= 10;
		*adjp = gtk_adjustment_new(min, min, max, step, 10.0, 10.0);
	}
	gtk_adjustment_set_value(GTK_ADJUSTMENT(*adjp), initial);

	scale = gtk_hscale_new(GTK_ADJUSTMENT(*adjp));
	gtk_widget_set_size_request (GTK_WIDGET(scale), 200, -1);
	gtk_range_set_update_policy (GTK_RANGE(scale), GTK_UPDATE_CONTINUOUS);
	gtk_scale_set_draw_value (GTK_SCALE(scale), FALSE);
	gtk_box_pack_start (GTK_BOX(container), scale, TRUE, TRUE, 0);
	gtk_widget_set_size_request(scale, 120, 0);

	button = gtk_spin_button_new(GTK_ADJUSTMENT(*adjp), step, dec);
	gtk_box_pack_start(GTK_BOX(container), button, FALSE, FALSE, 0);

	gtk_widget_show(container);
	gtk_widget_show(label);
	gtk_widget_show(scale);
	gtk_widget_show(button);

	return(container);
}

static void
cb_close_edit_window(GtkWidget* item, gpointer data)
{
	texture_search *	search;

	search = (texture_search *)data;
	search->close_edit_win();
}


void
vj_face_search::close_edit_win()
{

	/* save any changes from the edit windows */
	save_edits();

	/* call the parent class to give them change to cleanup */
	window_search::close_edit_win();

	edit_window = NULL;

}

static void
edit_search_done_cb(GtkButton *item, gpointer data)
{
	GtkWidget * widget = (GtkWidget *)data;
	gtk_widget_destroy(widget);
}

static void
cb_merge_face(GtkWidget *widget, gpointer ptr)
{
	vj_face_search *face;

	face = (vj_face_search *)ptr;

	face->update_toggle();
}



void
vj_face_search::update_toggle()
{
	do_merge = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(face_merge));
	gtk_widget_set_sensitive(overlap_widget, do_merge);
}

void
vj_face_search::edit_search()
{
	GtkWidget * 	widget;
	GtkWidget * 	box;
	GtkWidget * 	frame;
	GtkWidget * 	hbox;
	GtkWidget * 	container;
	char		name[MAX_DISPLAY_NAME];

	/* see if it already exists */
	if (edit_window != NULL) {
		/* raise to top ??? */
		gdk_window_raise(GTK_WIDGET(edit_window)->window);
		return;
	}

	edit_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	snprintf(name, MAX_DISPLAY_NAME - 1, "Edit %s", get_name());
	name[MAX_DISPLAY_NAME -1] = '\0';
	gtk_window_set_title(GTK_WINDOW(edit_window), name);
	//gtk_window_set_default_size(GTK_WINDOW(edit_window), 750, 350);
	g_signal_connect(G_OBJECT(edit_window), "destroy",
	                 G_CALLBACK(cb_close_edit_window), this);
	box = gtk_vbox_new(FALSE, 10);


	hbox = gtk_hbox_new(FALSE, 10);
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, TRUE, 0);

	widget = gtk_button_new_with_label("Close");
	g_signal_connect(G_OBJECT(widget), "clicked",
	                 G_CALLBACK(edit_search_done_cb), edit_window);
	GTK_WIDGET_SET_FLAGS(widget, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);


	/*
		 * Get the controls from the img_search.
	 */
	widget = img_search_display();
	gtk_box_pack_start(GTK_BOX(box), widget, FALSE, TRUE, 0);

	/*
	 	 * Create the texture parameters.
	 */

	frame = gtk_frame_new("VJ Face Search");
	container = gtk_vbox_new(FALSE, 10);
	gtk_container_add(GTK_CONTAINER(frame), container);

	widget = create_slider_entry("Number of Faces", 0.0, 20.0, 0,
	                             face_count, 1.0, &count_widget);
	gtk_box_pack_start(GTK_BOX(container), widget, FALSE, TRUE, 0);

	widget = create_slider_entry("Start Stage", 0.0, 38.0, 0,
	                             start_stage, 1.0, &start_widget);
	gtk_box_pack_start(GTK_BOX(container), widget, FALSE, TRUE, 0);

	widget = create_slider_entry("End Stage", 0.0, 38.0, 0,
	                             end_stage, 1.0, &end_widget);
	gtk_box_pack_start(GTK_BOX(container), widget, FALSE, TRUE, 0);

	face_merge = gtk_check_button_new_with_label("Merge Faces");
	gtk_box_pack_start(GTK_BOX(container), face_merge, FALSE, TRUE, 0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(face_merge), do_merge);

	overlap_widget = create_slider_entry("Overlap Value", 0.0, 1.0, 2,
	                                     overlap_val, 0.1, &merge_overlap);
	gtk_box_pack_start(GTK_BOX(container), overlap_widget, FALSE, FALSE, 0);
	update_toggle();
	g_signal_connect(G_OBJECT(face_merge), "toggled", G_CALLBACK(cb_merge_face),
	                 (void *)this);


	gtk_box_pack_start(GTK_BOX(box), frame, FALSE, TRUE, 0);

	gtk_widget_show(container);

	/*
	 * Get the controls from the window search class.
		 */
	widget = get_window_cntrl();
	gtk_box_pack_start(GTK_BOX(box), widget, FALSE, TRUE, 0);


	gtk_container_add(GTK_CONTAINER(edit_window), box);

	//gtk_window_set_default_size(GTK_WINDOW(edit_window), 400, 500);
	gtk_widget_show_all(edit_window);

}

/*
 * This method reads the values from the current edit
 * window if there is an active one.
 */

void
vj_face_search::save_edits()
{
	int		val;

	/* no active edit window, so return */
	if (edit_window == NULL) {
		return;
	}

	val = (int)gtk_adjustment_get_value(GTK_ADJUSTMENT(count_widget));
	set_face_count(val);

	val = (int)gtk_adjustment_get_value(GTK_ADJUSTMENT(start_widget));
	set_start_level(val);

	val = (int)gtk_adjustment_get_value(GTK_ADJUSTMENT(end_widget));
	set_end_level(val);

	do_merge = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(face_merge));
	overlap_val = (float)gtk_adjustment_get_value(GTK_ADJUSTMENT(merge_overlap));



	/* call the parent class */
	window_search::save_edits();
}

/*
 * This write the relevant section of the filter specification file
 * for this search.
 */

void
vj_face_search::write_fspec(FILE *ostream)
{
	img_search *	ss;
	save_edits();
	/*
		 * First we write the header section that corrspons
		 * to the filter, the filter name, the assocaited functions.
		 */

	fprintf(ostream, "\n");
	fprintf(ostream, "FILTER %s \n", get_name());
	fprintf(ostream, "THRESHOLD %d \n", 1);	/* XXX should we change */

	fprintf(ostream, "EVAL_FUNCTION  f_eval_vj_detect \n");
	fprintf(ostream, "INIT_FUNCTION  f_init_vj_detect \n");
	fprintf(ostream, "FINI_FUNCTION  f_fini_vj_detect \n");
	fprintf(ostream, "ARG  %s  # name \n", get_name());

	/*
	 * Next we write call the parent to write out the releated args,
	 * not that since the args are passed as a vector of strings
	 * we need keep the order the args are written constant or silly
	 * things will happen.
	 */
	window_search::write_fspec(ostream);

	/*
	 * Now write the state needed that is just dependant on the histogram
	 * search.  This will have the histo releated parameters
	 * as well as the linearized histograms.
	 */

	fprintf(ostream, "ARG  %d  # num faces \n", face_count);
	fprintf(ostream, "ARG  %d  # start stage \n", start_stage);
	fprintf(ostream, "ARG  %d  # end_stage \n", end_stage);

	/* XXX fix RGB */
	fprintf(ostream, "REQUIRES  INTEGRATE  # dependancies \n");
	fprintf(ostream, "MERIT  10  # some relative cost \n");
	fprintf(ostream, "\n");

	ss = new ii_img("II image", "II image");
	(this->get_parent())->add_dep(ss);

	if (do_merge) {
		fprintf(ostream, "\n");
		fprintf(ostream, "FILTER  MERGE  # name \n");
		fprintf(ostream, "THRESHOLD  1  # threshold \n");
		fprintf(ostream, "EVAL_FUNCTION f_eval_bbox_merge # eval fn\n");
		fprintf(ostream, "INIT_FUNCTION f_init_bbox_merge # init fn\n");
		fprintf(ostream, "FINI_FUNCTION f_fini_bbox_merge # fini fn\n");
		fprintf(ostream, "REQUIRES  %s  # dependencies \n",get_name() );
		fprintf(ostream, "MERIT  8  # merit value \n");
		fprintf(ostream, "ARG  %f  # overlap val   \n", overlap_val);
	}

	ss = new rgb_img("RGB image", "RGB image");
	(this->get_parent())->add_dep(ss);
}

void
vj_face_search::write_config(FILE *ostream, const char *dirname)
{

	save_edits();

	/* create the search configuration */
	fprintf(ostream, "\n\n");
	fprintf(ostream, "SEARCH vj_face_search %s\n", get_name());

	fprintf(ostream, "NUMFACE %d \n", face_count);
	fprintf(ostream, "START %d \n", start_stage);
	fprintf(ostream, "END %d \n", end_stage);
	fprintf(ostream, "MERGE %d \n", do_merge);
	fprintf(ostream, "OVERLAP %f \n", overlap_val);

	window_search::write_config(ostream, dirname);
	return;
}



void
vj_face_search::region_match(RGBImage *img, bbox_list_t *blist)
{

	fconfig_fdetect_t	fconfig;
	ii_image_t	 *	ii;
	ii2_image_t	 *	ii2;
	int				pass;
	int				size;
	int				width, height;

	save_edits();

	/* XXX */
	init_classifier();

	fconfig.name = strdup(get_name());
	assert(fconfig.name != NULL);

	fconfig.scale_mult = get_scale();
	fconfig.xsize = get_testx();
	fconfig.ysize = get_testy();
	fconfig.stride = get_stride();
	fconfig.lev1 = start_stage;
	fconfig.lev2 = end_stage;

	width = img->width;
	height = img->height;

	size = sizeof(ii_image_t) + sizeof(uint32_t) * (width + 1) *
	       (height + 1);
	ii = (ii_image_t *) malloc(size);
	assert(ii != NULL);
	ii->nbytes = size;
	ii->width = width + 1;
	ii->height = height + 1;

	size = sizeof(ii2_image_t) + sizeof(float) * (width + 1) *
	       (height + 1);
	ii2 = (ii2_image_t *) malloc(size);
	assert(ii2 != NULL);
	ii2->nbytes = size;
	ii2->width = width + 1;
	ii2->height = height + 1;

	/* build the integral image */
	rgb_integrate(img, ii->data, ii2->data, width + 1, height + 1);

	/* scan the image using the set parameters */
	pass = face_scan_image(ii, ii2, &fconfig, blist, height, width);

	/* free any allocated state */
	free(ii);
	free(ii2);
	return;
}


