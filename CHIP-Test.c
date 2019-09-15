/******************************************************************************
 * CHIP-Test: Hexagonal Image Processing Framework in C
 ******************************************************************************
 * v1.0 - 01.04.2016
 *
 * Copyright (c) 2016 Tobias Schlosser
 *  (tobias.schlosser@informatik.tu-chemnitz.de)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ******************************************************************************/


// timespec
#if __STDC_VERSION__ >= 199901L
#define _XOPEN_SOURCE 600
#else
#define _XOPEN_SOURCE 500
#endif


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <math.h>

#include "Misc/pArray2d.h"
#include "Misc/Precalcs.h"
#include "Misc/Types.h"

#include "CHIP/Hexarray.h"
#include "CHIP/Hexsamp.h"
#include "HDCT/HDCT.h"
#include "HFBs/HFBs.h"
#include "Quant/Quant.h"

#include <opencv2/imgcodecs/imgcodecs_c.h>


#define CLOCK_DIFF(i) ( \
	runtimes[i] = end.tv_sec  - begin.tv_sec + \
	             (end.tv_nsec - begin.tv_nsec) / 1000000000.0 )


int main(int argc, char** argv) {
	char*        ifname;
	char         ofname[256];

	// Parameter
	char*        title;    // Beschriftung Ausgaben
	unsigned int order;    // Ordnung Hexarray
	unsigned int mode;     // Bilineare | bikubische | Lanczos- | B-Spline-Interpolation (B_3)
	float        radius;   // Radius Rücktransformation
	unsigned int threads;  // Anzahl Pthreads Rücktransformation
	unsigned int DCT_N;    // DCT deaktiviert | Ordnung 1 | Ordnung 2
	unsigned int DCT_mode; // HDCT | DCTH (nur für N = 5) | Logspace-1D-DCTH | 1D-DCTH (3)
	unsigned int quant_qf; // Qualitätsfaktor Quantisierung
	unsigned int filter;   // Blurring- | Unblurring- | Low- | High-pass filters (4)
	unsigned int scale;    // Herunterskalierung nach HIP (2), eigene Herunter-, Hochskalierung


	if(argc < 12) {
		printf(
			"Usage: %s [ [<Pfad>/]<Eingabe-Bilddatei> <title> \\\n"
			"           <order=(1-7)> <mode=(0-3)> <radius=(>0.0f)> <threads=(>0)> \\\n"
			"           <DCT_N=(0|2|5)> <DCT_mode=(0-5)> <quant_qf=(1-99)> \\\n"
			"           <filter=(0-6)> <scale=(0|4)> ]\n",
			 argv[0]);

		ifname   = "Tests/Testset/Lena_cropped.bmp";
		title    = "Lena";
		order    =  5;
		mode     =  1;
		radius   =  1.0f;
		threads  =  1;
		DCT_N    =  5;
		DCT_mode =  3;
		quant_qf = 90;
		filter   =  0;
		scale    =  0;
	} else {
		ifname   =                    argv[1];
		title    =                    argv[2];
		order    = (unsigned int)atoi(argv[3]);
		mode     = (unsigned int)atoi(argv[4]);
		radius   =        (float)atof(argv[5]);
		threads  = (unsigned int)atoi(argv[6]);
		DCT_N    = (unsigned int)atoi(argv[7]);
		DCT_mode = (unsigned int)atoi(argv[8]);
		quant_qf = (unsigned int)atoi(argv[9]);
		filter   = (unsigned int)atoi(argv[10]);
		scale    = (unsigned int)atoi(argv[11]);
	}




	// Initialisierungen


	IplImage*          img    = cvLoadImage(ifname, CV_LOAD_IMAGE_COLOR);
	const unsigned int width  = (unsigned int)img->width;
	const unsigned int height = (unsigned int)img->height;
	RGB_Array          rgb_array;
	RGB_Hexarray       rgb_hexarray;

	double runtimes[5] = { 0.0, 0.0, 0.0, 0.0, 0.0 };
	struct timespec begin, end;


	printf(
		"\n\"%s\" (%u x %u): \"%s\"\n"
		" -> order = %u, mode = %u, radius = %.2f, threads = %u\n"
		" -> DCT_N = %u, DCT_mode = %u, quant_qf = %u\n"
		" -> filter = %u, scale = %u\n",
		 ifname, width, height, title,
		 order, mode, radius, threads,
		 DCT_N, DCT_mode, quant_qf,
		 filter, scale);


	// img -> rgb_array

	pArray2d_init(&rgb_array, width, height);

	for(unsigned int h = 0; h < img->height; h++) {
		for(unsigned int w = 0; w < img->width; w++) {
			const unsigned int p = h * img->widthStep + 3 * w;

			rgb_array.p[w][h][0] = (unsigned char)img->imageData[p + 2]; // R
			rgb_array.p[w][h][1] = (unsigned char)img->imageData[p + 1]; // G
			rgb_array.p[w][h][2] = (unsigned char)img->imageData[p];     // B
		}
	}

	cvReleaseImage(&img);


	// Vorberechnungen (scale_sq = 0.5f)
	if(scale < 3) {
		precalcs_init(order,     0.5f, radius);
	} else {
		precalcs_init(order + 1, 0.5f, radius);
	}




	// Sq -> HIP

	clock_gettime(CLOCK_MONOTONIC, &begin);
	hipsampleColour(rgb_array, &rgb_hexarray, order, 2.0f, mode); // TODO: Notation
	clock_gettime(CLOCK_MONOTONIC, &end);
	CLOCK_DIFF(0);
	pArray2d_free(&rgb_array);
	strcpy(ofname, title);
	strcat(ofname, "-HIP.dat");
	Hexarray2file(&rgb_hexarray, ofname, 0);

	// TODO: Notation
	strcpy(ofname, title);
	strcat(ofname, "-HIP_1D.png");
	Hexarray2PNG_1D(rgb_hexarray, ofname);
	strcpy(ofname, title);
	strcat(ofname, "-HIP_2D.png");
	Hexarray2PNG_2D(rgb_hexarray, ofname);
	strcpy(ofname, title);
	strcat(ofname, "-HIP_2D_directed.png");
	Hexarray2PNG_2D_directed(rgb_hexarray, ofname);




	/*
	 * DCT
	 */

	if(DCT_N == 5) {
		float**       psiCos_table;
		RGB_Array      rgb_HDCTArray;
		fRGB_Hexarray frgb_HDCTHexarray;


		if(!DCT_mode) {
			HDCT_init(  &psiCos_table, 5 );
		} else if(DCT_mode == 1) {
			DCTH_init(  &psiCos_table, 5 );
		} else {
			DCTH2_init( &psiCos_table, 5 );
		}


		clock_gettime(CLOCK_MONOTONIC, &begin);

		if(!DCT_mode) {
			HDCT_N5(rgb_hexarray, &rgb_HDCTArray, psiCos_table);
		} else if(DCT_mode == 1) {
			DCTH(rgb_hexarray, &rgb_HDCTArray, psiCos_table, 5);
		} else if(DCT_mode == 2) {
			DCTH2(rgb_hexarray, &frgb_HDCTHexarray, psiCos_table, 1, 5);
		} else {
			DCTH2(rgb_hexarray, &frgb_HDCTHexarray, psiCos_table, 0, 5);
		}

		clock_gettime(CLOCK_MONOTONIC, &end);
		CLOCK_DIFF(1);

		if(DCT_mode > 1) {
			strcpy(ofname, title);
			strcat(ofname, "-HDCT.dat");
			Hexarray2file(&frgb_HDCTHexarray, ofname, 1);
		}


		/*
		 * Quantisierung
		 */

		if(quant_qf > 0 && quant_qf < 100) {
			clock_gettime(CLOCK_MONOTONIC, &begin);

			if(!DCT_mode) {
				Quant_custom(&rgb_HDCTArray, 0, 5, quant_qf);
			} else if(DCT_mode == 1) {
				Quant_custom(&rgb_HDCTArray, 1, 5, quant_qf);
			} else if(DCT_mode == 2 || DCT_mode == 3) {
				Quant_custom(&frgb_HDCTHexarray, 2, 5, quant_qf);
			} else {
				Quant_custom(&frgb_HDCTHexarray, DCT_mode - 1, 5, quant_qf);
			}

			clock_gettime(CLOCK_MONOTONIC, &end);
			CLOCK_DIFF(2);
		}


		clock_gettime(CLOCK_MONOTONIC, &begin);

		if(!DCT_mode) {
			IHDCT_N5(rgb_HDCTArray, &rgb_hexarray, psiCos_table);
		} else if(DCT_mode == 1) {
			IDCTH(rgb_HDCTArray, &rgb_hexarray, psiCos_table, 5);
		} else if(DCT_mode == 2) {
			IDCTH2(frgb_HDCTHexarray, &rgb_hexarray, psiCos_table, 1, 5);
		} else {
			IDCTH2(frgb_HDCTHexarray, &rgb_hexarray, psiCos_table, 0, 5);
		}

		clock_gettime(CLOCK_MONOTONIC, &end);
		CLOCK_DIFF(3);

		strcpy(ofname, title);
		strcat(ofname, "-HIP_nach_HDCT.dat");
		Hexarray2file(&rgb_hexarray, ofname, 0);


		if(!DCT_mode) {
			HDCT_free(&psiCos_table, 5);
		} else if(DCT_mode == 1) {
			DCTH_free(&psiCos_table, 0, 5);
		} else {
			DCTH_free(&psiCos_table, 1, 5);
		}
	} else if(DCT_N == 2) {
		float**       psiCos_table;
		fRGB_Hexarray frgb_HDCTHexarray;


		if(DCT_mode < 2) {
			HDCT_init(  &psiCos_table, 2 );
		} else {
			DCTH2_init( &psiCos_table, 2 );
		}


		clock_gettime(CLOCK_MONOTONIC, &begin);

		if(DCT_mode < 2) {
			HDCT_N2(rgb_hexarray, &frgb_HDCTHexarray, psiCos_table);
		} else if(DCT_mode == 2) {
			DCTH2(rgb_hexarray, &frgb_HDCTHexarray, psiCos_table, 1, 2);
		} else {
			DCTH2(rgb_hexarray, &frgb_HDCTHexarray, psiCos_table, 0, 2);
		}

		clock_gettime(CLOCK_MONOTONIC, &end);
		CLOCK_DIFF(1);

		strcpy(ofname, title);
		strcat(ofname, "-HDCT.dat");
		Hexarray2file(&frgb_HDCTHexarray, ofname, 1);


		/*
		 * Quantisierung
		 */

		if(quant_qf > 0 && quant_qf < 100) {
			clock_gettime(CLOCK_MONOTONIC, &begin);

			if(DCT_mode < 4) {
				Quant_custom(&frgb_HDCTHexarray, 2, 2, quant_qf);
			} else {
				Quant_custom(&frgb_HDCTHexarray, DCT_mode - 1, 2, quant_qf);
			}

			clock_gettime(CLOCK_MONOTONIC, &end);
			CLOCK_DIFF(2);
		}


		clock_gettime(CLOCK_MONOTONIC, &begin);

		if(DCT_mode < 2) {
			IHDCT_N2(frgb_HDCTHexarray, &rgb_hexarray, psiCos_table);
		} else if(DCT_mode == 2) {
			IDCTH2(frgb_HDCTHexarray, &rgb_hexarray, psiCos_table, 1, 2);
		} else {
			IDCTH2(frgb_HDCTHexarray, &rgb_hexarray, psiCos_table, 0, 2);
		}

		clock_gettime(CLOCK_MONOTONIC, &end);
		CLOCK_DIFF(3);

		strcpy(ofname, title);
		strcat(ofname, "-HIP_nach_HDCT.dat");
		Hexarray2file(&rgb_hexarray, ofname, 0);


		if(DCT_mode < 2) {
			HDCT_free(&psiCos_table, 2);
		} else {
			DCTH_free(&psiCos_table, 1, 2);
		}
	}


	/*
	 * Filter Banks (TODO: Notation)
	 */

	if(filter == 1 || filter == 2) {
		filter_unblurring( &rgb_hexarray, filter - 1 );
	} else if(filter) {
		filter_4x16(       &rgb_hexarray, filter - 3 );
	}


	/*
	 * Skalierung
	 */

	if(scale == 1) {
		Hexarray_scale_HIP(&rgb_hexarray, 0, 10);
	} else if(scale == 2) {
		Hexarray_scale_HIP(&rgb_hexarray, 1, 10);
	} else if(scale == 3) {
		Hexarray_scale(&rgb_hexarray, 0, 1, 0);
	} else if(scale > 3) {
		Hexarray_scale(&rgb_hexarray, 1, 1, 0);
	}




	// HIP -> sq
	clock_gettime(CLOCK_MONOTONIC, &begin);
	sqsampleColour(rgb_hexarray, &rgb_array, radius, 0.5f, mode, threads); // TODO: Notation
	clock_gettime(CLOCK_MONOTONIC, &end);
	CLOCK_DIFF(4);
	Hexarray_free(&rgb_hexarray, 0);




	// rgb_array -> img

	img = cvCreateImage(cvSize(rgb_array.x, rgb_array.y), IPL_DEPTH_8U, 3);

	for(unsigned int h = 0; h < rgb_array.y; h++) {
		for(unsigned int w = 0; w < rgb_array.x; w++) {
			const unsigned int p = h * img->widthStep + 3 * w;

			img->imageData[p + 2] = rgb_array.p[w][h][0]; // R
			img->imageData[p + 1] = rgb_array.p[w][h][1]; // G
			img->imageData[p]     = rgb_array.p[w][h][2]; // B
		}
	}

	strcpy(ofname, title);
	strcat(ofname, "-HIP2sq.png");
	cvSaveImage(ofname, img, NULL);
	cvReleaseImage(&img);
	strcpy(ofname, title);
	strcat(ofname, "-HIP2sq.ppm");
	pArray2d2PPM(rgb_array, ofname);
	pArray2d_free(&rgb_array);


	// Vorberechnungen
	precalcs_free();




	printf(
		"\nSq -> HIP : %.6fs\n"
		  "HDCT      : %.6fs\n"
		  "Quant     : %.6fs\n"
		  "IHDCT     : %.6fs\n"
		  "HIP -> sq : %.6fs\n",
		 runtimes[0],
		 runtimes[1],
		 runtimes[2],
		 runtimes[3],
		 runtimes[4]);


	return 0;
}
