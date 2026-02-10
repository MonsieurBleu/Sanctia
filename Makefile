ifeq ($(OS),Windows_NT)
	GEXEC = Sanctia_WIN64.exe
else
	GEXEC = Sanctia_UNIX.bin
endif

CC = clang++-20

MAKE_FLAGS = --no-print-directory CC="$(CC)" BUILD_DIR="Sanctia-Release" GEXEC="$(GEXEC)"
MAKE_FLAGS_WIN = --no-print-directory CC="clang++-20 --target=x86_64-w64-windows-gnu -femulated-tls" BUILD_DIR="Sanctia-Release" GEXEC="Sanctia_WIN64.exe"

MAKE_PARALLEL = -j8 -k
MAKE_PARALLEL_LIGHT = -j4 -k


default : build-debug 

build-all-plateforms : clean build-windows-release clean2 build-release

run :
ifeq ($(OS),Windows_NT)
	cd Sanctia-Release && $(GEXEC)
else
	cd Sanctia-Release && ./$(GEXEC)
endif

run-debug :
	@cd Sanctia-Release && gdb ./$(GEXEC)

run-windows : 
	@cd Sanctia-Release && wine ./Sanctia_WIN64.exe

run-windows-debug : 
	@cd Sanctia-Release && winedbg --gdb  ./Sanctia_WIN64.exe

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

clean2 : 
	@$(MAKE) -C ./Engine clean $(MAKE_FLAGS)


build-windows-release : 
	@$(MAKE) -C ./Engine game $(MAKE_FLAGS_WIN) $(MAKE_PARALLEL) OPTFLAGS="-O3 -ffast-math -Os"

build-windows-debug : 
	@$(MAKE) -C ./Engine game $(MAKE_FLAGS_WIN) $(MAKE_PARALLEL) OPTFLAGS="-g"



package-all-plateforms :
	rm -rf "Public Release"
	mkdir "Public Release"
	cp -R Sanctia-Release "Public Release/Sanctia Windows"
	cp -R Sanctia-Release "Public Release/Sanctia Linux"
	find "Public Release/Sanctia Linux" -name "*.exe" -type f -delete
	find "Public Release/Sanctia Linux" -name "*.dll*" -type f -delete
	find "Public Release/Sanctia Linux" -name "*3dll*" -type f -delete
	find "Public Release/Sanctia Linux" -name "*.lib" -type f -delete
	find "Public Release/Sanctia Linux" -name "*_WIN64.*" -type f -delete
	find "Public Release/Sanctia Linux" -name ".git*" -type f -delete
	find "Public Release/Sanctia Windows" -name ".git*" -type f -delete
	find "Public Release/Sanctia Windows" -name "*_UNIX.*" -type f -delete
	rm -rf "Public Release/Sanctia Windows/lib"
	rm -rf "Public Release/Sanctia Windows/Import"
	rm -rf "Public Release/Sanctia Linux/Import"
	cd "Public Release" && zip -r "Sanctia Linux.zip" "Sanctia Linux/"
	cd "Public Release" && zip -r "Sanctia Windows.zip" "Sanctia Windows/"