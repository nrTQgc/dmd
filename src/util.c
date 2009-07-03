/*
 * Some portions copyright (c) 1984-1993 by Symantec
 * Copyright (c) 1999-2008 by Digital Mars
 * All Rights Reserved
 * http://www.digitalmars.com
 * Written by Walter Bright
 *
 * This source file is made available for personal use
 * only. The license is in /dmd/src/dmd/backendlicense.txt
 * For any other uses, please contact Digital Mars.
 */

// Utility subroutines

#include	<stdio.h>
#include	<ctype.h>
#include	<string.h>
#include	<stdlib.h>
#include	<time.h>

#include	"cc.h"
#include	"global.h"
#include	"mem.h"
#include	"token.h"
#if SCPP || MARS
#include	"el.h"
#endif
#include	"parser.h"

#if _WIN32 && __DMC__
//#include	"scdll.h"
#include	<controlc.h>
#endif

static char __file__[] = __FILE__;	/* for tassert.h		*/
#include	"tassert.h"

void util_exit(int exitcode);

void file_progress()
{
}

/*******************************
 * Alternative assert failure.
 */

void util_assert(char *file,int line)
{
    fflush(stdout);
    printf("Internal error: %s %d\n",file,line);
    err_exit();
}

/****************************
 * Clean up and exit program.
 */

void err_exit()
{
    util_exit(EXIT_FAILURE);
}

/********************************
 * Clean up and exit program.
 */

void err_break()
{
    util_exit(255);
}


/****************************
 * Clean up and exit program.
 */

void util_exit(int exitcode)
{
    exit(exitcode);			/* terminate abnormally		*/
}


#if _WIN32

volatile int controlc_saw;

/********************************
 * Control C interrupts go here.
 */

static void __cdecl controlc_handler(void)
{
    //printf("saw controlc\n");
    controlc_saw = 1;
}

/*********************************
 * Trap control C interrupts.
 */

void _STI_controlc()
{
    //printf("_STI_controlc()\n");
    _controlc_handler = controlc_handler;
    controlc_open();			/* trap control C		*/
}

void _STD_controlc()
{
    //printf("_STD_controlc()\n");
    controlc_close();
}


/***********************************
 * Send progress report.
 */

void util_progress()
{
    if (controlc_saw)
	err_break();
}

void util_progress(int linnum)
{
    if (controlc_saw)
	err_break();
}

#endif

#if linux || __APPLE__
void util_progress()
{
}

void util_progress(int linnum)
{
}
#endif

/**********************************
 * Binary string search.
 * Input:
 *	p ->	string of characters
 *	tab	array of pointers to strings
 *	n =	number of pointers in the array
 * Returns:
 *	index (0..n-1) into tab[] if we found a string match
 *	else -1
 */

#if !(TX86 && __INTSIZE == 4 && __SC__ && !_DEBUG_TRACE)

int binary(const char *p, const char __near * __near *table,int high)
{ int low,mid;
  signed char cond;
  char cp;

  low = 0;
  high--;
  cp = *p;
  p++;
  while (low <= high)
  {	mid = (low + high) >> 1;
	if ((cond = table[mid][0] - cp) == 0)
	    cond = strcmp(table[mid] + 1,p);
	if (cond > 0)
	    high = mid - 1;
	else if (cond < 0)
	    low = mid + 1;
	else
	    return mid;			/* match index			*/
  }
  return -1;
}

#else

int binary(const char *p, const char __near * __near *table,int high)
{
#define len high	// reuse parameter storage
    _asm
    {

;First find the length of the identifier.
	xor	EAX,EAX		;Scan for a 0.
	mov	EDI,p
	mov	ECX,EAX
	dec	ECX		;Longest possible string.
	repne	scasb
	mov	EDX,high	;EDX = high
	not	ECX		;length of the id including '/0', stays in ECX
	dec	EDX		;high--
	js	short Lnotfound
	dec	EAX		;EAX = -1, so that eventually EBX = low (0)
	mov	len,ECX

	even
L4D:	mov	EBX,EAX		;EBX (low) = mid
	inc	EBX		;low = mid + 1
	cmp	EBX,EDX
	jg	Lnotfound

	even
L15:	lea	EAX,[EBX + EDX] ;EAX = EBX + EDX

;Do the string compare.

	mov	EDI,table
	sar	EAX,1		;mid = (low + high) >> 1;
	mov	ESI,p
	mov	EDI,DS:[4*EAX+EDI] ;Load table[mid]
	mov	ECX,len		;length of id
	repe	cmpsb

	je	short L63	;return mid if equal
	jns	short L4D	;if (cond < 0)
	lea	EDX,-1[EAX]	;high = mid - 1
	cmp	EBX,EDX
	jle	L15

Lnotfound:
	mov	EAX,-1		;Return -1.

	even
L63:
    }
#undef len
}

#endif

#if !MACINTOSH
/**********************
 * If c is a power of 2, return that power else -1.
 */

int ispow2(unsigned long long c)
{	int i;

	if (c == 0 || (c & (c - 1)))
	    i = -1;
	else
	    for (i = 0; c >>= 1; i++)
		;
	return i;
}
#endif
/***************************
 */

#define UTIL_PH	1

#if _WIN32
void *util_malloc(unsigned n,unsigned size)
{
#if 0 && MEM_DEBUG
    void *p;

    p = mem_malloc(n * size);
    //dbg_printf("util_calloc(%d) = %p\n",n * size,p);
    return p;
#elif UTIL_PH
    return ph_malloc(n * size);
#else
    void *p;
    unsigned long nbytes;

    nbytes = (unsigned long) n * (unsigned long) size;
    p = malloc(nbytes);
    if (!p && (size_t)nbytes)
	err_nomem();
    return p;
#endif
}
#endif

/***************************
 */

#if _WIN32
void *util_calloc(unsigned n,unsigned size)
{
#if 0 && MEM_DEBUG
    void *p;

    p = mem_calloc(n * size);
    //dbg_printf("util_calloc(%d) = %p\n",n * size,p);
    return p;
#elif UTIL_PH
    return ph_calloc(n * size);
#else
    void *p;
    unsigned long nbytes;

    nbytes = (unsigned long) n * (unsigned long) size;
    p = calloc(n,size);
    if (!p && (size_t)nbytes)
	err_nomem();
    return p;
#endif
}
#endif

/***************************
 */

#if _WIN32
void util_free(void *p)
{
    //dbg_printf("util_free(%p)\n",p);
#if 0 && MEM_DEBUG
    mem_free(p);
#elif UTIL_PH
    ph_free(p);
#else
    free(p);
#endif
}
#endif

/***************************
 */

#if _WIN32
void *util_realloc(void *oldp,unsigned n,unsigned size)
{
#if 0 && MEM_DEBUG
    //dbg_printf("util_realloc(%p,%d)\n",oldp,n * size);
    return mem_realloc(oldp,n * size);
#elif UTIL_PH
    return ph_realloc(oldp,n * size);
#else
    void *p;
    unsigned long nbytes;

    nbytes = (unsigned long) n * (unsigned long) size;
    p = realloc(oldp,nbytes);
    if (!p && (size_t)nbytes)
	err_nomem();
    return p;
#endif
}
#endif
