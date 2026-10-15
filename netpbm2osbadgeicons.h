/*
 * netpbm2osbadgeicons - Convert PPM & PGM files into PowerMac <OS-BADGE-ICONS> bootinfo data
 * Copyright (C) 2026  Cosima Neidahl

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef NETPBM2OSBADGEICONS_H
#define NETPBM2OSBADGEICONS_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <math.h>
#include <netpbm/pam.h>

#define _LOG(...) \
	{ \
		fflush (stdout); \
		fprintf (stderr, __VA_ARGS__); \
		fprintf (stderr, "\n"); \
	}

#ifdef NDEBUG
#	define DEBUG(...) ((void) 0)
#else
#	define DEBUG(...) _LOG ("DEBUG: " __VA_ARGS__);
#endif

#define WARN(...) _LOG ("WARNING: " __VA_ARGS__);

#define ERROR(...) \
	{ \
		_LOG ("ERROR: " __VA_ARGS__); \
		goto fail; \
	}

#define PROGNAME "netpbm2osbadgeicons"

enum {
	RASTER_COUNT = 3,
	STEPS_LOWPRECISION = 6,
	STEPS_HIGHPRECISION = 10
};

struct intensityTuple {
	double r;
	double g;
	double b;
};

struct colorTuple {
	uint8_t r;
	uint8_t g;
	uint8_t b;
};

enum paletteOption {
	LOW_PRECISION_RGB,
	HIGH_PRECISION_R,
	HIGH_PRECISION_G,
	HIGH_PRECISION_B,
	HIGH_PRECISION_BW_R,
	HIGH_PRECISION_BW_G,
	HIGH_PRECISION_BW_B,
	END_OF_OPTIONS
};

#endif /* NETPBM2OSBADGEICONS_H */
