/* This file is from vgagl */

/* Based on functions in linux/string.h */

#if !defined(ASM_INTEL)

#include <string.h>

#define __memcpy(dst,src,n)			memcpy((dst),(src),(n))
#define __memcpy_conventional(dst,src,n)	memcpy((dst),(src),(n))
#define __memcpyb(dst,src,n)			memcpy((dst),(src),(n))
#define __memsetb(dst,c,n)			memset((dst),(c),(n))
#define __memsetlong(dst,c,n)			memset((dst),(c),(n))
#define __memset(dst,c,n)			memset((dst),(c),(n))
#define __memset2(dst,c,n)			memset((dst),(c),2*(n))
#define __memset3(dst,c,n)			memset((dst),(c),3*(n))

#else

# include <sys/types.h>	/* for size_t */

static inline void *
 __memcpy_conventional(void *to, const void *from, size_t n)
{
  int dummy1;
  s32 dummy2, dummy3;
    __asm__ __volatile__("cld\n\t"
    	    "cmpl $0,%%edx\n\t"
    	    "jle 2f\n\t"
	    "movl %%edi,%%ecx\n\t"
	    "andl $1,%%ecx\n\t"
	    "subl %%ecx,%%edx\n\t"
	    "rep ; movsb\n\t"	/* 16-bit align destination */
	    "movl %%edx,%%ecx\n\t"
	    "shrl $2,%%ecx\n\t"
	    "jz 3f\n\t"
	    "rep ; movsl\n\t"
	    "3:\n\t"
	    "testb $1,%%dl\n\t"
	    "je 1f\n\t"
	    "movsb\n"
	    "1:\ttestb $2,%%dl\n\t"
	    "je 2f\n\t"
	    "movsw\n"
	    "2:\n"
  :         "=d"(dummy1), "=D"(dummy2), "=S"(dummy3)   /* fake output */ 
  :	    "0"(n), "1"((s32) to), "2"((s32) from)
  :	    "cx"/***rjr***, "dx", "di", "si"***/
  );
    return (to);
}


static inline void *
 __memcpyb(void *to, const void *from, size_t n)
{
  int dummy1;
  s32 dummy2, dummy3;
    __asm__ __volatile__("cld\n\t"
	    "rep ; movsb\n\t"
  :         "=c"(dummy1), "=D"(dummy2), "=S"(dummy3) /* fake output */
  :	    "0"(n), "1"((s32) to), "2"((s32) from)
			 /***rjr***:	    "cx", "di", "si"***/
  );
    return (to);
}

static inline void *
 __memsetb(void *s, char c, size_t count)
{
    __asm__("cld\n\t"
	    "rep\n\t"
	    "stosb"
  : :	    "a"(c), "D"(s), "c"(count)
  :	    "cx", "di");
    return s;
}

static inline void *
 __memsetlong(void *s, unsigned c, size_t count)
{
  s32 dummy1;
  int dummy2;
    __asm__ __volatile__("cld\n\t"
	    "rep\n\t"
	    "stosl"
  :         "=D"(dummy1), "=c"(dummy2) /* fake outputs */
  :	    "a"(c), "0"(s), "1"(count)
			 /***rjr***:	    "cx", "di"***/
  );
    return s;
}

static inline void *
 __memset(void *s, char c, size_t count)
{
  int dummy1;
  s32 dummy2;
  int dummy3;
    __asm__ __volatile__(
	       "cld\n\t"
	       "cmpl $12,%%edx\n\t"
	       "jl 1f\n\t"	/* if (count >= 12) */

	       "movzbl %%al,%%eax\n\t"
	       "movl %%eax,%%ecx\n\t"
	       "shll $8,%%ecx\n\t"	/* c |= c << 8 */
	       "orl %%ecx,%%eax\n\t"
	       "movl %%eax,%%ecx\n\t"
	       "shll $16,%%ecx\n\t"	/* c |= c << 16 */
	       "orl %%ecx,%%eax\n\t"

	       "movl %%edx,%%ecx\n\t"
	       "negl %%ecx\n\t"
	       "andl $3,%%ecx\n\t"	/* (-s % 4) */
	       "subl %%ecx,%%edx\n\t"	/* count -= (-s % 4) */
	       "rep ; stosb\n\t"	/* align to longword boundary */

	       "movl %%edx,%%ecx\n\t"
	       "shrl $2,%%ecx\n\t"
	       "rep ; stosl\n\t"	/* fill longwords */

	       "andl $3,%%edx\n"	/* fill last few bytes */
	       "1:\tmovl %%edx,%%ecx\n\t"	/* <= 12 entry point */
	       "rep ; stosb\n\t"
  :            "=a"(dummy1), "=D"(dummy2), "=d"(dummy3) /* fake outputs */
  :	       "0"(c), "1"(s), "2"(count)
  :	       /***rjr***"ax",*/ "cx"/*, "dx", "di"*/
  );
    return s;
}

static inline void *
 __memset2(void *s, s16 c, size_t count)
/* count is in 16-bit pixels */
/* s is assumed to be 16-bit aligned */
{
  int dummy1;
  s32 dummy2;
  int dummy3;
    __asm__ __volatile__(
	       "cld\n\t"
	       "cmpl $12,%%edx\n\t"
	       "jl 1f\n\t"	/* if (count >= 12) */

	       "movzwl %%ax,%%eax\n\t"
	       "movl %%eax,%%ecx\n\t"
	       "shll $16,%%ecx\n\t"	/* c |= c << 16 */
	       "orl %%ecx,%%eax\n\t"

	       "movl %%edi,%%ecx\n\t"
	       "andl $2,%%ecx\n\t"	/* s & 2 */
	       "jz 2f\n\t"
	       "decl %%edx\n\t"	/* count -= 1 */
	       "movw %%ax,(%%edi)\n\t"	/* align to longword boundary */
	       "addl $2,%%edi\n\t"

	       "2:\n\t"
	       "movl %%edx,%%ecx\n\t"
	       "shrl $1,%%ecx\n\t"
	       "rep ; stosl\n\t"	/* fill longwords */

	       "andl $1,%%edx\n"	/* one 16-bit word left? */
	       "jz 3f\n\t"	/* no, finished */
	       "1:\tmovl %%edx,%%ecx\n\t"	/* <= 12 entry point */
	       "rep ; stosw\n\t"
	       "3:\n\t"
  :            "=a"(dummy1), "=D"(dummy2), "=d"(dummy3) /* fake outputs */
  :	       "0"(c), "1"(s), "2"(count)
  :	       /***rjr***"ax",*/ "cx"/*, "dx", "di"*/
  );
    return s;
}

static inline void *
 __memset3(void *s, int c, size_t count)
/* count is in 24-bit pixels (3 bytes per pixel) */
{
  int dummy1;
  s32 dummy2;
  int dummy3;
    __asm__ __volatile__(
	       "cmpl $8,%%edx\n\t"
    /*      "jmp 2f\n\t" *//* debug */
	       "jl 2f\n\t"

	       "movl %%eax,%%esi\n\t"	/* esi = (low) BGR0 (high) */
	       "shll $24,%%eax\n\t"	/* eax = 000B */
	       "orl %%eax,%%esi\n\t"	/* esi = BGRB */

	       "movl %%esi,%%eax\n\t"
	       "shrl $8,%%eax\n\t"	/* eax = GRB0 */
	       "movl %%eax,%%ecx\n\t"
	       "shll $24,%%ecx\n\t"	/* ecx = 000G */
	       "orl %%ecx,%%eax\n\t"	/* eax = GRBG */

	       "movl %%esi,%%ecx\n\t"
	       "shll $8,%%ecx\n\t"	/* ecx = 0BGR */
	       "movb %%ah,%%cl\n\t"	/* ecx = RBGR */

	       "cmpl $16,%%edx\n\t"
	       "jl 1f\n\t"
	       "jmp 5f\n\t"
	       ".align 4,0x90\n\t"

	       "5:\n\t"		/* loop unrolling */
	       "movl %%esi,(%%edi)\n\t"		/* write BGRB */
	       "movl %%eax,4(%%edi)\n\t"	/* write GRBG */
	       "movl %%ecx,8(%%edi)\n\t"	/* write RBGR */
	       "movl %%esi,12(%%edi)\n\t"
	       "movl %%eax,16(%%edi)\n\t"
	       "movl %%ecx,20(%%edi)\n\t"
	       "movl %%esi,24(%%edi)\n\t"
	       "movl %%eax,28(%%edi)\n\t"
	       "movl %%ecx,32(%%edi)\n\t"
	       "movl %%esi,36(%%edi)\n\t"
	       "subl $16,%%edx\n\t"	/* blend end-of-loop instr. */
	       "movl %%eax,40(%%edi)\n\t"
	       "movl %%ecx,44(%%edi)\n\t"
	       "addl $48,%%edi\n\t"
	       "cmpl $16,%%edx\n\t"
	       "jge 5b\n\t"
	       "andl %%edx,%%edx\n\t"
	       "jz 4f\n\t"	/* finished */
	       "cmpl $4,%%edx\n\t"
	       "jl 2f\n\t"	/* less than 4 pixels left */
	       "jmp 1f\n\t"
	       ".align 4,0x90\n\t"

	       "1:\n\t"
	       "movl %%esi,(%%edi)\n\t"		/* write BGRB */
	       "movl %%eax,4(%%edi)\n\t"	/* write GRBG */
	       "movl %%ecx,8(%%edi)\n\t"	/* write RBGR */
	       "addl $12,%%edi\n\t"
	       "subl $4,%%edx\n\t"
	       "cmpl $4,%%edx\n\t"
	       "jge 1b\n\t"

	       "2:\n\t"
	       "cmpl $0,%%edx\n\t"	/* none left? */
	       "jle 4f\n\t"	/* finished */

	       "mov %%ecx,%%eax\n\t"
	       "shrl $8,%%ecx\n\t"	/* R in cl */

	       "3:\n\t"		/* write last few pixels */
	       "movw %%cx,(%%edi)\n\t"	/* write BG */
	       "movb %%al,2(%%edi)\n\t"		/* write R */
	       "addl $3,%%edi\n\t"
	       "decl %%edx\n\t"
	       "jnz 3b\n\t"

	       "4:\n\t"
  :            "=a"(dummy1), "=D"(dummy2), "=d"(dummy3) /* fake outputs */
  :	       "0"(c), "1"(s), "2"(count)
  :	       /***rjr***"ax",*/ "cx", /*"dx",*/ "si"/*, "di"*/
  );
    return s;
}

/* Functions defined in mem.S */

extern void __svgalib_memcpy4to3(void *dest, void *src, int n);
extern void __svgalib_memcpy32shift8(void *dest, void *src, int n);

/* Functions for which arguments must be passed in %ebx, %edx, and %ecx. */
#if 0				/* Why declare 'em? Just confuses the compiler and can't be called from C
				   anyway */
extern __memcpyasm_regargs();	/* nu_bytes >= 3 */
extern __memcpyasm_regargs_aligned();	/* nu_bytes >= 32 */
#endif


/* Always 32-bit align destination, even for a small number of bytes. */
static inline void *
 __memcpy_aligndest(void *dest, const void *src, int n)
{
    __asm__ __volatile__("cmpl $3, %%ecx\n\t"
			 "ja 1f\n\t"
			 "call * __memcpy_jumptable (, %%ecx, 4)\n\t"
			 "jmp 2f\n\t"
			 "1:call __memcpyasm_regargs\n\t"
			 "2:":
			 :"S"(dest), "d"(src), "c"(n)
			 :"ax", "0", "1", "2");
    return dest;
}


/* Optimized version for 32-bit aligned destination. */
static inline void *
 __memcpy_destaligned(void *dest, const void *src, int n)
{
    __asm__ __volatile__("cmpl $32, %%ecx\n\t"
			 "ja 1f\n\t"
			 "call * __memcpy_jumptable (, %%ecx, 4)\n\t"
			 "jmp 2f\n\t"
			 "1:call __memcpyasm_regargs_aligned\n\t"
			 "2:\n\t":
			 :"S"(dest), "d"(src), "c"(n)
			 :"ax", "0", "1", "2");
    return dest;
}


/* Balanced inline memcpy; 32-bit align destination if nu_bytes >= 20. */
static inline void *
 __memcpy_balanced(void *dest, const void *src, int n)
{
    __asm__ __volatile__("cmpl $19, %%ecx\n\t"
			 "ja 1f\n\t"
			 "call * __memcpy_jumptable (, %%ecx, 4)\n\t"
			 "jmp 2f\n\t"
			 "1:call __memcpyasm_regargs\n\t"
			 "2:\n\t"
			 :
			 :"S"((s32) dest), "d"((s32) src), "c"((s32) n)
			 :"ax", "0", "1", "2");
    return dest;
}


#define __memcpy __memcpy_conventional

#endif				/* !__alpha__ */
