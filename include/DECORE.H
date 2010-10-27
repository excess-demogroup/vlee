/**************************************************************************
 *                                                                        *
 * This code has been developed by Andrea Graziani. Those intending to    *
 * use this software module in hardware or software products are advised  *
 * that its use may infringe existing patents or copyrights, and any such * 
 * use would be at such party's own risk. The original developer of this  *
 * software module and his/her company, and subsequent editors and their  *
 * companies (including Project Mayo), will have no liability for use of  *
 * this software or modifications or derivatives thereof.                 *
 *                                                                        *
 * Project Mayo gives users of the Codec and Filter a license to this     *
 * software  module or modifications thereof for use in hardware or       *
 * software products claiming conformance to the MPEG-4 Video Standard    *
 * as described in the Open DivX license.                                 *
 *                                                                        *
 * The complete Open DivX license can be found at                         *
 * http://www.projectmayo.com/opendivx/license.php                        *
 *                                                                        *
 **************************************************************************/
/**
*  Copyright (C) 2001 - Project Mayo
 *
 * Adam Li
 * Andrea Graziani
 * Jonathan White
 *
 * DivX Advanced Research Center <darc@projectmayo.com>
*
**/
// decore.h //

// This is the header file describing 
// the entrance function of the encoder core
// or the encore ...

#ifdef __cplusplus
extern "C" {
#endif 

#ifndef _DECORE_H_
#define _DECORE_H_

#ifdef WIN32
#define STDCALL _stdcall
#else
#define STDCALL
#endif

#if ( (! defined (WIN32)) && (! defined (LINUX)) )
#define BIG_ENDIAN
#endif

/**
 *
**/

// decore options
#define DEC_OPT_MEMORY_REQS	0x00004000
#define DEC_OPT_INIT				0x00008000
#define DEC_OPT_RELEASE			0x00010000
#define DEC_OPT_SETPP				0x00020000 // set postprocessing mode
#define DEC_OPT_SETOUT			0x00040000 // set output mode

// decore return values
#define DEC_OK					0
#define DEC_MEMORY			1
#define DEC_BAD_FORMAT	2
#define DEC_EXIT				3

// decore YUV color format
#define DEC_YUV2			1
#define DEC_UYVY			2
#define DEC_420				3

// decore RGB color format
#define DEC_RGB32			4 
#define DEC_RGB24			5 
#define DEC_RGB555		6 
#define DEC_RGB565		7	

#define DEC_RGB32_INV		8
#define DEC_RGB24_INV		9
#define DEC_RGB555_INV 10
#define DEC_RGB565_INV 11

/* this is necessary for the max resolution (juice resolution) */
#define DEC_MBC         45
#define DEC_MBR         36

/**
 *
**/

typedef struct _DEC_MEM_REQS_
{
	unsigned long mp4_edged_ref_buffers_size;
	unsigned long mp4_edged_for_buffers_size;
	unsigned long mp4_display_buffers_size;
	unsigned long mp4_state_size;
	unsigned long mp4_tables_size;
	unsigned long mp4_stream_size;
} DEC_MEM_REQS;

typedef struct _DEC_BUFFERS_ 
{
	void * mp4_edged_ref_buffers;  
	void * mp4_edged_for_buffers;  
	void * mp4_display_buffers;
	void * mp4_state;
	void * mp4_tables;
	void * mp4_stream;
} DEC_BUFFERS;

typedef struct _DEC_PARAM_ 
{
	int x_dim; // x dimension of the frames to be decoded
	int y_dim; // y dimension of the frames to be decoded
	int output_format;	// output color format
	int time_incr;
	DEC_BUFFERS buffers;
} DEC_PARAM;

typedef struct _DEC_FRAME_
{
	void *bmp; // the decoded bitmap 
	void *bitstream; // the decoder buffer
	long length; // the lenght of the decoder stream
	int render_flag;	// 1: the frame is going to be rendered
	unsigned int stride; // decoded bitmap stride
} DEC_FRAME;

typedef struct _DEC_SET_
{
	int postproc_level; // valid interval are [0..100]
} DEC_SET;

/**
 *
**/

// the prototype of the decore() - main decore engine entrance
//
int STDCALL decore(
			unsigned long handle,	// handle	- the handle of the calling entity, must be unique
			unsigned long dec_opt, // dec_opt - the option for docoding, see below
			void *param1,	// param1	- the parameter 1 (it's actually meaning depends on dec_opt
			void *param2);	// param2	- the parameter 2 (it's actually meaning depends on dec_opt

#endif // _DECORE_H_
#ifdef __cplusplus
}
#endif 
