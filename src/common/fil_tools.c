
#include <stdio.h>
#include <stdint.h>

#include "fil_assert.h"
#include "filter_api.h"
#include "fil_tools.h"

char           *
ft_read_alloc_attr(lf_fhandle_t fhandle, lf_obj_handle_t ohandle,
                   const char *name)
{
    int             err;
    char           *ptr;
    off_t           bsize;

    /*
     * assume this attr > 0 size 
     */

    bsize = 0;
    err = lf_read_attr(fhandle, ohandle, name, &bsize, (char *) NULL);
    if (err != ENOMEM) {
        // fprintf(stderr, "attribute lookup error: %s\n", name);
        return NULL;
    }

    err = lf_alloc_buffer(fhandle, bsize, (char **) &ptr);
    if (err) {
        fprintf(stderr, "alloc error\n");
        return NULL;
    }

    err = lf_read_attr(fhandle, ohandle, name, &bsize, (char *) ptr);
    if (err) {
        fprintf(stderr, "attribute read error: %s\n", name);
        return NULL;
    }

    return ptr;
}

void
ft_free(lf_fhandle_t fhandle, char *ptr)
{
    int             err;
    err = lf_free_buffer(fhandle, ptr);
    // assert(err == 0);
}



u_int32_t
ii_probe(ii_image_t * ii, dim_t x, dim_t y)
{
    u_int32_t       value;

    ASSERTX(value = 0, x >= 0);
    ASSERTX(value = 0, y >= 0);
    ASSERTX(value = 0, x < ii->width);
    ASSERTX(value = 0, y < ii->height);
    value = ii->data[y * ii->width + x];
  done:
    return value;
}




int
write_param(lf_fhandle_t fhandle, lf_obj_handle_t ohandle, char *fmt,
            search_param_t * param, int i)
{
    off_t           bsize;
    char            buf[BUFSIZ];
    int             err;

#ifdef VERBOSE
    lf_log(fhandle, LOGL_TRACE, "FOUND!!! ul=%ld,%ld; scale=%f\n",
           param->bbox.xmin, param->bbox.ymin, param->scale);
#endif
    sprintf(buf, fmt, i);
    bsize = sizeof(search_param_t);
    err = lf_write_attr(fhandle, ohandle, buf, bsize, (char *) param);

    return err;
}

int
read_param(lf_fhandle_t fhandle, lf_obj_handle_t ohandle, char *fmt,
           search_param_t * param, int i)
{
    off_t           bsize;
    char            buf[BUFSIZ];
    int             err;

    sprintf(buf, fmt, i);
    bsize = sizeof(search_param_t);
    err = lf_read_attr(fhandle, ohandle, buf, &bsize, (char *) param);

    return err;
}


/*
 ********************************************************************** */

int
log2_int(int x)
{
    int             logval = 0;

    /*
     * garbage in, garbage out 
     */

    switch (x) {
    case 4:
        logval = 2;
        break;
    case 8:
        logval = 3;
        break;
    case 16:
        logval = 4;
        break;
    case 32:
        logval = 5;
        break;
    default:
        while (x > 1) {
            // assert((x & 1) == 0);
            x >>= 1;
            logval++;
        }
    }

    return logval;
}


/*
 ********************************************************************** */