/* $Id: benchmark.c,v 1.3 2002/10/20 15:19:32 micahjd Exp $
 *
 * benchmark.c - Run benchmarks on vidlib functions
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2002 Micah Dowty <micahjd@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * 
 * Contributors:
 * 
 * 
 * 
 */

#include <pgserver/common.h>
#include <pgserver/video.h>
#include <pgserver/configfile.h>

hwrbitmap benchmark_srcbit;
u8 *benchmark_char, *benchmark_gamma;
hwrcolor benchmark_color;

struct benchmark_param {
  int lgop;
  int w,h;
};

/************************************************************ Tests */

void bench_noop(struct benchmark_param *b) {
}

void bench_color_pgtohwr(struct benchmark_param *b) {
  benchmark_color = VID(color_pgtohwr)(0xFFFF00);
}

void bench_color_hwrtopg(struct benchmark_param *b) {
  VID(color_hwrtopg)(benchmark_color);
}

void bench_update(struct benchmark_param *b) {
  VID(update)(0,0,b->w,b->h);
}

void bench_pixel(struct benchmark_param *b) {
  VID(pixel)(vid->display,42,42,benchmark_color,b->lgop);
}

void bench_getpixel(struct benchmark_param *b) {
  VID(getpixel)(vid->display,42,42);
}

void bench_slab(struct benchmark_param *b) {
  VID(slab)(vid->display,0,0,b->w,benchmark_color,b->lgop);
}

void bench_bar(struct benchmark_param *b) {
  VID(bar)(vid->display,0,0,b->h,benchmark_color,b->lgop);
}

void bench_line(struct benchmark_param *b) {
  VID(line)(vid->display,0,0,b->w,b->h,benchmark_color,b->lgop);
}

void bench_rect(struct benchmark_param *b) {
  VID(rect)(vid->display,0,0,b->w,b->h,benchmark_color,b->lgop);
}

void bench_gradient(struct benchmark_param *b) {
  VID(gradient)(vid->display,0,0,b->w,b->h,0,0xFF0000,0x0000FF,b->lgop);
}

void bench_blit(struct benchmark_param *b) {
  VID(blit)(vid->display,0,0,b->w,b->h,benchmark_srcbit,0,0,b->lgop);
}

void bench_scrollblit_up(struct benchmark_param *b) {
  VID(scrollblit)(vid->display,0,0,b->w,b->h-1,vid->display,0,1,b->lgop);
}

void bench_scrollblit_down(struct benchmark_param *b) {
  VID(scrollblit)(vid->display,0,1,b->w,b->h-1,vid->display,0,0,b->lgop);
}

void bench_scrollblit_left(struct benchmark_param *b) {
  VID(scrollblit)(vid->display,0,0,b->w-1,b->h,vid->display,1,0,b->lgop);
}

void bench_scrollblit_right(struct benchmark_param *b) {
  VID(scrollblit)(vid->display,1,0,b->w-1,b->h,vid->display,0,0,b->lgop);
}

void bench_multiblit_1_1(struct benchmark_param *b) {
  VID(multiblit)(vid->display,0,0,b->w,b->h,benchmark_srcbit,0,0,1,1,0,0,b->lgop);
}

void bench_multiblit_1_128(struct benchmark_param *b) {
  VID(multiblit)(vid->display,0,0,b->w,b->h,benchmark_srcbit,0,0,1,128,0,0,b->lgop);
}

void bench_multiblit_128_1(struct benchmark_param *b) {
  VID(multiblit)(vid->display,0,0,b->w,b->h,benchmark_srcbit,0,0,128,1,0,0,b->lgop);

}

void bench_multiblit_128_128(struct benchmark_param *b) {
  VID(multiblit)(vid->display,0,0,b->w,b->h,benchmark_srcbit,0,0,128,128,0,0,b->lgop);
}

void bench_rotateblit_0(struct benchmark_param *b) {
  struct quad clip;
  clip.x1 = 0;
  clip.y1 = 0;
  clip.x2 = vid->lxres-1;
  clip.y2 = vid->lyres-1;
  VID(rotateblit)(vid->display,0,0,
		  benchmark_srcbit,0,0,b->w,b->h,&clip,0,b->lgop);
}

void bench_rotateblit_90(struct benchmark_param *b) {
  struct quad clip;
  clip.x1 = 0;
  clip.y1 = 0;
  clip.x2 = vid->lxres-1;
  clip.y2 = vid->lyres-1;
  VID(rotateblit)(vid->display,0,vid->lyres-1,
		  benchmark_srcbit,0,0,b->w,b->h,&clip,90,b->lgop);
}

void bench_rotateblit_180(struct benchmark_param *b) {
  struct quad clip;
  clip.x1 = 0;
  clip.y1 = 0;
  clip.x2 = vid->lxres-1;
  clip.y2 = vid->lyres-1;
  VID(rotateblit)(vid->display,vid->lxres-1,vid->lyres-1,
		  benchmark_srcbit,0,0,b->w,b->h,&clip,180,b->lgop);
}

void bench_rotateblit_270(struct benchmark_param *b) {
  struct quad clip;
  clip.x1 = 0;
  clip.y1 = 0;
  clip.x2 = vid->lxres-1;
  clip.y2 = vid->lyres-1;
  VID(rotateblit)(vid->display,vid->lxres-1,0,
		  benchmark_srcbit,0,0,b->w,b->h,&clip,270,b->lgop);
}

void bench_ellipse(struct benchmark_param *b) {
  VID(ellipse)(vid->display,0,0,b->w,b->h,0,b->lgop);
}

void bench_fellipse(struct benchmark_param *b) {
  VID(fellipse)(vid->display,0,0,b->w,b->h,0,b->lgop);
}

void bench_polygon_star(struct benchmark_param *b) {
  s32 star[] = {
    12,
    63,0,
    95,127,
    0,63,
    127,63,
    31,127,
    63,0
  };
  VID(fpolygon)(vid->display,star,0,0,0,b->lgop);
}

void bench_blur_2(struct benchmark_param *b) {
  VID(blur)(vid->display,0,0,b->w,b->h,2);
}

void bench_blur_16(struct benchmark_param *b) {
  VID(blur)(vid->display,0,0,b->w,b->h,16);
}

void bench_charblit_0(struct benchmark_param *b) {
  struct quad clip;
  clip.x1 = 0;
  clip.y1 = 0;
  clip.x2 = vid->lxres-1;
  clip.y2 = vid->lyres-1;
  VID(charblit)(vid->display,benchmark_char,0,0,b->w,b->h,
		0,0,0,&clip,b->lgop,b->w>>3);
}

void bench_charblit_90(struct benchmark_param *b) {
  struct quad clip;
  clip.x1 = 0;
  clip.y1 = 0;
  clip.x2 = vid->lxres-1;
  clip.y2 = vid->lyres-1;
  VID(charblit)(vid->display,benchmark_char,0,vid->lyres-1,b->w,b->h,
		0,90,0,&clip,b->lgop,b->w>>3);
}

void bench_charblit_180(struct benchmark_param *b) {
  struct quad clip;
  clip.x1 = 0;
  clip.y1 = 0;
  clip.x2 = vid->lxres-1;
  clip.y2 = vid->lyres-1;
  VID(charblit)(vid->display,benchmark_char,vid->lxres-1,vid->lyres-1,b->w,b->h,
		0,180,0,&clip,b->lgop,b->w>>3);
}

void bench_charblit_270(struct benchmark_param *b) {
  struct quad clip;
  clip.x1 = 0;
  clip.y1 = 0;
  clip.x2 = vid->lxres-1;
  clip.y2 = vid->lyres-1;
  VID(charblit)(vid->display,benchmark_char,vid->lxres-1,0,b->w,b->h,
		0,270,0,&clip,b->lgop,b->w>>3);
}

void bench_alpha_charblit_0(struct benchmark_param *b) {
  struct quad clip;
  clip.x1 = 0;
  clip.y1 = 0;
  clip.x2 = vid->lxres-1;
  clip.y2 = vid->lyres-1;
  VID(alpha_charblit)(vid->display,benchmark_char,0,0,b->w,b->h,
		128,benchmark_gamma,0,0,&clip,b->lgop);
}

void bench_alpha_charblit_90(struct benchmark_param *b) {
  struct quad clip;
  clip.x1 = 0;
  clip.y1 = 0;
  clip.x2 = vid->lxres-1;
  clip.y2 = vid->lyres-1;
  VID(alpha_charblit)(vid->display,benchmark_char,0,vid->lyres-1,b->w,b->h,
		128,benchmark_gamma,90,0,&clip,b->lgop);
}

void bench_alpha_charblit_180(struct benchmark_param *b) {
  struct quad clip;
  clip.x1 = 0;
  clip.y1 = 0;
  clip.x2 = vid->lxres-1;
  clip.y2 = vid->lyres-1;
  VID(alpha_charblit)(vid->display,benchmark_char,vid->lxres-1,vid->lyres-1,b->w,b->h,
		128,benchmark_gamma,180,0,&clip,b->lgop);
}

void bench_alpha_charblit_270(struct benchmark_param *b) {
  struct quad clip;
  clip.x1 = 0;
  clip.y1 = 0;
  clip.x2 = vid->lxres-1;
  clip.y2 = vid->lyres-1;
  VID(alpha_charblit)(vid->display,benchmark_char,vid->lxres-1,0,b->w,b->h,
		128,benchmark_gamma,270,0,&clip,b->lgop);
}


/************************************************************ Frontend */

/* Table of tests to run */
struct benchmark_test {
  void (*func)(struct benchmark_param *b);
  int needs_lgop, needs_size;
  const char *name;
} benchmark_tests[] = {  
  { &bench_noop,                0,0, "nop"                          },
  { &bench_color_pgtohwr,       0,0, "color pgtohwr"                },
  { &bench_color_hwrtopg,       0,0, "color hwrtopg"                },
  { &bench_update,              0,0, "update"                       },
  { &bench_pixel,               1,0, "pixel"                        },
  { &bench_getpixel,            0,0, "getpixel"                     },
  { &bench_slab,                1,1, "slab"                         },
  { &bench_bar,                 1,1, "bar"                          },
  { &bench_line,                1,1, "line"                         },
  { &bench_rect,                1,1, "rect"                         },
  { &bench_gradient,            1,1, "gradient"                     },
  { &bench_blit,                1,1, "blit"                         },
  { &bench_scrollblit_up,       1,1, "scrollblit up"                },
  { &bench_scrollblit_down,     1,1, "scrollblit down"              },
  { &bench_scrollblit_left,     1,1, "scrollblit left"              },
  { &bench_scrollblit_right,    1,1, "scrollblit right"             },
  { &bench_multiblit_1_1,       1,1, "multiblit (1x1 tile)"         },
  { &bench_multiblit_1_128,     1,1, "multiblit (1x128 tile)"       },
  { &bench_multiblit_128_1,     1,1, "multiblit (128x1 tile)"       },
  { &bench_multiblit_128_128,   1,1, "multiblit (128x128 tile)"     },
  { &bench_rotateblit_0,        1,1, "rotateblit (0 degrees)"       },
  { &bench_rotateblit_90,       1,1, "rotateblit (90 degrees)"      },
  { &bench_rotateblit_180,      1,1, "rotateblit (180 degrees)"     },
  { &bench_rotateblit_270,      1,1, "rotateblit (270 degrees)"     },
  { &bench_ellipse,             1,1, "ellipse"                      },
  { &bench_fellipse,            1,1, "fellipse"                     },
  { &bench_polygon_star,        1,0, "polygon (star)"               },
  { &bench_blur_2,              1,1, "blur (radius 2)"              },
  { &bench_blur_16,             1,1, "blur (radius 16)"             },
  { &bench_charblit_0,          1,1, "charblit (0 degrees)"         },
  { &bench_charblit_90,         1,1, "charblit (90 degrees)"        },
  { &bench_charblit_180,        1,1, "charblit (180 degrees)"       },
  { &bench_charblit_270,        1,1, "charblit (270 degrees)"       },
  { &bench_alpha_charblit_0,    1,1, "alpha charblit (0 degrees)"   },
  { &bench_alpha_charblit_90,   1,1, "alpha charblit (90 degrees)"  },
  { &bench_alpha_charblit_180,  1,1, "alpha charblit (180 degrees)" },
  { &bench_alpha_charblit_270,  1,1, "alpha charblit (270 degrees)" },

  { NULL,0,0,NULL }
};

/* Table of LGOP names */
const char *lgop_names[] = {
  /*  0 */  "null",
  /*  1 */  "none",
  /*  2 */  "or",
  /*  3 */  "and",
  /*  4 */  "xor",
  /*  5 */  "invert",
  /*  6 */  "inverted or",
  /*  7 */  "inverted and",
  /*  8 */  "inverted xor",
  /*  9 */  "add",
  /* 10 */  "subtract",
  /* 11 */  "multiply",
  /* 12 */  "stipple",
  /* 13 */  "alpha",
};

/* Time one test with one set of parameters, print the results */
void benchmark_run_one(struct benchmark_test *test, struct benchmark_param *b) {
  struct timeval start, end;
  int i, result;

  gettimeofday(&start,NULL);
  for (i=0;i<1000;i++)
    test->func(b);
  gettimeofday(&end,NULL);
  VID(update)(0,0,vid->lxres,vid->lyres);

  result = (end.tv_sec - start.tv_sec)*1000000 + (end.tv_usec - start.tv_usec);

  printf("%12d ns  %-30s %-15s %dx%d\n",
	 result, test->name, lgop_names[b->lgop], b->w,b->h);
}

/* Run a benchmark at multiple sizes if necessary */
void benchmark_run_sizes(struct benchmark_test *test, struct benchmark_param *b) {
  if (test->needs_size && get_param_str("benchmark","use_sizes",0)) {
    b->w = b->h = 4;
    benchmark_run_one(test,b);
  }

  b->w = b->h = 128;
  benchmark_run_one(test,b);
}

/* Run a benchmark at multiple lgops if necessary */
void benchmark_run_lgops(struct benchmark_test *test, struct benchmark_param *b) {
  if (test->needs_lgop && get_param_str("benchmark","use_lgops",0)) {
    for (b->lgop=0;b->lgop<=PG_LGOPMAX;b->lgop++)
      benchmark_run_sizes(test,b);
  }
  else {
    b->lgop = PG_LGOP_NONE;
    benchmark_run_sizes(test,b);
  }
}

/* Run all benchmark tests */
void videotest_benchmark(void) {
  struct benchmark_test *t;
  struct benchmark_param b;
  const char *testname = get_param_str("benchmark","name","");
  
  if (vid->lxres < 128 || vid->lyres < 128) {
    printf("The benchmarks require a video mode of at least 128x128!\n");
    return;
  }

  printf("\nRunning video driver benchmarks...\n\n"
	 "Time             Name                           LGOP            Size   \n"
	 "-----------------------------------------------------------------------\n");

  VID(rect)(vid->display, 0,0,vid->lxres,vid->lyres,
	    VID(color_pgtohwr)(0x606060), PG_LGOP_NONE);
  VID(rect)(vid->display, 0,0,vid->lxres,vid->lyres,
	    VID(color_pgtohwr)(0xA0A0A0), PG_LGOP_STIPPLE);

  /* Create a source bitmap needed by some of the tests */
  vid->bitmap_new(&benchmark_srcbit,128,128,vid->bpp);
  benchmark_char = malloc(128*128);
  benchmark_gamma = malloc(256);
  memset(benchmark_char,0xFF,128*128);
  memset(benchmark_gamma,0,256);

  for (t=benchmark_tests; t->func; t++) {
    if (!strstr(t->name,testname))
      continue;
    benchmark_run_lgops(t, &b);
  }

  vid->bitmap_free(benchmark_srcbit);
  free(benchmark_char);
  free(benchmark_gamma);
  printf("\nDone.\n");
}
      

/* The End */
