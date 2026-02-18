#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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

/* magic number of binary PPM/PGM */
char* binaryPpmMagic = "P6";
char* binaryPgmMagic = "P5";

enum netpbmType {
	PPM,
	PGM,
};

struct netpbmFile {
	FILE* file;
	enum netpbmType type;
	unsigned int width;
	unsigned int height;
	unsigned int depth;
};

bool openNetpbmFile (struct netpbmFile** outFile, char* path, bool ppmAllowed) {
	bool ret = true;
	struct netpbmFile* tempFile;
	char buffer[8];

	tempFile = malloc (sizeof (struct netpbmFile));
	if (tempFile == NULL) goto fail;

	DEBUG ("Opening Netpbm file: %s\n", path);
	tempFile->file = fopen (path, "rb");
	if (tempFile->file == NULL)
		ERROR ("Failed to open Netpbm file: %s\n", path);
	DEBUG ("Netpbm file opened: %s\n", path);

	DEBUG (
		"Checking %s for magic: %s%s%s\n",
		path,
		ppmAllowed ? binaryPpmMagic : binaryPgmMagic,
		ppmAllowed ? " " : "",
		ppmAllowed ? binaryPgmMagic : ""
	);
	if (fread (buffer, sizeof (char), 2, tempFile->file) != 2)
		ERROR ("Failed to read magic bytes from: %s\n", path);
	if (ppmAllowed && (memcmp (buffer, binaryPpmMagic, 2) == 0)) {
		DEBUG ("Identified: binary PPM\n");
		tempFile->type = PPM;
	} else if (memcmp (buffer, binaryPgmMagic, 2) == 0) {
		DEBUG ("Identified: binary PGM\n");
		tempFile->type = PGM;
	} else
		ERROR ("Illegal file magic: %c%c\n", buffer[0], buffer[1]);

	DEBUG ("Reading image width.\n");
	if (!readUntilNextElement (tempFile->file)) goto fail;
	if (fscanf (tempFile->file, "%u", &tempFile->width) != 1)
		ERROR ("Failed to read image width.\n");
	DEBUG ("Width: %u\n", tempFile->width);

	DEBUG ("Reading image height.\n");
	if (!readUntilNextElement (tempFile->file)) goto fail;
	if (fscanf (tempFile->file, "%u", &tempFile->height) != 1)
		ERROR ("Failed to read image height.\n");
	DEBUG ("Height: %u\n", tempFile->height);

	DEBUG ("Reading image depth.\n");
	if (!readUntilNextElement (tempFile->file)) goto fail;
	if (fscanf (tempFile->file, "%u", &tempFile->depth) != 1)
		ERROR ("Failed to read image depth.\n");
	DEBUG ("Depth: %u\n", tempFile->depth);

	*outFile = tempFile;
	goto end;

fail:
	ret = false;
	if (tempFile != NULL) {
		if (tempFile->file != NULL) {
			fclose (tempFile->file);
			tempFile->file = NULL;
		}
		*outFile = NULL;
		free (tempFile);
	}

end:
	return ret;
}

bool checkImageParameters (
	struct netpbmFile* primaryFile,
	struct netpbmFile* secondaryFile,
	struct netpbmFile* alphaFile
) {
	bool ret = true;

	if (primaryFile->width > 52)
		ERROR ("Primary image has invalid width (>52): %u\n", primaryFile->width);

	if (primaryFile->height > 52)
		ERROR ("Primary image has invalid height (>52): %u\n", primaryFile->height);

	if (primaryFile->depth > DEPTH_LIMIT)
		ERROR (
			"TODO: Primary image's depth (%u) larger than what we can currently handle (%u)\n",
			primaryFile->depth,
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

		if (secondaryFile->depth > 0xFF)
			ERROR (
				"TODO: Secondary image's depth (%u) larger than what we can currently handle (%u)\n",
				secondaryFile->depth,
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

		if (alphaFile->depth > 0xFF)
			ERROR (
				"TODO: Alpha mask's depth (%u) larger than what we can currently handle (%u)\n",
				secondaryFile->depth,
				DEPTH_LIMIT
			);
	}

	goto end;

fail:
	ret = false;

end:
	return ret;
}

bool convertToPalettedRaster (struct netpbmFile* file) {
	bool ret = true;
	unsigned int x, y;
	uint8_t r, g, b, paletteEntry;

	if (!readUntilNextElement (file->file)) goto fail;

	for (y = 0; y < file->height; ++y) {
		for (x = 0; x < file->width; ++x) {
			if (fread (&r, sizeof (uint8_t), 1, file->file) != 1) goto fail;

			if (file->type == PPM) {
				if (fread (&g, sizeof (uint8_t), 1, file->file) != 1) goto fail;
			} else {
				g = r;
			}

			if (file->type == PPM) {
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
				((double)r) / file->depth,
				((double)g) / file->depth,
				((double)b) / file->depth
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

void dummyPalettedRaster (struct netpbmFile* primary) {
	unsigned int x, y;

	for (y = 0; y < primary->height; ++y) {
		for (x = 0; x < primary->width; ++x) {
			printf ("%02X", 255);
		}
		printf ("\n");
	}
}

bool convertToAlphaMask (struct netpbmFile* file) {
	bool ret = true;
	unsigned int x, y;
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

void dummyAlphaMask (struct netpbmFile* primary) {
	unsigned int x, y;

	for (y = 0; y < primary->height; ++y) {
		for (x = 0; x < primary->width; ++x) {
			printf ("%02X", 255);
		}
		printf ("\n");
	}
}

int main (int argc, char** argv) {
	int ret = 0;
	struct netpbmFile* inputFiles[RASTER_COUNT];

	inputFiles[0] = NULL;
	inputFiles[1] = NULL;
	inputFiles[2] = NULL;

	if (argc < 2 || argc > 4) {
		printf (
			"Usage: %s primary.<ppm|pgm> [<secondary.<ppm|pgm>|\"none\">] [<alphamask.pgm|\"none\">]\n",
			(argc > 0) ? argv[0] : "netpbm2osbadgeicons"
		);
		goto end;
	}

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
