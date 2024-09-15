ifeq ($(OS),Windows_NT)
	GEXEC = Game.exe
else
	GEXEC = Game
endif

MAKE_FLAGS = --no-print-directory
MAKE_PARALLEL = -j -k

default : debug

run :
ifeq ($(OS),Windows_NT)
	cd build && $(GEXEC)
else
	cd build && ./$(GEXEC)
endif

run-debug :
	@cd build && gdb ./$(GEXEC)

debug : 
	@$(MAKE) -C ./Engine game $(MAKE_FLAGS) $(MAKE_PARALLEL) OPTFLAGS="-g"

debug-linear : 
	@$(MAKE) -C ./Engine game $(MAKE_FLAGS) OPTFLAGS="-g"

debug-fast :
	@$(MAKE) -C ./Engine game $(MAKE_FLAGS) OPTFLAGS="-g -Ofast"

release : 
	@$(MAKE) -C ./Engine game $(MAKE_FLAGS) $(MAKE_PARALLEL) OPTFLAGS="-Ofast"

clean : 
	@$(MAKE) -C ./Engine clean $(MAKE_FLAGS)
