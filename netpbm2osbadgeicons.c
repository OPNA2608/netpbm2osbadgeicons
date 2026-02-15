#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define RASTER_COUNT 3

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
	uint8_t r = round (5 * rIntensity);
	uint8_t g = round (5 * gIntensity);
	uint8_t b = round (5 * bIntensity);

	fprintf (stderr,
		"Intensity: "
		"%lf|"
		"%lf|"
		"%lf\n",
		rIntensity, gIntensity, bIntensity
	);
	fprintf (stderr,
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

	fprintf (stderr, "Opening Netpbm file: %s\n", path);
	tempFile->file = fopen (path, "rb");
	if (tempFile->file == NULL) {
		fprintf (stderr, "Failed to open Netpbm file: %s\n", path);
		goto fail;
	}
	fprintf (stderr, "Netpbm file opened: %s\n", path);

	fprintf (
		stderr,
		"Checking %s for magic: %s%s%s\n",
		path,
		ppmAllowed ? binaryPpmMagic : binaryPgmMagic,
		ppmAllowed ? " " : "",
		ppmAllowed ? binaryPgmMagic : ""
	);
	if (fread (buffer, sizeof (char), 2, tempFile->file) != 2) {
		fprintf (stderr, "Failed to read magic bytes from: %s\n", path);
		goto fail;
	}
	if (ppmAllowed && (memcmp (buffer, binaryPpmMagic, 2) == 0)) {
		fprintf (stderr, "Identified: binary PPM\n");
		tempFile->type = PPM;
	} else if (memcmp (buffer, binaryPgmMagic, 2) == 0) {
		fprintf (stderr, "Identified: binary PGM\n");
		tempFile->type = PGM;
	} else {
		fprintf (stderr, "Illegal file magic: %c%c\n", buffer[0], buffer[1]);
		goto fail;
	}

	fprintf (stderr, "DUMMY: openNetpbmFile\n");
	goto fail;

	/* TODO: Read & set dimensions and depth */
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

bool checkImageDimensions (
	struct netpbmFile* primaryFile,
	struct netpbmFile* secondaryFile,
	struct netpbmFile* alphaFile
) {
	fprintf (stderr, "DUMMY: checkImageDimensions\n");
	return false;
}

bool convertToPalettedRaster (struct netpbmFile* file) {
	fprintf (stderr, "DUMMY: convertToPalettedRaster\n");
	return false;
}

void dummyPalettedRaster (struct netpbmFile* primary) {
	fprintf (stderr, "DUMMY: dummyPalettedRaster\n");
}

bool convertToAlphaMask (struct netpbmFile* file) {
	fprintf (stderr, "DUMMY: convertToAlphaMask\n");
	return false;
}

void dummyAlphaMask (struct netpbmFile* primary) {
	fprintf (stderr, "DUMMY: dummyAlphaMask\n");
}

int main (int argc, char** argv) {
	int ret = 0;
	struct netpbmFile* inputFiles[RASTER_COUNT];
	//char buffer[8];
	//unsigned int width, height, depth;
	//uint8_t x, y, r, g, b, paletteEntry;
	//long startPpmPixelData;

	inputFiles[0] = NULL;
	inputFiles[1] = NULL;
	inputFiles[2] = NULL;

	if (argc < 2 || argc > 4) {
		printf ("Usage: primary.<ppm|pgm> [secondary.<ppm|pgm>] [alphamask.pgm]\n");
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

	if (!checkImageDimensions (inputFiles[0], inputFiles[1], inputFiles[2])) goto fail;

	if (!convertToPalettedRaster (inputFiles[0])) goto fail;

	if (inputFiles[1] != NULL) {
		if (!convertToPalettedRaster (inputFiles[1])) goto fail;
	} else {
		dummyPalettedRaster (inputFiles[0]);
	}

	if (inputFiles[2] != NULL) {
		if (!convertToAlphaMask (inputFiles[2])) goto fail;
	} else {
		dummyAlphaMask (inputFiles[0]);
	}

/*

	fprintf (stderr, "Reading image width.\n");
	if (!readUntilNextElement (ppmFile)) goto fail;
	if (fscanf (ppmFile, "%u", &width) != 1) goto fail;
	fprintf (stderr, "Width: %u\n", width);

	fprintf (stderr, "Reading image height.\n");
	if (!readUntilNextElement (ppmFile)) goto fail;
	if (fscanf (ppmFile, "%u", &height) != 1) goto fail;
	fprintf (stderr, "Height: %u\n", height);

	fprintf (stderr, "Reading colour depth.\n");
	if (!readUntilNextElement (ppmFile)) goto fail;
	if (fscanf (ppmFile, "%u", &depth) != 1) goto fail;
	fprintf (stderr, "Depth: %u\n", depth);
	if (depth > 0xFF) {
		fprintf (stderr, "Error: Not handling bit depth >= 16.\n");
		goto fail;
	}

	if (!readUntilNextElement (ppmFile)) goto fail;
	startPpmPixelData = ftell (ppmFile);

	printf ("%02X%02X\n", width, height);

	// Grid 1: Normal icon
	for (y = 0; y < height; ++y) {
		for (x = 0; x < width; ++x) {
			if (fread (&r, sizeof (uint8_t), 1, ppmFile) != 1) goto fail;
			if (fread (&g, sizeof (uint8_t), 1, ppmFile) != 1) goto fail;
			if (fread (&b, sizeof (uint8_t), 1, ppmFile) != 1) goto fail;

			fprintf (stderr,
				"Original RGB: "
				"%03u|"
				"%03u|"
				"%03u\n",
				r, g, b
			);

			paletteEntry = getClosestColourValue (
				((double)r) / depth,
				((double)g) / depth,
				((double)b) / depth
			);
			printf ("%02X", paletteEntry);
		}
		printf ("\n");
	}

	printf ("\n");

	// Grid 2: ?
	fseek (ppmFile, startPpmPixelData, SEEK_SET);
	for (y = 0; y < height; ++y) {
		for (x = 0; x < width; ++x) {
			printf ("%02X", 255);
		}
		printf ("\n");
	}

	printf ("\n");

	// Grid 3: Alpha mask
	for (y = 0; y < height; ++y) {
		for (x = 0; x < width; ++x) {
			printf ("%02X", 255);
		}
		printf ("\n");
	}
*/

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
