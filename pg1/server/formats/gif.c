/* $Id$
 *
 * gif.c - Read only GIF loader based on libungif
 *
 * Yeah, we should be using libungif. But, libungif doesn't support
 * a custom read function. So we'd either have to use fopencookie(),
 * which only works in glibc, or pipes, which only work in unix.
 * Besides, this way we can ditch the code for writing GIF files and in
 * general cut out pieces picogui doesn't need.
 *
 * Most of this is a hacked up version of dgif_lib.c
 * The only functions used by picogui, gif_detect() and gif_load(),
 * are at the bottom.
 *
 * libungif came with the following COPYING notice:
 *
 *    The GIFLIB distribution is Copyright (c) 1997  Eric S. Raymond
 *
 *    Permission is hereby granted, free of charge, to any person obtaining a copy
 *    of this software and associated documentation files (the "Software"), to deal
 *    in the Software without restriction, including without limitation the rights
 *    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *    copies of the Software, and to permit persons to whom the Software is
 *    furnished to do so, subject to the following conditions:
 *
 *    The above copyright notice and this permission notice shall be included in
 *    all copies or substantial portions of the Software.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 *    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *    THE SOFTWARE.
 *
 * Additionally, this is the original comment block from dgif_lib.c:
 * 
 *******************************************************************************
 *   "Gif-Lib" - Yet another gif library.				      *
 *									      *
 * Written by:  Gershon Elber			IBM PC Ver 1.1,	Aug. 1990     *
 *******************************************************************************
 * The kernel of the GIF Decoding process can be found here.		      *
 *******************************************************************************
 * History:								      *
 * 16 Jun 89 - Version 1.0 by Gershon Elber.				      *
 *  3 Sep 90 - Version 1.1 by Gershon Elber (Support for Gif89, Unique names). *
 ******************************************************************************/

#include <pgserver/common.h>
#include <pgserver/video.h>

#include <stdio.h>
#include <string.h>

#define HT_SIZE			8192	   /* 12bits = 4096 or twice as big! */
#define HT_KEY_MASK		0x1FFF			      /* 13bits keys */
#define HT_KEY_NUM_BITS		13			      /* 13bits keys */
#define HT_MAX_KEY		8191	/* 13bits - 1, maximal code possible */
#define HT_MAX_CODE		4095	/* Biggest code possible in 12 bits. */

#define	GIF_ERROR	0
#define GIF_OK		1

#define MAX(x, y)	(((x) > (y)) ? (x) : (y))

#ifndef TRUE
#define TRUE		1
#define FALSE		0
#endif

#ifdef SYSV
#define VoidPtr char *
#else
#define VoidPtr void *
#endif /* SYSV */

typedef	int		GifBooleanType;
typedef	unsigned char	GifPixelType;
typedef unsigned char *	GifRowType;
typedef unsigned char	GifByteType;

typedef struct GifColorType {
    GifByteType Red, Green, Blue;
} GifColorType;

typedef struct ColorMapObject
{
    int	ColorCount;
    int BitsPerPixel;
    GifColorType *Colors;		/* on malloc(3) heap */
}
ColorMapObject;

typedef struct GifImageDesc {
    int Left, Top, Width, Height,	/* Current image dimensions. */
	Interlace;			/* Sequential/Interlaced lines. */
    ColorMapObject *ColorMap;		/* The local color map */
} GifImageDesc;

typedef struct GifFileType {
    int SWidth, SHeight,		/* Screen dimensions. */
	SColorResolution, 		/* How many colors can we generate? */
	SBackGroundColor;		/* I hope you understand this one... */
    ColorMapObject *SColorMap;		/* NULL if not exists. */
    int ImageCount;			/* Number of current image */
    GifImageDesc Image;			/* Block describing current image */
    VoidPtr Private;	  		/* Don't mess with this! */
} GifFileType;

typedef enum {
    UNDEFINED_RECORD_TYPE,
    IMAGE_DESC_RECORD_TYPE,		/* Begin with ',' */
    EXTENSION_RECORD_TYPE,		/* Begin with '!' */
    TERMINATE_RECORD_TYPE		/* Begin with ';' */
} GifRecordType;

/******************************************************************************
*  GIF89 extension function codes                                             *
******************************************************************************/

#define COMMENT_EXT_FUNC_CODE		0xfe	/* comment */
#define GRAPHICS_EXT_FUNC_CODE		0xf9	/* graphics control */
#define PLAINTEXT_EXT_FUNC_CODE		0x01	/* plaintext */
#define APPLICATION_EXT_FUNC_CODE	0xff	/* application block */

#define	D_GIF_ERR_OPEN_FAILED	101		/* And DGif possible errors. */
#define	D_GIF_ERR_READ_FAILED	102
#define	D_GIF_ERR_NOT_GIF_FILE	103
#define D_GIF_ERR_NO_SCRN_DSCR	104
#define D_GIF_ERR_NO_IMAG_DSCR	105
#define D_GIF_ERR_NO_COLOR_MAP	106
#define D_GIF_ERR_WRONG_RECORD	107
#define D_GIF_ERR_DATA_TOO_BIG	108
#define D_GIF_ERR_NOT_ENOUGH_MEM 109
#define D_GIF_ERR_CLOSE_FAILED	110
#define D_GIF_ERR_NOT_READABLE	111
#define D_GIF_ERR_IMAGE_DEFECT	112
#define D_GIF_ERR_EOF_TOO_SOON	113

ColorMapObject *MakeMapObject(int ColorCount, GifColorType *ColorMap);
void FreeMapObject(ColorMapObject *Object);
int BitSize(int n);

/* This is the in-core version of an extension record */
typedef struct {
    int		ByteCount;
    char	*Bytes;		/* on malloc(3) heap */
} ExtensionBlock;

#define COMMENT_EXT_FUNC_CODE	0xfe /* Extension function code for comment. */
#define GIF_STAMP	"GIFVER"	 /* First chars in file - GIF stamp. */
#define GIF_STAMP_LEN	sizeof(GIF_STAMP) - 1
#define GIF_VERSION_POS	3		/* Version first character in stamp. */

#define LZ_MAX_CODE	4095		/* Biggest code possible in 12 bits. */
#define LZ_BITS		12

#define FILE_STATE_READ		0x01/* 1 write, 0 read - EGIF_LIB compatible.*/

#define FLUSH_OUTPUT		4096    /* Impossible code, to signal flush. */
#define FIRST_CODE		4097    /* Impossible code, to signal first. */
#define NO_SUCH_CODE		4098    /* Impossible code, to signal empty. */

#define IS_READABLE(Private)	(!(Private->FileState & FILE_STATE_READ))

typedef struct GifFilePrivateType {
    int FileState,
	FileHandle,			     /* Where all this data goes to! */
	BitsPerPixel,	    /* Bits per pixel (Codes uses at list this + 1). */
	ClearCode,				       /* The CLEAR LZ code. */
	EOFCode,				         /* The EOF LZ code. */
	RunningCode,		    /* The next code algorithm can generate. */
	RunningBits,/* The number of bits required to represent RunningCode. */
	MaxCode1,  /* 1 bigger than max. possible code, in RunningBits bits. */
	LastCode,		        /* The code before the current code. */
	CrntCode,				  /* Current algorithm code. */
	StackPtr,		         /* For character stack (see below). */
	CrntShiftState;		        /* Number of bits in CrntShiftDWord. */
    u32 CrntShiftDWord; 	      /* For bytes decomposition into codes. */
    u32 PixelCount;		               /* Number of pixels in image. */
    GifByteType Buf[256];	       /* Compressed input is buffered here. */
    GifByteType Stack[LZ_MAX_CODE];	 /* Decoded pixels are stacked here. */
    GifByteType Suffix[LZ_MAX_CODE+1];	       /* So we can trace the codes. */
    unsigned int Prefix[LZ_MAX_CODE+1];

    /* Original image in memory here */
    const u8 *input;
    u32 input_len;
} GifFilePrivateType;

int _GifError;

int DGifGetScreenDesc(GifFileType *GifFile);
int DGifGetCodeNext(GifFileType *GifFile, GifByteType **CodeBlock);
int DGifGetExtensionNext(GifFileType *GifFile, GifByteType **Extension);
static int DGifGetWord(GifFilePrivateType *p, int *Word);
static int DGifSetupDecompress(GifFileType *GifFile);
static int DGifDecompressLine(GifFileType *GifFile, GifPixelType *Line,
								int LineLen);
static int DGifGetPrefixChar(unsigned int *Prefix, int Code, int ClearCode);
static int DGifDecompressInput(GifFilePrivateType *Private, int *Code);
static int DGifBufferedInput(GifFilePrivateType *File, GifByteType *Buf,
						     GifByteType *NextByte);

size_t my_fread(void *ptr, size_t size, size_t nmemb, GifFilePrivateType *p) {
  if (nmemb > p->input_len)
    nmemb = p->input_len;
  memcpy(ptr,p->input,nmemb);
  p->input_len -= nmemb;
  p->input += nmemb;
  return nmemb;
}

/******************************************************************************
*   Update a new gif file, given its file handle.			      *
*   Returns GifFileType pointer dynamically allocated which serves as the gif *
* info record. _GifError is cleared if succesfull.			      *
******************************************************************************/
GifFileType *DGifOpenFileMemory(const u8* input, u32 input_len)
{
    GifFileType *GifFile;
    GifFilePrivateType *Private;

    if ((GifFile = (GifFileType *) malloc(sizeof(GifFileType))) == NULL) {
	_GifError = D_GIF_ERR_NOT_ENOUGH_MEM;
	return NULL;
    }

    memset(GifFile, '\0', sizeof(GifFileType));

    if ((Private = (GifFilePrivateType *) malloc(sizeof(GifFilePrivateType)))
	== NULL) {
	_GifError = D_GIF_ERR_NOT_ENOUGH_MEM;
	free((char *) GifFile);
	return NULL;
    }
    GifFile->Private = (VoidPtr) Private;
    Private->input = input + GIF_STAMP_LEN;
    Private->input_len = input_len - GIF_STAMP_LEN;
    Private->FileState = 0;   /* Make sure bit 0 = 0 (File open for read). */

    if (DGifGetScreenDesc(GifFile) == GIF_ERROR) {
	free((char *) Private);
	free((char *) GifFile);
	return NULL;
    }

    _GifError = 0;

    return GifFile;
}

/******************************************************************************
*   This routine should be called before any other DGif calls. Note that      *
* this routine is called automatically from DGif file open routines.	      *
******************************************************************************/
int DGifGetScreenDesc(GifFileType *GifFile)
{
    int i, BitsPerPixel;
    GifByteType Buf[3];
    GifFilePrivateType *Private = (GifFilePrivateType *) GifFile->Private;

    if (!IS_READABLE(Private)) {
	/* This file was NOT open for reading: */
	_GifError = D_GIF_ERR_NOT_READABLE;
	return GIF_ERROR;
    }

    /* Put the screen descriptor into the file: */
    if (DGifGetWord(Private, &GifFile->SWidth) == GIF_ERROR ||
	DGifGetWord(Private, &GifFile->SHeight) == GIF_ERROR)
	return GIF_ERROR;

    if (my_fread(Buf, 1, 3, Private) != 3) {
	_GifError = D_GIF_ERR_READ_FAILED;
	return GIF_ERROR;
    }
    GifFile->SColorResolution = (((Buf[0] & 0x70) + 1) >> 4) + 1;
    BitsPerPixel = (Buf[0] & 0x07) + 1;
    GifFile->SBackGroundColor = Buf[1];
    if (Buf[0] & 0x80) {		     /* Do we have global color map? */

	GifFile->SColorMap = MakeMapObject(1 << BitsPerPixel, NULL);

	/* Get the global color map: */
	for (i = 0; i < GifFile->SColorMap->ColorCount; i++) {
	    if (my_fread(Buf, 1, 3, Private) != 3) {
		_GifError = D_GIF_ERR_READ_FAILED;
		return GIF_ERROR;
	    }
	    GifFile->SColorMap->Colors[i].Red = Buf[0];
	    GifFile->SColorMap->Colors[i].Green = Buf[1];
	    GifFile->SColorMap->Colors[i].Blue = Buf[2];
	}
    }

    return GIF_OK;
}

/******************************************************************************
*   This routine should be called before any attemp to read an image.         *
******************************************************************************/
int DGifGetRecordType(GifFileType *GifFile, GifRecordType *Type)
{
    GifByteType Buf;
    GifFilePrivateType *Private = (GifFilePrivateType *) GifFile->Private;

    if (!IS_READABLE(Private)) {
	/* This file was NOT open for reading: */
	_GifError = D_GIF_ERR_NOT_READABLE;
	return GIF_ERROR;
    }

    if (my_fread(&Buf, 1, 1, Private) != 1) {
	_GifError = D_GIF_ERR_READ_FAILED;
	return GIF_ERROR;
    }

    switch (Buf) {
	case ',':
	    *Type = IMAGE_DESC_RECORD_TYPE;
	    break;
	case '!':
	    *Type = EXTENSION_RECORD_TYPE;
	    break;
	case ';':
	    *Type = TERMINATE_RECORD_TYPE;
	    break;
	default:
	    *Type = UNDEFINED_RECORD_TYPE;
	    _GifError = D_GIF_ERR_WRONG_RECORD;
	    return GIF_ERROR;
    }

    return GIF_OK;
}

/******************************************************************************
*   This routine should be called before any attemp to read an image.         *
*   Note it is assumed the Image desc. header (',') has been read.	      *
******************************************************************************/
int DGifGetImageDesc(GifFileType *GifFile)
{
    int i, BitsPerPixel;
    GifByteType Buf[3];
    GifFilePrivateType *Private = (GifFilePrivateType *) GifFile->Private;

    if (!IS_READABLE(Private)) {
	/* This file was NOT open for reading: */
	_GifError = D_GIF_ERR_NOT_READABLE;
	return GIF_ERROR;
    }

    if (DGifGetWord(Private, &GifFile->Image.Left) == GIF_ERROR ||
	DGifGetWord(Private, &GifFile->Image.Top) == GIF_ERROR ||
	DGifGetWord(Private, &GifFile->Image.Width) == GIF_ERROR ||
	DGifGetWord(Private, &GifFile->Image.Height) == GIF_ERROR)
	return GIF_ERROR;
    if (my_fread(Buf, 1, 1, Private) != 1) {
	_GifError = D_GIF_ERR_READ_FAILED;
	return GIF_ERROR;
    }
    BitsPerPixel = (Buf[0] & 0x07) + 1;
    GifFile->Image.Interlace = (Buf[0] & 0x40);
    if (Buf[0] & 0x80) {	    /* Does this image have local color map? */

	if (GifFile->Image.ColorMap)
	    FreeMapObject(GifFile->Image.ColorMap);

	GifFile->Image.ColorMap = MakeMapObject(1 << BitsPerPixel, NULL);
    
	/* Get the image local color map: */
	for (i = 0; i < GifFile->Image.ColorMap->ColorCount; i++) {
	    if (my_fread(Buf, 1, 3, Private) != 3) {
		_GifError = D_GIF_ERR_READ_FAILED;
		return GIF_ERROR;
	    }
	    GifFile->Image.ColorMap->Colors[i].Red = Buf[0];
	    GifFile->Image.ColorMap->Colors[i].Green = Buf[1];
	    GifFile->Image.ColorMap->Colors[i].Blue = Buf[2];
	}
    }

    GifFile->ImageCount++;

    Private->PixelCount = (s32) GifFile->Image.Width *
			    (s32) GifFile->Image.Height;

    DGifSetupDecompress(GifFile);  /* Reset decompress algorithm parameters. */

    return GIF_OK;
}

/******************************************************************************
* Put one pixel (Pixel) into GIF file.					      *
******************************************************************************/
int DGifGetPixel(GifFileType *GifFile, GifPixelType *Pixel)
{
    GifByteType *Dummy;
    GifFilePrivateType *Private = (GifFilePrivateType *) GifFile->Private;

    if (!IS_READABLE(Private)) {
	/* This file was NOT open for reading: */
	_GifError = D_GIF_ERR_NOT_READABLE;
	return GIF_ERROR;
    }

#if defined(__MSDOS__) || defined(__GNUC__)
    if (--Private->PixelCount > 0xffff0000UL)
#else
    if (--Private->PixelCount > 0xffff0000)
#endif /* __MSDOS__ */
    {
	_GifError = D_GIF_ERR_DATA_TOO_BIG;
	return GIF_ERROR;
    }

    if (DGifDecompressLine(GifFile, Pixel, 1) == GIF_OK) {
	if (Private->PixelCount == 0) {
	    /* We probably would not be called any more, so lets clean 	     */
	    /* everything before we return: need to flush out all rest of    */
	    /* image until empty block (size 0) detected. We use GetCodeNext.*/
	    do if (DGifGetCodeNext(GifFile, &Dummy) == GIF_ERROR)
		return GIF_ERROR;
	    while (Dummy != NULL);
	}
	return GIF_OK;
    }
    else
	return GIF_ERROR;
}

/******************************************************************************
*   Get an extension block (see GIF manual) from gif file. This routine only  *
* returns the first data block, and DGifGetExtensionNext shouldbe called      *
* after this one until NULL extension is returned.			      *
*   The Extension should NOT be freed by the user (not dynamically allocated).*
*   Note it is assumed the Extension desc. header ('!') has been read.	      *
******************************************************************************/
int DGifGetExtension(GifFileType *GifFile, int *ExtCode,
						    GifByteType **Extension)
{
    GifByteType Buf;
    GifFilePrivateType *Private = (GifFilePrivateType *) GifFile->Private;

    if (!IS_READABLE(Private)) {
	/* This file was NOT open for reading: */
	_GifError = D_GIF_ERR_NOT_READABLE;
	return GIF_ERROR;
    }

    if (my_fread(&Buf, 1, 1, Private) != 1) {
	_GifError = D_GIF_ERR_READ_FAILED;
	return GIF_ERROR;
    }
    *ExtCode = Buf;

    return DGifGetExtensionNext(GifFile, Extension);
}

/******************************************************************************
*   Get a following extension block (see GIF manual) from gif file. This      *
* routine sould be called until NULL Extension is returned.		      *
*   The Extension should NOT be freed by the user (not dynamically allocated).*
******************************************************************************/
int DGifGetExtensionNext(GifFileType *GifFile, GifByteType **Extension)
{
    GifByteType Buf;
    GifFilePrivateType *Private = (GifFilePrivateType *) GifFile->Private;

    if (my_fread(&Buf, 1, 1, Private) != 1) {
	_GifError = D_GIF_ERR_READ_FAILED;
	return GIF_ERROR;
    }
    if (Buf > 0) {
	*Extension = Private->Buf;           /* Use private unused buffer. */
	(*Extension)[0] = Buf;  /* Pascal strings notation (pos. 0 is len.). */
	if (my_fread(&((*Extension)[1]), 1, Buf, Private) != Buf) {
	    _GifError = D_GIF_ERR_READ_FAILED;
	    return GIF_ERROR;
	}
    }
    else
	*Extension = NULL;

    return GIF_OK;
}

/******************************************************************************
*   This routine should be called last, to close the GIF file.		      *
******************************************************************************/
int DGifCloseFile(GifFileType *GifFile)
{
    GifFilePrivateType *Private;

    if (GifFile == NULL) return GIF_ERROR;

    Private = (GifFilePrivateType *) GifFile->Private;

    if (!IS_READABLE(Private)) {
	/* This file was NOT open for reading: */
	_GifError = D_GIF_ERR_NOT_READABLE;
	return GIF_ERROR;
    }

    if (GifFile->Image.ColorMap)
	FreeMapObject(GifFile->Image.ColorMap);
    if (GifFile->SColorMap)
	FreeMapObject(GifFile->SColorMap);
    if (Private)
	free((char *) Private);
    free(GifFile);

    return GIF_OK;
}

/******************************************************************************
*   Get 2 bytes (word) from the given file:				      *
******************************************************************************/
static int DGifGetWord(GifFilePrivateType *p, int *Word)
{
    const u8 *c;

    if (p->input_len < 2) {
	_GifError = D_GIF_ERR_READ_FAILED;
	return GIF_ERROR;
    }
    
    c = p->input;
    p->input_len -= 2;
    p->input += 2;

    *Word = (((unsigned int) c[1]) << 8) + c[0];
    return GIF_OK;
}

/******************************************************************************
*   Get the image code in compressed form.  his routine can be called if the  *
* information needed to be piped out as is. Obviously this is much faster     *
* than decoding and encoding again. This routine should be followed by calls  *
* to DGifGetCodeNext, until NULL block is returned.			      *
*   The block should NOT be freed by the user (not dynamically allocated).    *
******************************************************************************/
int DGifGetCode(GifFileType *GifFile, int *CodeSize, GifByteType **CodeBlock)
{
    GifFilePrivateType *Private = (GifFilePrivateType *) GifFile->Private;

    if (!IS_READABLE(Private)) {
	/* This file was NOT open for reading: */
	_GifError = D_GIF_ERR_NOT_READABLE;
	return GIF_ERROR;
    }

    *CodeSize = Private->BitsPerPixel;

    return DGifGetCodeNext(GifFile, CodeBlock);
}

/******************************************************************************
*   Continue to get the image code in compressed form. This routine should be *
* called until NULL block is returned.					      *
*   The block should NOT be freed by the user (not dynamically allocated).    *
******************************************************************************/
int DGifGetCodeNext(GifFileType *GifFile, GifByteType **CodeBlock)
{
    GifByteType Buf;
    GifFilePrivateType *Private = (GifFilePrivateType *) GifFile->Private;

    if (my_fread(&Buf, 1, 1, Private) != 1) {
	_GifError = D_GIF_ERR_READ_FAILED;
	return GIF_ERROR;
    }

    if (Buf > 0) {
	*CodeBlock = Private->Buf;	       /* Use private unused buffer. */
	(*CodeBlock)[0] = Buf;  /* Pascal strings notation (pos. 0 is len.). */
	if (my_fread(&((*CodeBlock)[1]), 1, Buf, Private) != Buf) {
	    _GifError = D_GIF_ERR_READ_FAILED;
	    return GIF_ERROR;
	}
    }
    else {
	*CodeBlock = NULL;
	Private->Buf[0] = 0;		   /* Make sure the buffer is empty! */
	Private->PixelCount = 0;   /* And local info. indicate image read. */
    }

    return GIF_OK;
}

/******************************************************************************
*   Setup the LZ decompression for this image:				      *
******************************************************************************/
static int DGifSetupDecompress(GifFileType *GifFile)
{
    int i, BitsPerPixel;
    GifByteType CodeSize;
    unsigned int *Prefix;
    GifFilePrivateType *Private = (GifFilePrivateType *) GifFile->Private;

    my_fread(&CodeSize, 1, 1, Private);    /* Read Code size from file. */
    BitsPerPixel = CodeSize;

    Private->Buf[0] = 0;			      /* Input Buffer empty. */
    Private->BitsPerPixel = BitsPerPixel;
    Private->ClearCode = (1 << BitsPerPixel);
    Private->EOFCode = Private->ClearCode + 1;
    Private->RunningCode = Private->EOFCode + 1;
    Private->RunningBits = BitsPerPixel + 1;	 /* Number of bits per code. */
    Private->MaxCode1 = 1 << Private->RunningBits;     /* Max. code + 1. */
    Private->StackPtr = 0;		    /* No pixels on the pixel stack. */
    Private->LastCode = NO_SUCH_CODE;
    Private->CrntShiftState = 0;	/* No information in CrntShiftDWord. */
    Private->CrntShiftDWord = 0;

    Prefix = Private->Prefix;
    for (i = 0; i <= LZ_MAX_CODE; i++) Prefix[i] = NO_SUCH_CODE;

    return GIF_OK;
}

/******************************************************************************
*   The LZ decompression routine:					      *
*   This version decompress the given gif file into Line of length LineLen.   *
*   This routine can be called few times (one per scan line, for example), in *
* order the complete the whole image.					      *
******************************************************************************/
static int DGifDecompressLine(GifFileType *GifFile, GifPixelType *Line,
								int LineLen)
{
    int i = 0, j, CrntCode, EOFCode, ClearCode, CrntPrefix, LastCode, StackPtr;
    GifByteType *Stack, *Suffix;
    unsigned int *Prefix;
    GifFilePrivateType *Private = (GifFilePrivateType *) GifFile->Private;

    StackPtr = Private->StackPtr;
    Prefix = Private->Prefix;
    Suffix = Private->Suffix;
    Stack = Private->Stack;
    EOFCode = Private->EOFCode;
    ClearCode = Private->ClearCode;
    LastCode = Private->LastCode;

    if (StackPtr != 0) {
	/* Let pop the stack off before continueing to read the gif file: */
	while (StackPtr != 0 && i < LineLen) Line[i++] = Stack[--StackPtr];
    }

    while (i < LineLen) {			    /* Decode LineLen items. */
	if (DGifDecompressInput(Private, &CrntCode) == GIF_ERROR)
    	    return GIF_ERROR;

	if (CrntCode == EOFCode) {
	    /* Note however that usually we will not be here as we will stop */
	    /* decoding as soon as we got all the pixel, or EOF code will    */
	    /* not be read at all, and DGifGetLine/Pixel clean everything.   */
	    if (i != LineLen - 1 || Private->PixelCount != 0) {
		_GifError = D_GIF_ERR_EOF_TOO_SOON;
		return GIF_ERROR;
	    }
	    i++;
	}
	else if (CrntCode == ClearCode) {
	    /* We need to start over again: */
	    for (j = 0; j <= LZ_MAX_CODE; j++) Prefix[j] = NO_SUCH_CODE;
	    Private->RunningCode = Private->EOFCode + 1;
	    Private->RunningBits = Private->BitsPerPixel + 1;
	    Private->MaxCode1 = 1 << Private->RunningBits;
	    LastCode = Private->LastCode = NO_SUCH_CODE;
	}
	else {
	    /* Its regular code - if in pixel range simply add it to output  */
	    /* stream, otherwise trace to codes linked list until the prefix */
	    /* is in pixel range:					     */
	    if (CrntCode < ClearCode) {
		/* This is simple - its pixel scalar, so add it to output:   */
		Line[i++] = CrntCode;
	    }
	    else {
		/* Its a code to needed to be traced: trace the linked list  */
		/* until the prefix is a pixel, while pushing the suffix     */
		/* pixels on our stack. If we done, pop the stack in reverse */
		/* (thats what stack is good for!) order to output.	     */
		if (Prefix[CrntCode] == NO_SUCH_CODE) {
		    /* Only allowed if CrntCode is exactly the running code: */
		    /* In that case CrntCode = XXXCode, CrntCode or the	     */
		    /* prefix code is last code and the suffix char is	     */
		    /* exactly the prefix of last code!			     */
		    if (CrntCode == Private->RunningCode - 2) {
			CrntPrefix = LastCode;
			Suffix[Private->RunningCode - 2] =
			Stack[StackPtr++] = DGifGetPrefixChar(Prefix,
							LastCode, ClearCode);
		    }
		    else {
			_GifError = D_GIF_ERR_IMAGE_DEFECT;
			return GIF_ERROR;
		    }
		}
		else
		    CrntPrefix = CrntCode;

		/* Now (if image is O.K.) we should not get an NO_SUCH_CODE  */
		/* During the trace. As we might loop forever, in case of    */
		/* defective image, we count the number of loops we trace    */
		/* and stop if we got LZ_MAX_CODE. obviously we can not      */
		/* loop more than that.					     */
		j = 0;
		while (j++ <= LZ_MAX_CODE &&
		       CrntPrefix > ClearCode &&
		       CrntPrefix <= LZ_MAX_CODE) {
		    Stack[StackPtr++] = Suffix[CrntPrefix];
		    CrntPrefix = Prefix[CrntPrefix];
		}
		if (j >= LZ_MAX_CODE || CrntPrefix > LZ_MAX_CODE) {
		    _GifError = D_GIF_ERR_IMAGE_DEFECT;
		    return GIF_ERROR;
		}
		/* Push the last character on stack: */
		Stack[StackPtr++] = CrntPrefix;

		/* Now lets pop all the stack into output: */
		while (StackPtr != 0 && i < LineLen)
		    Line[i++] = Stack[--StackPtr];
	    }
	    if (LastCode != NO_SUCH_CODE) {
		Prefix[Private->RunningCode - 2] = LastCode;

		if (CrntCode == Private->RunningCode - 2) {
		    /* Only allowed if CrntCode is exactly the running code: */
		    /* In that case CrntCode = XXXCode, CrntCode or the	     */
		    /* prefix code is last code and the suffix char is	     */
		    /* exactly the prefix of last code!			     */
		    Suffix[Private->RunningCode - 2] =
			DGifGetPrefixChar(Prefix, LastCode, ClearCode);
		}
		else {
		    Suffix[Private->RunningCode - 2] =
			DGifGetPrefixChar(Prefix, CrntCode, ClearCode);
		}
	    }
	    LastCode = CrntCode;
	}
    }

    Private->LastCode = LastCode;
    Private->StackPtr = StackPtr;

    return GIF_OK;
}

/******************************************************************************
* Routine to trace the Prefixes linked list until we get a prefix which is    *
* not code, but a pixel value (less than ClearCode). Returns that pixel value.*
* If image is defective, we might loop here forever, so we limit the loops to *
* the maximum possible if image O.k. - LZ_MAX_CODE times.		      *
******************************************************************************/
static int DGifGetPrefixChar(unsigned int *Prefix, int Code, int ClearCode)
{
    int i = 0;

    while (Code > ClearCode && i++ <= LZ_MAX_CODE) Code = Prefix[Code];
    return Code;
}

/******************************************************************************
*   Interface for accessing the LZ codes directly. Set Code to the real code  *
* (12bits), or to -1 if EOF code is returned.				      *
******************************************************************************/
int DGifGetLZCodes(GifFileType *GifFile, int *Code)
{
    GifByteType *CodeBlock;
    GifFilePrivateType *Private = (GifFilePrivateType *) GifFile->Private;

    if (!IS_READABLE(Private)) {
	/* This file was NOT open for reading: */
	_GifError = D_GIF_ERR_NOT_READABLE;
	return GIF_ERROR;
    }

    if (DGifDecompressInput(Private, Code) == GIF_ERROR)
	return GIF_ERROR;

    if (*Code == Private->EOFCode) {
	/* Skip rest of codes (hopefully only NULL terminating block): */
	do if (DGifGetCodeNext(GifFile, &CodeBlock) == GIF_ERROR)
    	    return GIF_ERROR;
	while (CodeBlock != NULL);

	*Code = -1;
    }
    else if (*Code == Private->ClearCode) {
	/* We need to start over again: */
	Private->RunningCode = Private->EOFCode + 1;
	Private->RunningBits = Private->BitsPerPixel + 1;
	Private->MaxCode1 = 1 << Private->RunningBits;
    }

    return GIF_OK;
}

/******************************************************************************
*   The LZ decompression input routine:					      *
*   This routine is responsable for the decompression of the bit stream from  *
* 8 bits (bytes) packets, into the real codes.				      *
*   Returns GIF_OK if read succesfully.					      *
******************************************************************************/
static int DGifDecompressInput(GifFilePrivateType *Private, int *Code)
{
    GifByteType NextByte;
    static unsigned int CodeMasks[] = {
	0x0000, 0x0001, 0x0003, 0x0007,
	0x000f, 0x001f, 0x003f, 0x007f,
	0x00ff, 0x01ff, 0x03ff, 0x07ff,
	0x0fff
    };

    while (Private->CrntShiftState < Private->RunningBits) {
	/* Needs to get more bytes from input stream for next code: */
	if (DGifBufferedInput(Private, Private->Buf, &NextByte)
	    == GIF_ERROR) {
	    return GIF_ERROR;
	}
	Private->CrntShiftDWord |=
		((u32) NextByte) << Private->CrntShiftState;
	Private->CrntShiftState += 8;
    }
    *Code = Private->CrntShiftDWord & CodeMasks[Private->RunningBits];

    Private->CrntShiftDWord >>= Private->RunningBits;
    Private->CrntShiftState -= Private->RunningBits;

    /* If code cannt fit into RunningBits bits, must raise its size. Note */
    /* however that codes above 4095 are used for special signaling.      */
    if (++Private->RunningCode > Private->MaxCode1 &&
	Private->RunningBits < LZ_BITS) {
	Private->MaxCode1 <<= 1;
	Private->RunningBits++;
    }
    return GIF_OK;
}

/******************************************************************************
*   This routines read one gif data block at a time and buffers it internally *
* so that the decompression routine could access it.			      *
*   The routine returns the next byte from its internal buffer (or read next  *
* block in if buffer empty) and returns GIF_OK if succesful.		      *
******************************************************************************/
static int DGifBufferedInput(GifFilePrivateType *File, GifByteType *Buf,
						      GifByteType *NextByte)
{
    if (Buf[0] == 0) {
	/* Needs to read the next buffer - this one is empty: */
	if (my_fread(Buf, 1, 1, File) != 1)
	{
	    _GifError = D_GIF_ERR_READ_FAILED;
	    return GIF_ERROR;
	}
	if (my_fread(&Buf[1], 1, Buf[0], File) != Buf[0])
	{
	    _GifError = D_GIF_ERR_READ_FAILED;
	    return GIF_ERROR;
	}
	*NextByte = Buf[1];
	Buf[1] = 2;	   /* We use now the second place as last char read! */
	Buf[0]--;
    }
    else {
	*NextByte = Buf[Buf[1]++];
	Buf[0]--;
    }

    return GIF_OK;
}

/******************************************************************************
* Miscellaneous utility functions					      *
******************************************************************************/

int BitSize(int n)
/* return smallest bitfield size n will fit in */
{
    register int i;

    for (i = 1; i <= 8; i++)
	if ((1 << i) >= n)
	    break;
    return(i);
}


/******************************************************************************
* Color map object functions						      *
******************************************************************************/

ColorMapObject *MakeMapObject(int ColorCount, GifColorType *ColorMap)
/*
 * Allocate a color map of given size; initialize with contents of
 * ColorMap if that pointer is non-NULL.
 */
{
    ColorMapObject *Object;

    if (ColorCount != (1 << BitSize(ColorCount)))
	return((ColorMapObject *)NULL);

    Object = (ColorMapObject *)malloc(sizeof(ColorMapObject));
    if (Object == (ColorMapObject *)NULL)
	return((ColorMapObject *)NULL);

    Object->Colors = (GifColorType *)calloc(ColorCount, sizeof(GifColorType));
    if (Object->Colors == (GifColorType *)NULL)
	return((ColorMapObject *)NULL);

    Object->ColorCount = ColorCount;
    Object->BitsPerPixel = BitSize(ColorCount);

    if (ColorMap)
	memcpy((char *)Object->Colors,
	       (char *)ColorMap, ColorCount * sizeof(GifColorType));

    return(Object);
}

void FreeMapObject(ColorMapObject *Object)
/*
 * Free a color map object
 */
{
    free(Object->Colors);
    free(Object);
}

/* Transcribe one scanline from the GIF to the picogui bitmap */
void transcribe_line(hwrbitmap b, GifFileType *f, int y, 
		     int transparent_flag, int transparent_color) {
  pgcolor c;
  u8 pixel;
  GifColorType *color;
  int x;

  for (x=0;x<f->Image.Width;x++) {
    DGifGetPixel(f, &pixel);
    color = f->SColorMap->Colors + (pixel % f->SColorMap->ColorCount);
    if (transparent_flag) {
      c = mkcolora(pixel==transparent_color ? 0 : 127,color->Red,color->Green,color->Blue);
    }
    else {
      c = mkcolor(color->Red,color->Green,color->Blue);
    }
    vid->pixel(b,x,y,VID(color_pgtohwr)(c),PG_LGOP_NONE);
  }
}

#ifdef CONFIG_DITHER
/* Transcribe one scanline from the GIF to the picogui bitmap */
void dither_transcribe_line(hwrdither d, GifFileType *f, 
		     int transparent_flag, int transparent_color) {
  pgcolor c;
  u8 pixel;
  GifColorType *color;
  int x;

  for (x=0;x<f->Image.Width;x++) {
    DGifGetPixel(f, &pixel);
    color = f->SColorMap->Colors + (pixel % f->SColorMap->ColorCount);
    if (transparent_flag) {
      c = mkcolora(pixel==transparent_color ? 0 : 127,color->Red,color->Green,color->Blue);
    }
    else {
      c = mkcolor(color->Red,color->Green,color->Blue);
    }
    vid->dither_store(d,c,PG_LGOP_NONE);
  }
}
#endif

/****************** PicoGUI public functions */

bool gif_detect(const u8 *data, u32 datalen) {
  /* Too short? */
  if (datalen < GIF_STAMP_LEN)
    return 0;

  /* Bad stamp? */
  if (strncmp(GIF_STAMP, data, GIF_VERSION_POS))
    return 0;
  
  return 1;
}

g_error gif_load(hwrbitmap *hbmp, const u8 *data, u32 datalen) {
  GifFileType *f = DGifOpenFileMemory(data,datalen);
  GifRecordType RecordType;
  int x,y;
  g_error e;
  GifByteType *Extension;
  int ExtCode;
  int transparent_color = 0;
  int transparent_flag = 0;
#ifdef CONFIG_DITHER
  hwrdither dither;
#endif
  
  if (!f)
    return mkerror(PG_ERRT_IO,72);   /* Error reading GIF */

  /* Chomp through any extension records preceeding the image */
  do {

    if (DGifGetRecordType(f, &RecordType) == GIF_ERROR) {
      DGifCloseFile(f);
      return mkerror(PG_ERRT_IO,72);   /* Error reading GIF */
    }

    switch (RecordType) {

    case EXTENSION_RECORD_TYPE:
      if (!DGifGetExtension(f, &ExtCode, &Extension)) {
	DGifCloseFile(f);
	return mkerror(PG_ERRT_IO,72);   /* Error reading GIF */
      }
      while (Extension) {
	/*
	 * Process graphic control label extension blocks
	 */
	if (ExtCode == 0xF9) {
	  transparent_color = Extension[4];
	  transparent_flag  = Extension[1] & 1;
	}
	if (!DGifGetExtensionNext(f, &Extension)) {
	  DGifCloseFile(f);
	  return mkerror(PG_ERRT_IO,72);   /* Error reading GIF */
	}
      }
      break;
      
    case IMAGE_DESC_RECORD_TYPE:
      break;

    default:
      DGifCloseFile(f);
      return mkerror(PG_ERRT_IO,72);   /* Error reading GIF */
    }

  } while (RecordType != IMAGE_DESC_RECORD_TYPE);

  if (DGifGetImageDesc(f) == GIF_ERROR) {
    DGifCloseFile(f);
    return mkerror(PG_ERRT_IO,72);   /* Error reading GIF */
  }

  /* Make sure we have a color map */
  if (!f->SColorMap) {
    DGifCloseFile(f);
    return mkerror(PG_ERRT_IO,72);   /* Error reading GIF */
  }

  /* Now we know how big the image is, so allocate it.
   * Note that if we have an alpha channel, we must use a 32bpp
   * image to hold the ARGB colors with the PGCF_ALPHA flag.
   */
  if (transparent_flag)
    e = vid->bitmap_new(hbmp,f->Image.Width, f->Image.Height, 32);
  else
    e = vid->bitmap_new(hbmp,f->Image.Width, f->Image.Height, vid->bpp);
  if (iserror(e))
    DGifCloseFile(f);
  errorcheck;

#ifdef CONFIG_DITHER

  /* Start dithering */
  e = vid->dither_start(&dither, *hbmp, 0,0,0,f->Image.Width, f->Image.Height);
  errorcheck;

  /* Interlaced images are a pain to dither, we have to create a temporary buffer
   * to read them in to, then dither from there. Noninterlaced gifs are simple.
   */
  if (f->Image.Interlace) {
    hwrbitmap tempbmp;

    e = vid->bitmap_new(&tempbmp, f->Image.Width, f->Image.Height, 32);
    errorcheck;

    for (y=0;y<f->Image.Height;y+=8)
      transcribe_line(tempbmp, f, y, transparent_flag, transparent_color);

    for (y=4;y<f->Image.Height;y+=8)
      transcribe_line(tempbmp, f, y, transparent_flag, transparent_color);

    for (y=2;y<f->Image.Height;y+=4)
      transcribe_line(tempbmp, f, y, transparent_flag, transparent_color);

    for (y=1;y<f->Image.Height;y+=2)
      transcribe_line(tempbmp, f, y, transparent_flag, transparent_color);

    for (y=0;y<f->Image.Height;y++)
      for (x=0;x<f->Image.Width;x++)
	vid->dither_store(dither, vid->getpixel(tempbmp,x,y),PG_LGOP_NONE);
    
    vid->bitmap_free(tempbmp);
  }
  else {
    /* Non-interlaced */
    
    for (y=0;y<f->Image.Height;y++)
      dither_transcribe_line(dither, f, transparent_flag, transparent_color);
  }
  
  vid->dither_finish(dither);

#else /* !CONFIG_DITHER */

  if (f->Image.Interlace) {
    /* Interlaced */
    
    for (y=0;y<f->Image.Height;y+=8)
      transcribe_line(*hbmp, f, y, transparent_flag, transparent_color);

    for (y=4;y<f->Image.Height;y+=8)
      transcribe_line(*hbmp, f, y, transparent_flag, transparent_color);

    for (y=2;y<f->Image.Height;y+=4)
      transcribe_line(*hbmp, f, y, transparent_flag, transparent_color);

    for (y=1;y<f->Image.Height;y+=2)
      transcribe_line(*hbmp, f, y, transparent_flag, transparent_color);
  }
  else {
    /* Non-interlaced */

    for (y=0;y<f->Image.Height;y++)
      transcribe_line(*hbmp, f, y, transparent_flag, transparent_color);
  }

#endif /* CONFIG_DITHER */
  
  DGifCloseFile(f);
  return success;
}

/* The End */

