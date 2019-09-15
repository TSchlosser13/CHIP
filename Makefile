# Makefile


CFLAGS    = -march=native -Ofast -pipe -std=c99 -Wall
CFLAGS   += -Wno-unused-function -Wno-unused-result
includes  = Misc/pArray2d.c Misc/Precalcs.c
includes += CHIP/Hexint.c CHIP/Hexarray.c CHIP/Hexsamp.c
includes += HDCT/HDCT.c HFBs/HFBs.c Quant/coefTables.c Quant/Quant.c
LDFLS     = -lm -lpthread `pkg-config opencv --cflags --libs`


.PHONY: gcc icc clang optimize

gcc: CHIP-Test.c
	gcc   $(CFLAGS) CHIP-Test.c -o CHIP-Test $(includes) $(LDFLS)

clang: CHIP-Test.c
	clang $(CFLAGS) CHIP-Test.c -o CHIP-Test $(includes) $(LDFLS)

icc: CHIP-Test.c
	icc   $(CFLAGS) CHIP-Test.c -o CHIP-Test $(includes) $(LDFLS)

optimize: CHIP-Test.c
	test -d _Profiling || mkdir _Profiling
	gcc -fprofile-generate=_Profiling $(CFLAGS) CHIP-Test.c -o CHIP-Test $(includes) $(LDFLS)
	@printf "\n\n__Testdurchlauf__\n\n"
	./CHIP-Test Tests/Testset/Lena_cropped.bmp Lena 5 1 1.0 1 5 3 90 0 0
	@printf "\n\n"
	gcc -fprofile-use=_Profiling      $(CFLAGS) CHIP-Test.c -o CHIP-Test $(includes) $(LDFLS)

help:
	@echo "/*****************************************************************************"
	@echo " * help : CHIP-Test - v1.0 - 01.04.2016"
	@echo " *****************************************************************************"
	@echo " * + gcc, clang und icc : Wahl des Compilers"
	@echo " * + optimize           : Optimierter Bau mittels Profiling (GCC)"
	@echo " * + test               : Testlauf"
	@echo " * + clean              : Aufräumen"
	@echo " *****************************************************************************/"

clean:
	find . -maxdepth 1 ! -name "*.c" ! -name "*.man" ! -name "COPYING" \
		! -name "Makefile" -type f -delete
	rm -fr _Profiling

test:
	./CHIP-Test Tests/Testset/Lena_cropped.bmp Lena 5 1 1.0 1 5 3 90 0 0
