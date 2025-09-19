ifeq ($(OS),Windows_NT)
	GEXEC = Sanctia_WIN64.exe
else
	GEXEC = Sanctia_UNIX.bin
endif

CC = clang++

MAKE_FLAGS = --no-print-directory CC="$(CC)" BUILD_DIR="Sanctia-Release" GEXEC="$(GEXEC)"
MAKE_PARALLEL = -j8 -k
MAKE_PARALLEL_LIGHT = -j4 -k


default : build-debug

run :
ifeq ($(OS),Windows_NT)
	cd Sanctia-Release && $(GEXEC)
else
	cd Sanctia-Release && ./$(GEXEC)
endif

run-debug :
	@cd Sanctia-Release && gdb ./$(GEXEC)

build-debug : 
	@$(MAKE) -C ./Engine game $(MAKE_FLAGS) $(MAKE_PARALLEL) OPTFLAGS="-g"

build-debug-light : 
	@$(MAKE) -C ./Engine game $(MAKE_FLAGS) $(MAKE_PARALLEL_LIGHT) OPTFLAGS="-g"

build-debug-linear : 
	@$(MAKE) -C ./Engine game $(MAKE_FLAGS) OPTFLAGS="-g"

build-debug-fast :
	@$(MAKE) -C ./Engine game $(MAKE_FLAGS) $(MAKE_PARALLEL) OPTFLAGS="-g -O3 -ffast-math"

build-debug-fast-light :
	@$(MAKE) -C ./Engine game $(MAKE_FLAGS) $(MAKE_PARALLEL_LIGHT) OPTFLAGS="-g -O3 -ffast-math"

build-release : 
	@$(MAKE) -C ./Engine game $(MAKE_FLAGS) $(MAKE_PARALLEL) OPTFLAGS="-O3 -ffast-math -Os"

build-release-light :
	@$(MAKE) -C ./Engine game $(MAKE_FLAGS) $(MAKE_PARALLEL_LIGHT) OPTFLAGS="-O3 -ffast-math"

clean : 
	@$(MAKE) -C ./Engine clean $(MAKE_FLAGS)
