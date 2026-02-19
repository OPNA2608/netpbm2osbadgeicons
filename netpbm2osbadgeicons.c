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

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <netpbm/pam.h>

#ifdef NDEBUG
#	define DEBUG(...) ((void)0)
#else
#	define DEBUG(...) fprintf (stderr, __VA_ARGS__)
#endif

#define ERROR(...) {\
	fflush (stdout);\
	fprintf (stderr, __VA_ARGS__);\
	goto fail;\
}

#define PROGNAME "netpbm2osbadgeicons"
#define RASTER_COUNT 3
#define DEPTH_LIMIT 0xFF

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
uint8_t getClosestColourValue (double rIntensity, double gIntensity, double bIntensity) {
	uint8_t r = (uint8_t) (round (5 * rIntensity));
	uint8_t g = (uint8_t) (round (5 * gIntensity));
	uint8_t b = (uint8_t) (round (5 * bIntensity));

	DEBUG (
		"Intensity: "
		"%lf|"
		"%lf|"
		"%lf\n",
		rIntensity, gIntensity, bIntensity
	);
	DEBUG (
		"Newval: "
		"%u|"
		"%u|"
		"%u\n",
		r, g, b
	);

	if ((r + g + b) == 0) {
		// entry stolen by high-precision R palette, point at other index instead
		return 0xFF;
	} else {
		return
			((6 * 6 * 6) - 1) // lowest-intensity value (except see above)
			- (
				b
				+ (g * 6)
				+ (r * 6 * 6)
			);
	}
}

bool readUntilNextElement (FILE* ppmFile) {
	char buffer = '\0';
	bool inComment = false;

	while (true) {
		if (fread (&buffer, sizeof (char), 1, ppmFile) != 1) return false;
		switch (buffer) {
			case ' ':
			case '\t':
				break;

			case '\r':
			case '\n':
				if (inComment) {
					inComment = false;
				}
				break;

			case '#':
				inComment = true;
				break;

			default:
				if (!inComment) {
					fseek (ppmFile, -1, SEEK_CUR);
					return true;
				}
		}
	}
}

bool openNetpbmFile (struct pam** outPamLocation, char* path, bool colourAllowed) {
	bool ret = true;
	FILE* fileHandle = NULL;
	struct pam* pamHandle = NULL;

	if (*outPamLocation != NULL)
		ERROR ("Passed return location for Netpbm handle is non-NULL. Forgot to de-init & free?\n");

	pamHandle = malloc (sizeof (struct pam));
	if (pamHandle == NULL)
		ERROR ("Failed to allocate memory for PAM struct\n");

	DEBUG ("Opening Netpbm file: %s\n", path);
	fileHandle = fopen (path, "rb");
	if (fileHandle == NULL)
		ERROR ("Failed to open Netpbm file: %s\n", path);
	DEBUG ("Netpbm file opened: %s\n", path);

	pamHandle->file = NULL;
	pamHandle->allocation_depth = 0;
	pamHandle->comment_p = NULL;

	// TODO: This aborts if an error is found. I think that's kinda not nice. Catch signal & handle more gracefully?
	pnm_readpaminit (fileHandle, pamHandle, PAM_STRUCT_SIZE(tuple_type));
	DEBUG ("Netpbm file header parsed.\n");

	DEBUG ("Width: %u\n", pamHandle->width);
	DEBUG ("Height: %u\n", pamHandle->height);
	DEBUG ("Depth: %lu\n", pamHandle->maxval);

	if (!colourAllowed) {
		if (!(
			pamHandle->format == PBM_FORMAT || pamHandle->format == RPBM_FORMAT
			|| pamHandle->format == PGM_FORMAT || pamHandle->format == RPGM_FORMAT
		))
			ERROR ("Netpbm file for this raster must be B/W only (PBM, PGM)\n");
	}

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

bool checkImageParameters (
	struct pam* primaryFile,
	struct pam* secondaryFile,
	struct pam* alphaFile
) {
	bool ret = true;

	if (primaryFile->width > 52)
		ERROR ("Primary image has invalid width (>52): %u\n", primaryFile->width);

	if (primaryFile->height > 52)
		ERROR ("Primary image has invalid height (>52): %u\n", primaryFile->height);

	if (primaryFile->maxval > DEPTH_LIMIT)
		ERROR (
			"TODO: Primary image's depth (%lu) larger than what we can currently handle (%u)\n",
			primaryFile->maxval,
			DEPTH_LIMIT
		);

	if (secondaryFile != NULL) {
		if (secondaryFile->width != primaryFile->width)
			ERROR (
				"Secondary image's width (%u) doesn't match primary image's width (%u)\n",
				secondaryFile->width,
				primaryFile->width
			);

		if (secondaryFile->height != primaryFile->height)
			ERROR (
				"Secondary image's height (%u) doesn't match primary image's height (%u)\n",
				secondaryFile->height,
				primaryFile->height
			);

		if (secondaryFile->maxval > 0xFF)
			ERROR (
				"TODO: Secondary image's depth (%lu) larger than what we can currently handle (%u)\n",
				secondaryFile->maxval,
				DEPTH_LIMIT
			);
	}

	if (alphaFile != NULL) {
		if (alphaFile->width != primaryFile->width)
			ERROR (
				"Alpha mask's width (%u) doesn't match primary image's width (%u)\n",
				alphaFile->width,
				primaryFile->width
			);

		if (alphaFile->height != primaryFile->height)
			ERROR (
				"Alpha mask's height (%u) doesn't match primary image's height (%u)\n",
				alphaFile->height,
				primaryFile->height
			);

		if (alphaFile->maxval > 0xFF)
			ERROR (
				"TODO: Alpha mask's depth (%lu) larger than what we can currently handle (%u)\n",
				secondaryFile->maxval,
				DEPTH_LIMIT
			);
	}

	DEBUG ("Checked headers of Netpbm files.\n");
	goto end;

fail:
	ret = false;

end:
	return ret;
}

bool convertToPalettedRaster (struct pam* file) {
	bool ret = true;
	int x, y;
	uint8_t r, g, b, paletteEntry;

	if (!readUntilNextElement (file->file)) goto fail;

	for (y = 0; y < file->height; ++y) {
		for (x = 0; x < file->width; ++x) {
			if (fread (&r, sizeof (uint8_t), 1, file->file) != 1) goto fail;

			if (file->format == RPPM_FORMAT) {
				if (fread (&g, sizeof (uint8_t), 1, file->file) != 1) goto fail;
			} else {
				g = r;
			}

			if (file->format == RPPM_FORMAT) {
				if (fread (&b, sizeof (uint8_t), 1, file->file) != 1) goto fail;
			} else {
				b = r;
			}

			DEBUG (
				"Original RGB: "
				"%03u|"
				"%03u|"
				"%03u\n",
				r, g, b
			);

			paletteEntry = getClosestColourValue (
				((double)r) / file->maxval,
				((double)g) / file->maxval,
				((double)b) / file->maxval
			);
			printf ("%02X", paletteEntry);
		}
		printf ("\n");
	}

	goto end;

fail:
	ret = false;

end:
	return ret;
}

void dummyPalettedRaster (struct pam* primary) {
	int x, y;

	for (y = 0; y < primary->height; ++y) {
		for (x = 0; x < primary->width; ++x) {
			printf ("%02X", 255);
		}
		printf ("\n");
	}
}

bool convertToAlphaMask (struct pam* file) {
	bool ret = true;
	int x, y;
	uint8_t a;

	if (!readUntilNextElement (file->file)) goto fail;

	for (y = 0; y < file->height; ++y) {
		for (x = 0; x < file->width; ++x) {
			if (fread (&a, sizeof (uint8_t), 1, file->file) != 1) goto fail;

			DEBUG (
				"Original alpha: "
				"%03u\n",
				a
			);

			printf ("%02X", a);
		}
		printf ("\n");
	}

	goto end;

fail:
	ret = false;

end:
	return ret;
}

void dummyAlphaMask (struct pam* primary) {
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

	if (!openNetpbmFile (&inputFiles[0], argv[1], true)) goto fail;

	if (
		(argc > 2)
		&& (strcmp (argv[2], "none") != 0)
		&& !openNetpbmFile (&inputFiles[1], argv[2], true)
	) goto fail;

	if (
		(argc > 3)
		&& (strcmp (argv[3], "none") != 0)
		&& !openNetpbmFile (&inputFiles[2], argv[3], false)
	) goto fail;

	if (!checkImageParameters (inputFiles[0], inputFiles[1], inputFiles[2])) goto fail;

	// <width><height>, printed in hex
	printf ("%02X%02X\n", inputFiles[0]->width, inputFiles[0]->height);

	// Grid 1: Normal icon
	if (!convertToPalettedRaster (inputFiles[0])) goto fail;

	printf ("\n");

	// Grid 2: ?
	if (inputFiles[1] != NULL) {
		if (!convertToPalettedRaster (inputFiles[1])) goto fail;
	} else {
		dummyPalettedRaster (inputFiles[0]);
	}

	printf ("\n");

	// Grid 3: Alpha mask
	if (inputFiles[2] != NULL) {
		if (!convertToAlphaMask (inputFiles[2])) goto fail;
	} else {
		dummyAlphaMask (inputFiles[0]);
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
			inputFiles[i] = NULL;
		}
	}

	return ret;
}
