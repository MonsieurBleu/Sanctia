ifeq ($(OS),Windows_NT)
	GEXEC = Game.exe
else
	GEXEC = Game
endif

CC = clang++

MAKE_FLAGS = --no-print-directory CC="$(CC)"
MAKE_PARALLEL = -j16 -k
MAKE_PARALLEL_LIGHT = -j4 -k


default : build-debug

run :
ifeq ($(OS),Windows_NT)
	cd build && $(GEXEC)
else
	cd build && ./$(GEXEC)
endif

run-debug :
	@cd build && gdb ./$(GEXEC)

build-debug : 
	@$(MAKE) -C ./Engine game $(MAKE_FLAGS) $(MAKE_PARALLEL) OPTFLAGS="-g"

build-debug-light : 
	@$(MAKE) -C ./Engine game $(MAKE_FLAGS) $(MAKE_PARALLEL_LIGHT) OPTFLAGS="-g"

build-debug-linear : 
	@$(MAKE) -C ./Engine game $(MAKE_FLAGS) OPTFLAGS="-g"

build-debug-fast :
	@$(MAKE) -C ./Engine game $(MAKE_FLAGS) $(MAKE_PARALLEL) OPTFLAGS="-g -Ofast"

build-debug-fast-light :
	@$(MAKE) -C ./Engine game $(MAKE_FLAGS) $(MAKE_PARALLEL_LIGHT) OPTFLAGS="-g -Ofast"

build-release : 
	@$(MAKE) -C ./Engine game $(MAKE_FLAGS) $(MAKE_PARALLEL) OPTFLAGS="-Ofast"

build-release-light :
	@$(MAKE) -C ./Engine game $(MAKE_FLAGS) $(MAKE_PARALLEL_LIGHT) OPTFLAGS="-Ofast"

clean : 
	@$(MAKE) -C ./Engine clean $(MAKE_FLAGS)
