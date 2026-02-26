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
#define RASTER_COUNT 3
#define DEPTH_LIMIT 0xFF

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

double distanceFromOriginalColour (
	struct colorTuple calculatedColour,
	unsigned int newIntensityMax,
	struct intensityTuple originalIntensities
) {
	return fabs (((double) calculatedColour.r / newIntensityMax) - originalIntensities.r)
		+ fabs (((double) calculatedColour.g / newIntensityMax) - originalIntensities.g)
		+ fabs (((double) calculatedColour.b / newIntensityMax) - originalIntensities.b);
}

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

enum paletteOption findPaletteOptionWithLeastDiff (double options[END_OF_OPTIONS]) {
	enum paletteOption lowestOption = END_OF_OPTIONS;
	double lowestDiff = 100.;
	unsigned int i;

	for (i = LOW_PRECISION_RGB; i < END_OF_OPTIONS; ++i) {
		if (options[i] < lowestDiff) {
			lowestOption = i;
			lowestDiff = options[i];
		}
	}

	return lowestOption;
}

/*
	palette consists of:

	1. RGB mixes
	2. 10-step pure R values
	3. 10-step pure G values
	4. 10-step pure B values
	5. 10-step B/W
	6. an extra pure black

	Try to find the palette entry that's closest to the desired colour.

	TODO: Check if colour may be closer to a high-precision R/G/B/BW entry
*/
uint8_t getClosestColourValue (struct intensityTuple intensities) {
	struct colorTuple lowPrecisionRGB = {
		.r = (uint8_t) (round ((6 - 1) * intensities.r)),
		.g = (uint8_t) (round ((6 - 1) * intensities.g)),
		.b = (uint8_t) (round ((6 - 1) * intensities.b)),
	};

	struct colorTuple highPrecisionR = {
		.r = (uint8_t) (round ((10 - 1) * intensities.r)),
		.g = 0,
		.b = 0,
	};

	struct colorTuple highPrecisionG = {
		.r = 0,
		.g = (uint8_t) (round ((10 - 1) * intensities.g)),
		.b = 0,
	};

	struct colorTuple highPrecisionB = {
		.r = 0,
		.g = 0,
		.b = (uint8_t) (round ((10 - 1) * intensities.b)),
	};

	struct colorTuple highPrecisionR_BW = {
		.r = highPrecisionR.r,
		.g = highPrecisionR.r,
		.b = highPrecisionR.r,
	};

	struct colorTuple highPrecisionG_BW = {
		.r = highPrecisionG.g,
		.g = highPrecisionG.g,
		.b = highPrecisionG.g,
	};

	struct colorTuple highPrecisionB_BW = {
		.r = highPrecisionB.b,
		.g = highPrecisionB.b,
		.b = highPrecisionB.b,
	};

	double lowPrecisionRGBDiff = distanceFromOriginalColour (lowPrecisionRGB, 6 - 1, intensities);
	double highPrecisionRDiff = distanceFromOriginalColour (highPrecisionR, 10 - 1, intensities);
	double highPrecisionGDiff = distanceFromOriginalColour (highPrecisionG, 10 - 1, intensities);
	double highPrecisionBDiff = distanceFromOriginalColour (highPrecisionB, 10 - 1, intensities);
	double highPrecisionR_BWDiff = distanceFromOriginalColour (highPrecisionR_BW, 10 - 1, intensities);
	double highPrecisionG_BWDiff = distanceFromOriginalColour (highPrecisionG_BW, 10 - 1, intensities);
	double highPrecisionB_BWDiff = distanceFromOriginalColour (highPrecisionB_BW, 10 - 1, intensities);

	double paletteOptionDiffs[] = {
		lowPrecisionRGBDiff,
		highPrecisionRDiff,
		highPrecisionGDiff,
		highPrecisionBDiff,
		highPrecisionR_BWDiff,
		highPrecisionG_BWDiff,
		highPrecisionB_BWDiff,
	};

	uint8_t paletteEntry;

	DEBUG (
		"Input intensities: "
		"%lf|"
		"%lf|"
		"%lf",
		intensities.r,
		intensities.g,
		intensities.b
	);

	DEBUG (
		"Low-precision values: "
		"%u|"
		"%u|"
		"%u"
		" (dist %lf)",
		lowPrecisionRGB.r,
		lowPrecisionRGB.g,
		lowPrecisionRGB.b,
		lowPrecisionRGBDiff
	);

	DEBUG (
		"High-precision R values: "
		"%u|"
		"%u|"
		"%u"
		" (dist %lf)",
		highPrecisionR.r,
		highPrecisionR.g,
		highPrecisionR.b,
		highPrecisionRDiff
	);

	DEBUG (
		"High-precision G values: "
		"%u|"
		"%u|"
		"%u"
		" (dist %lf)",
		highPrecisionG.r,
		highPrecisionG.g,
		highPrecisionG.b,
		highPrecisionGDiff
	);

	DEBUG (
		"High-precision B values: "
		"%u|"
		"%u|"
		"%u"
		" (dist %lf)",
		highPrecisionB.r,
		highPrecisionB.g,
		highPrecisionB.b,
		highPrecisionBDiff
	);

	DEBUG (
		"High-precision BW (R) values: "
		"%u|"
		"%u|"
		"%u"
		" (dist %lf)",
		highPrecisionR_BW.r,
		highPrecisionR_BW.g,
		highPrecisionR_BW.b,
		highPrecisionR_BWDiff
	);

	DEBUG (
		"High-precision BW (G) values: "
		"%u|"
		"%u|"
		"%u"
		" (dist %lf)",
		highPrecisionG_BW.r,
		highPrecisionG_BW.g,
		highPrecisionG_BW.b,
		highPrecisionG_BWDiff
	);

	DEBUG (
		"High-precision BW (B) values: "
		"%u|"
		"%u|"
		"%u"
		" (dist %lf)",
		highPrecisionB_BW.r,
		highPrecisionB_BW.g,
		highPrecisionB_BW.b,
		highPrecisionB_BWDiff
	);

	switch (findPaletteOptionWithLeastDiff (paletteOptionDiffs)) {
		case LOW_PRECISION_RGB:
			DEBUG ("Choosing low-precision RGB");
			if ((lowPrecisionRGB.r + lowPrecisionRGB.g + lowPrecisionRGB.b) == 0) {
				// entry stolen by high-precision R palette, point at other index instead
				paletteEntry = 0xFF;
			} else {
				// for clarity on how these values come together to form the index
				// clang-format off
				paletteEntry =
					// lowest-intensity value (except see above)
					((6 * 6 * 6) - 1)
					- (
						// offset into the table
						lowPrecisionRGB.b
						+ (lowPrecisionRGB.g * 6)
						+ (lowPrecisionRGB.r * 6 * 6)
					);
				// clang-format on
			}
			break;

		case HIGH_PRECISION_R:
			DEBUG ("Choosing high-precision R");
			paletteEntry = (((6 * 6 * 6) - 1) + (1 * 10)) - highPrecisionR.r;
			break;

		case HIGH_PRECISION_G:
			DEBUG ("Choosing high-precision G");
			paletteEntry = (((6 * 6 * 6) - 1) + (2 * 10)) - highPrecisionG.g;
			break;

		case HIGH_PRECISION_B:
			DEBUG ("Choosing high-precision B");
			paletteEntry = (((6 * 6 * 6) - 1) + (3 * 10)) - highPrecisionB.b;
			break;

		case HIGH_PRECISION_BW_R:
			DEBUG ("Choosing high-precision BW (based on R value)");
			paletteEntry = (((6 * 6 * 6) - 1) + (4 * 10)) - highPrecisionR_BW.r;
			break;

		case HIGH_PRECISION_BW_G:
			DEBUG ("Choosing high-precision BW (based on G value)");
			paletteEntry = (((6 * 6 * 6) - 1) + (4 * 10)) - highPrecisionG_BW.g;
			break;

		case HIGH_PRECISION_BW_B:
			DEBUG ("Choosing high-precision BW (based on B value)");
			paletteEntry = (((6 * 6 * 6) - 1) + (4 * 10)) - highPrecisionB_BW.b;
			break;

		case END_OF_OPTIONS:
		default:
			// TODO: Prolly error out?
			fprintf (stderr, "??????");
			paletteEntry = 0xFF;
			break;
	}

	return paletteEntry;
}

bool openNetpbmFile (struct pam** outPamLocation, char* path) {
	bool ret = true;
	FILE* fileHandle = NULL;
	struct pam* pamHandle = NULL;

	if (*outPamLocation != NULL)
		ERROR ("Passed return location for Netpbm handle is non-NULL. Forgot to de-init & free?");

	pamHandle = malloc (sizeof (struct pam));
	if (pamHandle == NULL)
		ERROR ("Failed to allocate memory for PAM struct");

	DEBUG ("Opening Netpbm file: %s", path);
	fileHandle = fopen (path, "rb");
	if (fileHandle == NULL)
		ERROR ("Failed to open Netpbm file: %s", path);
	DEBUG ("Netpbm file opened: %s", path);

	pamHandle->file = NULL;
	pamHandle->allocation_depth = 0;
	pamHandle->comment_p = NULL;

	// TODO: This aborts if an error is found. I think that's kinda not nice. Catch signal & handle more gracefully?
	pnm_readpaminit (fileHandle, pamHandle, PAM_STRUCT_SIZE (tuple_type));
	DEBUG ("Netpbm file header parsed.");

	DEBUG ("Width: %u", pamHandle->width);
	DEBUG ("Height: %u", pamHandle->height);
	DEBUG ("Depth: %lu", pamHandle->maxval);

	*outPamLocation = pamHandle;
	goto end;

fail:
	ret = false;

	if (fileHandle != NULL) {
		if (pamHandle->file == fileHandle) {
			pamHandle->file = NULL;
		}
		fclose (fileHandle);
		fileHandle = NULL;
	}

	if (pamHandle != NULL) {
		if (*outPamLocation == pamHandle) {
			*outPamLocation = NULL;
		}
		free (pamHandle);
		pamHandle = NULL;
	}

end:
	return ret;
}

bool checkImageParameters (struct pam* primaryFile, struct pam* secondaryFile, struct pam* alphaFile) {
	bool ret = true;

	if (primaryFile->width > 52)
		ERROR ("Primary image has invalid width (>52): %u", primaryFile->width);

	if (primaryFile->height > 52)
		ERROR ("Primary image has invalid height (>52): %u", primaryFile->height);

	if (primaryFile->maxval > DEPTH_LIMIT)
		ERROR (
			"TODO: Primary image's depth (%lu) larger than what we can currently handle (%u)",
			primaryFile->maxval,
			DEPTH_LIMIT
		);

	if (secondaryFile != NULL) {
		if (secondaryFile->width != primaryFile->width)
			ERROR (
				"Secondary image's width (%u) doesn't match primary image's width (%u)",
				secondaryFile->width,
				primaryFile->width
			);

		if (secondaryFile->height != primaryFile->height)
			ERROR (
				"Secondary image's height (%u) doesn't match primary image's height (%u)",
				secondaryFile->height,
				primaryFile->height
			);

		if (secondaryFile->maxval > 0xFF)
			ERROR (
				"TODO: Secondary image's depth (%lu) larger than what we can currently handle (%u)",
				secondaryFile->maxval,
				DEPTH_LIMIT
			);
	}

	if (alphaFile != NULL) {
		if (alphaFile->width != primaryFile->width)
			ERROR ("Alpha mask's width (%u) doesn't match primary image's width (%u)", alphaFile->width, primaryFile->width);

		if (alphaFile->height != primaryFile->height)
			ERROR (
				"Alpha mask's height (%u) doesn't match primary image's height (%u)",
				alphaFile->height,
				primaryFile->height
			);

		if (alphaFile->maxval > 0xFF)
			ERROR (
				"TODO: Alpha mask's depth (%lu) larger than what we can currently handle (%u)",
				primaryFile->maxval,
				DEPTH_LIMIT
			);

		if (!(alphaFile->format == PBM_FORMAT
					|| alphaFile->format == RPBM_FORMAT
					|| alphaFile->format == PGM_FORMAT
					|| alphaFile->format == RPGM_FORMAT))
			ERROR ("Netpbm file for the alpha mask must be B/W only (PBM, PGM)");

		if (alphaFile->format == PGM_FORMAT || alphaFile->format == RPGM_FORMAT)
			WARN ("Greyscale alpha maps might not get displayed accurately, see project notes");
	}

	DEBUG ("Checked headers of Netpbm files.");
	goto end;

fail:
	ret = false;

end:
	return ret;
}

bool convertToPalettedRaster (struct pam* file) {
	bool ret = true;
	tuple* currentInputRow = NULL;
	int x, y;
	uint8_t r, g, b, paletteEntry;

	// TODO: This allocates, so I assume it can fail. Does it abort, or return NULL?
	currentInputRow = pnm_allocpamrow (file);

	for (y = 0; y < file->height; ++y) {
		// TODO: This aborts if an error is found. I think that's kinda not nice. Catch signal & handle more gracefully?
		pnm_readpamrow (file, currentInputRow);

		for (x = 0; x < file->width; ++x) {
			switch (file->depth) {
				case 1:
					// only BW
					r = currentInputRow[x][0];
					g = currentInputRow[x][0];
					b = currentInputRow[x][0];
					break;

				case 3:
					// RGB
					r = currentInputRow[x][0];
					g = currentInputRow[x][1];
					b = currentInputRow[x][2];
					break;

				default:
					ERROR ("Don't know convert this amount of planes: %i", file->depth);
			}

			DEBUG (
				"Original RGB: "
				"%03u|"
				"%03u|"
				"%03u",
				r,
				g,
				b
			);

			paletteEntry = getClosestColourValue ((struct intensityTuple) {
				.r = ((double) r) / file->maxval,
				.g = ((double) g) / file->maxval,
				.b = ((double) b) / file->maxval,
			});
			printf ("%02X", paletteEntry);
		}
		printf ("\n");
	}

	goto end;

fail:
	ret = false;

end:
	pnm_freepamrow (currentInputRow);

	return ret;
}

void generateBlankPalettedRaster (struct pam* primary) {
	int x, y;

	for (y = 0; y < primary->height; ++y) {
		for (x = 0; x < primary->width; ++x) {
			printf ("%02X", 255);
		}
		printf ("\n");
	}
}

bool convertToAlphaMask (struct pam* file) {
	tuple* currentInputRow = NULL;
	int x, y;
	double aIntensity;
	uint8_t a;

	// TODO: This allocates, so I assume it can fail. Does it abort, or return NULL?
	currentInputRow = pnm_allocpamrow (file);

	for (y = 0; y < file->height; ++y) {
		// TODO: This aborts if an error is found. I think that's kinda not nice. Catch signal & handle more gracefully?
		pnm_readpamrow (file, currentInputRow);

		for (x = 0; x < file->width; ++x) {
			aIntensity = ((double) currentInputRow[x][0]) / file->maxval;

			DEBUG (
				"Original alpha intensity: "
				"%lf",
				aIntensity
			);

			a = round (aIntensity * 0xFF);
			DEBUG ("Output alpha: %03u", a);

			if ((a != 0) && (a != 0xFF)) {
				DEBUG ("Alpha value %03u might get treated as %03u by hardware!", a, 0xFF);
			}

			printf ("%02X", a);
		}
		printf ("\n");
	}

	pnm_freepamrow (currentInputRow);

	return true;
}

void generateBlankAlphaMask (struct pam* primary) {
	int x, y;

	for (y = 0; y < primary->height; ++y) {
		for (x = 0; x < primary->width; ++x) {
			printf ("%02X", 255);
		}
		printf ("\n");
	}
}

int main (int argc, char** argv) {
	int ret = 0;
	unsigned int i;
	struct pam* inputFiles[RASTER_COUNT];

	for (i = 0; i < RASTER_COUNT; ++i)
		inputFiles[i] = NULL;

	if (argc < 2 || argc > 4) {
		printf (
			"Usage: %s primary.<ppm|pgm> [<secondary.<ppm|pgm>|\"none\">] [<alphamask.pgm|\"none\">]\n",
			(argc > 0) ? argv[0] : PROGNAME
		);
		goto end;
	}

	pm_init ((argc > 0) ? argv[0] : PROGNAME, 0);

	if (!openNetpbmFile (&inputFiles[0], argv[1]))
		goto fail;

	if ((argc > 2) && (strcmp (argv[2], "none") != 0) && !openNetpbmFile (&inputFiles[1], argv[2]))
		goto fail;

	if ((argc > 3) && (strcmp (argv[3], "none") != 0) && !openNetpbmFile (&inputFiles[2], argv[3]))
		goto fail;

	if (!checkImageParameters (inputFiles[0], inputFiles[1], inputFiles[2]))
		goto fail;

	// <width><height>, printed in hex
	printf ("%02X%02X\n", inputFiles[0]->width, inputFiles[0]->height);

	// Grid 1: Normal icon
	if (!convertToPalettedRaster (inputFiles[0]))
		goto fail;

	printf ("\n");

	// Grid 2: ?
	if (inputFiles[1] != NULL) {
		if (!convertToPalettedRaster (inputFiles[1]))
			goto fail;
	} else {
		generateBlankPalettedRaster (inputFiles[0]);
	}

	printf ("\n");

	// Grid 3: Alpha mask
	if (inputFiles[2] != NULL) {
		if (!convertToAlphaMask (inputFiles[2]))
			goto fail;
	} else {
		generateBlankAlphaMask (inputFiles[0]);
	}

	goto end;

fail:
	fprintf (stderr, "Error encountered, exiting.\n");
	ret = 1;

end:
	for (int i = 0; i < RASTER_COUNT; ++i) {
		if (inputFiles[i] != NULL) {
			if (inputFiles[i]->file != NULL) {
				fclose (inputFiles[i]->file);
				inputFiles[i]->file = NULL;
			}
			free (inputFiles[i]);
			inputFiles[i] = NULL;
		}
	}

	return ret;
}
