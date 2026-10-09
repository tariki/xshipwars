// widgets/wfile.cpp
/*
                             Widget: File IO


	Functions:

	image_t *WidgetLoadImageFromTgaFile(char *filename)
	image_t *WidgetLoadImageFromTgaData(u_int8_t *data)

	pixmap_t WidgetLoadPixmapFromTgaFile(char *filename)
        pixmap_t WidgetLoadPixmapFromTgaData(u_int8_t *data)


        image_t *WidgetLoadImageFromXpmFile(char *filename)
	image_t *WidgetLoadImageFromXpmData(char **data)

	pixmap_t WidgetLoadPixmapFromXpmFile(char *filename)
        pixmap_t WidgetLoadPixmapFromXpmData(char **data)

	---


*/


#include "../include/widget.h"
#include "../include/tga.h"     /* For tga loading in wfile.c */

#ifndef MAX
#define MIN(a,b)        (((a) < (b)) ? (a) : (b))
#define MAX(a,b)	(((a) > (b)) ? (a) : (b))
#endif
/* #define MIN(a,b)        (((a) < (b)) ? (a) : (b)) */
/* #define MAX(a,b)        (((a) > (b)) ? (a) : (b)) */


/*
 *      Loads an image from a TGA file.
 *      The image's depth will be that of the GUI's.
 */
image_t *WidgetLoadImageFromTgaFile(char *filename)
{
        tga_data_struct *td;
        image_t *image = NULL;
        off_t img_data_len = 0;
        int status = 0;


	/* Errors. */
	if(!IDC() ||
           (filename == NULL)
	)
	    return(image);


	/* Allocate tga data structure. */
	td = (tga_data_struct *)malloc(sizeof(tga_data_struct));
	if(td == NULL)
	    return(image);

	/* Load tga image from file. */
        status = TgaReadFromFile(filename, td, osw_gui[0].depth);
	if(status != TgaSuccess)
	{
	    fprintf(stderr, "WidgetLoadImageFromTgaFile(): %s: ", filename);
            switch(status)
            {
              case TgaNoBuffers:
                fprintf(stderr, "No buffers.\n");
                break;

              case TgaBadHeader:
                fprintf(stderr, "Bad header.\n");
                break;

              case TgaBadValue:
                fprintf(stderr, "Bad value.\n");
                break;

              case TgaNoFile:
                fprintf(stderr, "No such file.\n");
                break;

              case TgaNoAccess:
                fprintf(stderr, "No access.\n");
                break;

              default:
		fprintf(stderr, "\n");
                break;
	    }

	    TgaDestroyData(td);
	    free(td);

            return(image);
        }


        /*   Copy data in td to GUI image.
	 *
         *   Targa library loads data to memory in ZPixmap format
         *   of the requested depth.
	 */

	switch(osw_gui[0].depth)
	{
	  /* 8 bits. */
	  case 8:
	    if(OSWCreateImage(&image, td->width, td->height))
            {
	        fprintf(
		    stderr,
		    "%s: Error: Cannot allocate GUI image.\n",
		    filename
		);
            }
	    else
            {
	        img_data_len = MIN(
		    (int)(image->width * image->height * BYTES_PER_PIXEL8),
		    (int)(td->width * td->height * BYTES_PER_PIXEL8)
                );

		memcpy(
		    image->data,        /* Target. */
		    td->data,            /* Source. */
		    img_data_len
		);
            }
	    break;

	  /* 15 or 16 bits. */
	  case 15:
	  case 16:
            if(OSWCreateImage(&image, td->width, td->height))
            {
                fprintf(stderr,
                    "%s: Error: Cannot allocate GUI image.\n",
                    filename
                );
            }
            else
            {
                img_data_len = MIN(
                    (int)(image->width * image->height * BYTES_PER_PIXEL16),
                    (int)(td->width * td->height * BYTES_PER_PIXEL16)
                );

                memcpy(
                    image->data,	/* Target. */
                    td->data,		/* Source. */
                    img_data_len
                );
            }
	    break;

          /* 24 or 32 bits. */
	  case 24:
	  case 32:
	    if(OSWCreateImage(&image, td->width, td->height))
	    {
                fprintf(stderr,
                    "%s: Error: Cannot allocate GUI image.\n",
                    filename
                );
            } 
	    else
	    {
                img_data_len = MIN(
                    (int)(image->width * image->height * BYTES_PER_PIXEL32),
		    (int)(td->width * td->height * BYTES_PER_PIXEL32)
		);

                memcpy(
                    image->data,	/* Target. */
                    td->data,		/* Source. */
                    img_data_len
                );
	    }
        }


        /* Destroy tga data. */
        TgaDestroyData(td);
	free(td);


        return(image);
}


/*
 *	Loads an image from TGA data in memory.
 *	The image's depth will be that of the GUI's.
 */
image_t *WidgetLoadImageFromTgaData(u_int8_t *data)
{
        tga_data_struct *td;
        image_t *image = NULL;
        off_t img_data_len = 0;
        int status = 0;


	/* Error checks. */
	if(!IDC() ||
           (data == NULL)
	)
	    return(image);


        /* Allocate tga data structure. */
        td = (tga_data_struct *)malloc(sizeof(tga_data_struct));
        if(td == NULL)
            return(image);

        /* Load tga image from data. */
        status = TgaReadFromData(data, td, osw_gui[0].depth);
        if(status != TgaSuccess)
        {
            fprintf(stderr, "WidgetLoadImageFromTgaData(): %p: ", (void *)data);
            switch(status)
            {
              case TgaNoBuffers:
                fprintf(stderr, "No buffers.\n");
                break;

              case TgaBadHeader:
                fprintf(stderr, "Bad header.\n");
                break;

              case TgaBadValue:
                fprintf(stderr, "Bad value.\n");
                break;

              case TgaNoAccess:
                fprintf(stderr, "No access.\n");
                break;

              default:
                fprintf(stderr, "\n");
                break;
            }

            TgaDestroyData(td);
            free(td);

            return(image);
        }


        /*   Copy data in td to GUI image.
         *
         *   Targa library loads data to memory in ZPixmap format
         *   of the requested depth.
         */

	switch(osw_gui[0].depth)
	{
	  /* 8 bits. */
	  case 8:
            if(OSWCreateImage(&image, td->width, td->height))
            {
                fprintf(stderr,
                    "%p: Error: Cannot allocate GUI image.\n",
                    (void *)data
                );
            }
            else
            {
                img_data_len = MIN(
                    (int)(image->width * image->height * BYTES_PER_PIXEL8),
                    (int)(td->width * td->height * BYTES_PER_PIXEL8)
                );

                memcpy(
                    image->data,	/* Target. */
                    td->data,		/* Source. */
                    img_data_len
                );
            }
	    break;

	  /* 15 or 16 bits. */
	  case 15:
	  case 16:
	    if(OSWCreateImage(&image, td->width, td->height))
            {
                fprintf(stderr,
                    "%p: Error: Cannot allocate GUI image.\n",
                    (void *)data
                );
            }
	    else
            {
	        img_data_len = MIN(
		    (int)(image->width * image->height * BYTES_PER_PIXEL16),
		    (int)(td->width * td->height * BYTES_PER_PIXEL16)
		);

	        memcpy(
		    image->data,	/* Target. */
		    td->data,		/* Source. */
		    img_data_len
		);
            }
	    break;

          /* 24 or 32 bits. */
	  case 24:
	  case 32:
	    if(OSWCreateImage(&image, td->width, td->height))
            {
                fprintf(stderr,
                    "%p: Error: Cannot allocate GUI image.\n",
                    (void *)data
                );
            }
	    else  
            {
	        img_data_len = MIN(
		    (int)(image->width * image->height * BYTES_PER_PIXEL32),
		    (int)(td->width * td->height * BYTES_PER_PIXEL32)
		);

                memcpy(
                    image->data,	/* Target. */
                    td->data,		/* Source. */
                    img_data_len
                );
            }
        }


        /* Destroy tga data. */
        TgaDestroyData(td);
	free(td);


        return(image);
}


/*
 *      Loads a pixmap from a TGA file.
 *      The pixmap's depth will match that of the GUI's.
 */
pixmap_t WidgetLoadPixmapFromTgaFile(char *filename)
{
        image_t *image;
        pixmap_t pixmap = 0;


        image = WidgetLoadImageFromTgaFile(filename);
        if(image == NULL)
            return(pixmap);

        pixmap = OSWCreatePixmapFromImage(image);
        OSWDestroyImage(&image);

        return(pixmap);
}


/*
 *      Loads a pixmap from TGA data in memory.
 *      The pixmap's depth will match that of the GUI's.
 */
pixmap_t WidgetLoadPixmapFromTgaData(u_int8_t *data)
{
	image_t *image;
	pixmap_t pixmap = 0;


	image = WidgetLoadImageFromTgaData(data);
	if(image == NULL)
	    return(pixmap);

	pixmap = OSWCreatePixmapFromImage(image);
	OSWDestroyImage(&image);

	return(pixmap);
}


/*
 *	Loads an image from an XPM file.
 *	The image's depth will match that of the GUI's.
 */
image_t *WidgetLoadImageFromXpmFile(char *filename)
{
	return(OSWLoadImageFromXpmFile(filename));
}

/*
 *      Loads an image from XPM data in memory.
 *      The image's depth will match that of the GUI's.
 */
image_t *WidgetLoadImageFromXpmData(char **data)
{
	return(OSWLoadImageFromXpmData(data));
}

/*
 *      Loads a pixmap from an XPM file.
 *      The pixmap's depth will match that of the GUI's.
 */
pixmap_t WidgetLoadPixmapFromXpmFile(char *filename)
{
	return(OSWLoadPixmapFromXpmFile(filename));
}

/*
 *      Loads a pixmap from XPM data in memory.
 *      The pixmap's depth will match that of the GUI's.
 */
pixmap_t WidgetLoadPixmapFromXpmData(char **data)
{
	return(OSWLoadPixmapFromXpmData(data));
}




