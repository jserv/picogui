/*
 * transform.h
 * Copyright (C) 1999 Bradley D. LaRonde <brad@ltc.com>
 *
 * This program is free software; you may redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

typedef struct  tagPOINT {
    int x;
    int y;
} POINT;

#define TRANSFORMATION_UNITS_PER_PIXEL 4

typedef struct
{
        /*
         * Coefficients for the transformation formulas:
         *
         *     m = (ax + by + c) / s
         *     n = (dx + ey + f) / s
         *
         * These formulas will transform a device point (x, y) to a
         * screen point (m, n) in fractional pixels.  The fraction
         * is 1 / TRANSFORMATION_UNITS_PER_PIXEL.
         */

        int a, b, c, d, e, f, s;
} TRANSFORMATION_COEFFICIENTS;


typedef struct
{
	POINT screen, device;
} CALIBRATION_PAIR;

typedef struct
{
	CALIBRATION_PAIR center;
	CALIBRATION_PAIR ul;
	CALIBRATION_PAIR ur;
	CALIBRATION_PAIR lr;
	CALIBRATION_PAIR ll;
} CALIBRATION_PAIRS;

POINT pentoscreen(POINT pt, TRANSFORMATION_COEFFICIENTS *ptc);
int CalcTransformationCoefficientsSimple(CALIBRATION_PAIRS *pcp, TRANSFORMATION_COEFFICIENTS *ptc);
int CalcTransformationCoefficientsBetter(CALIBRATION_PAIRS *pcp, TRANSFORMATION_COEFFICIENTS *ptc);
int CalcTransformationCoefficientsEvenBetter(CALIBRATION_PAIRS *pcp, TRANSFORMATION_COEFFICIENTS *ptc);
int CalcTransformationCoefficientsBest(CALIBRATION_PAIR *cp, TRANSFORMATION_COEFFICIENTS *ptc, int points);
