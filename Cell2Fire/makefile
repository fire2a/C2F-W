# =============================================================================
# Cell2Fire-W — makefile portable (Linux + macOS + Windows)
#
# Linux    : make
# macOS    : make OMP=libomp        (default: clang de Apple + libomp de brew)
#            make OMP=gcc           (alternativa: g++ de Homebrew)
#            make OMP=none          (sin paralelismo)
# Windows  : make                   (dentro de la shell MINGW64 de MSYS2)
#            make SHARED=1          (enlace dinamico; requiere DLLs en PATH)
# Cross    : make CROSS=win64       (compilar el .exe desde Linux)
#
# Extra    : make fat-binary        copia el binario con el nombre que espera FAT
#            make fat-name          solo imprime ese nombre
#            make info              muestra la configuracion detectada
#
# Dependencias
#   Linux  : apt install libtiff-dev libboost-all-dev
#   macOS  : brew install libtiff boost libomp        (o gcc en vez de libomp)
#   Windows: en la shell MSYS2 MINGW64:
#            pacman -S --needed make mingw-w64-x86_64-gcc \
#                               mingw-w64-x86_64-libtiff mingw-w64-x86_64-boost
#   Cross  : apt install g++-mingw-w64-x86-64   (+ libtiff/boost para mingw)
# =============================================================================

UNAME_S := $(shell uname -s)

# MSYS2/MinGW/Cygwin reportan MINGW64_NT-..., MSYS_NT-..., CYGWIN_NT-...
ifneq (,$(findstring MINGW,$(UNAME_S)))
  WINDOWS = 1
endif
ifneq (,$(findstring MSYS,$(UNAME_S)))
  WINDOWS = 1
endif
ifneq (,$(findstring CYGWIN,$(UNAME_S)))
  WINDOWS = 1
endif
ifeq ($(CROSS),win64)
  WINDOWS = 1
  CROSSING = 1
endif

# ---------- Fuentes ----------
SRCS = Cell2Fire.cpp Cells.cpp FuelModelUtils.cpp ScottAndBurganKernel.cpp \
       KitralKernel.cpp CanadianFBPKernel.cpp Spotting.cpp ReadCSV.cpp \
       ReadArgs.cpp Lightning.cpp WriteCSV.cpp DataGenerator.cpp
OBJS   = $(SRCS:.cpp=.o)
TEST_TARGET = test_cell2fire
TEST_DIR   = ../test/unit_tests
TEST_FILES = $(TEST_DIR)/*.cpp KitralKernel.cpp FuelModelUtils.cpp

# ---------- Version ----------
# Se deriva del tag de git si existe; si no, cae al default. Se puede forzar:
#   make VERSION=v2.0.1
VERSION ?= $(shell git describe --tags --dirty 2>/dev/null || echo v2.0.0)

# ---------- Flags comunes ----------
# _USE_MATH_DEFINES: sin esto M_PI no existe en MinGW ni MSVC (no es estandar C++)
COMMON = -fno-strict-aliasing -fexceptions -DNDEBUG -DIL_STD -D_USE_MATH_DEFINES -std=c++14 \
         -DC2FW_VERSION_STR='"$(VERSION)"'

# =============================================================================
# Windows (MSYS2/MinGW nativo, o cross desde Linux)
# =============================================================================
ifdef WINDOWS
  TARGET = Cell2Fire.exe

  ifdef CROSSING
    CXX = x86_64-w64-mingw32-g++
    # Raiz del sysroot mingw. NO usamos el pkg-config del host: devolveria rutas
    # de Linux (/usr/include/...) y los headers de glibc envenenan el build
    # cruzado con errores confusos. Apunta MINGW_ROOT a un prefijo que tenga
    # los headers/libs de libtiff y boost compilados para Windows.
    MINGW_ROOT ?= /usr/x86_64-w64-mingw32
  else
    CXX = g++
  endif

  ARCHFLAG =
  OMPC = -fopenmp
  OMPL = -fopenmp

  # -fPIC no aplica en Windows (todo es relocatable); gcc emite un warning.
  PICFLAG =

  # Enlace ESTATICO por defecto, a proposito: un .exe dinamico de MinGW necesita
  # libstdc++-6.dll, libgomp-1.dll, libwinpthread-1.dll, libtiff-*.dll y las
  # dependencias de libtiff (zlib, jpeg, lzma, zstd, webp...) en el PATH. QGIS
  # no tiene el PATH de MSYS2, asi que un .exe dinamico falla al arrancar dentro
  # del plugin con un error de DLL faltante poco informativo.
  ifdef SHARED
    STATICFLAGS =
    TIFFLIBS    = -ltiff
  else
    STATICFLAGS = -static -static-libgcc -static-libstdc++
    # pkg-config resuelve las dependencias transitivas de libtiff, que en
    # estatico son varias y varian segun el build de MSYS2.
    ifdef CROSSING
      TIFFLIBS =
    else
      TIFFLIBS := $(shell pkg-config --static --libs libtiff-4 2>/dev/null)
    endif
    ifeq ($(TIFFLIBS),)
      # Respaldo si pkg-config no esta disponible. En enlace estatico el ORDEN
      # importa: cada dependencia va DESPUES de quien la usa. libtiff necesita
      # jbig (codec JBIG) y libwebp necesita sharpyuv; omitirlas da errores
      # "undefined reference to jbg_*" y "SharpYuv*".
      # Lo correcto es instalar pkgconf: pacman -S mingw-w64-x86_64-pkgconf
      # Lista tomada de makefile.static de upstream (probada). Ojo: -lLerc con
      # mayuscula; en Linux el nombre es sensible a mayusculas.
      TIFFLIBS = -ltiff -ljpeg -ljbig -llzma -lzstd -lwebp -ldeflate -lz -lLerc -lsharpyuv
      $(warning pkg-config no resolvio libtiff: usando lista de respaldo, puede estar incompleta.)
      $(warning Instala pkgconf para obtener la lista real:  pacman -S mingw-w64-x86_64-pkgconf)
    endif
  endif

  ifdef CROSSING
    INCS = -I$(MINGW_ROOT)/include
    LIBS = -L$(MINGW_ROOT)/lib $(TIFFLIBS) -lm
  else
    # MSYS2 nativo: aqui pkg-config SI es el de mingw, asi que es correcto
    INCS = $(shell pkg-config --cflags libtiff-4 2>/dev/null)
    LIBS = $(TIFFLIBS) -lm
  endif
endif

# =============================================================================
# Linux
# =============================================================================
ifeq ($(UNAME_S),Linux)
ifndef WINDOWS
  TARGET    = Cell2Fire
  CXX      ?= g++
  ARCHFLAG  = -m64
  PICFLAG   = -fPIC
  OMPC      = -fopenmp
  OMPL      = -fopenmp
  INCS      =
  LIBS      = -lm -lpthread -ltiff
  STATICFLAGS =
endif
endif

# =============================================================================
# macOS
# =============================================================================
ifeq ($(UNAME_S),Darwin)
  TARGET = Cell2Fire
  BREW  := $(shell brew --prefix 2>/dev/null || echo /opt/homebrew)

  # SDK de macOS. Imprescindible si el compilador es el LLVM de Homebrew, que trae
  # su propio libc++ pero no sabe donde esta el SDK -> cascada de errores tipo
  # "unknown type name 'ldiv_t'", "mbstate_t without <wchar.h>", "FP_NAN undeclared".
  SDKPATH := $(shell xcrun --show-sdk-path 2>/dev/null)
  SYSROOT  = $(if $(SDKPATH),-isysroot $(SDKPATH),)

  ARCHFLAG =
  PICFLAG  = -fPIC
  STATICFLAGS =

  INCS = $(SYSROOT) -I$(BREW)/include -I$(BREW)/opt/libtiff/include
  LIBS = $(SYSROOT) -lm -lpthread -ltiff -L$(BREW)/lib -L$(BREW)/opt/libtiff/lib

  OMP ?= libomp

  ifeq ($(OMP),libomp)
    # Ruta ABSOLUTA a proposito: si en PATH viene primero el clang++ del LLVM de
    # Homebrew (/usr/local/opt/llvm/bin) o el de conda, la compilacion falla.
    CXX   = /usr/bin/clang++
    OMPC  = -Xpreprocessor -fopenmp -I$(BREW)/opt/libomp/include
    OMPL  = -lomp -L$(BREW)/opt/libomp/lib
  endif

  ifeq ($(OMP),gcc)
    GCC_BIN := $(shell ls $(BREW)/bin/g++-1[0-9] 2>/dev/null | sort -V | tail -1)
    ifeq ($(GCC_BIN),)
      $(warning No encontre g++-1X en $(BREW)/bin. Instala con: brew install gcc)
      $(warning OJO: el g++ del sistema en macOS es un alias de clang y NO acepta -fopenmp.)
      CXX = g++
    else
      CXX = $(GCC_BIN)
    endif
    OMPC = -fopenmp
    OMPL = -fopenmp
  endif

  ifeq ($(OMP),none)
    CXX   = /usr/bin/clang++
    OMPC  =
    OMPL  =
  endif
endif

# ---------- Ensamblado final ----------
CXXFLAGS = $(ARCHFLAG) $(PICFLAG) $(COMMON) $(OMPC) $(INCS) -O3
LDFLAGS  = $(ARCHFLAG) $(PICFLAG) $(COMMON) $(STATICFLAGS) $(OMPL) $(LIBS)

# Deteccion de Catch2 v3. Se enlaza a un archivo temporal y no a /dev/null: en MSYS2 el
# enlazador de Windows no puede escribir ahi y aborta con "final link failed: file
# truncated", lo que daba un falso negativo y hacia enlazar los tests SIN Catch2 --
# cientos de referencias no resueltas y un "undefined reference to WinMain" al final.
CATCH2_PROBE := $(shell mktemp -u).probe
CATCH2_V3_LIBS := $(shell echo "int main(){}" | $(CXX) -x c++ - $(CXXFLAGS) -lCatch2Main -lCatch2 -o $(CATCH2_PROBE) 2>/dev/null && echo "-lCatch2Main -lCatch2" || echo ""; rm -f $(CATCH2_PROBE) $(CATCH2_PROBE).exe)
TEST_LDFLAGS = $(LDFLAGS) $(CATCH2_V3_LIBS)

# =============================================================================
# Reglas
# =============================================================================
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

tests: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TEST_TARGET): $(TEST_FILES) $(SRCS)
	$(CXX) $(CXXFLAGS) -o $(TEST_TARGET) $(TEST_FILES) $(TEST_LDFLAGS)

clean:
	rm -f $(OBJS) Cell2Fire Cell2Fire.exe $(TEST_TARGET)

# -----------------------------------------------------------------------------
# Nombre del binario para el plugin FAT (replica get_ext())
#
#   Windows : Cell2Fire.exe                        (sin sufijo de plataforma)
#   Linux   : Cell2Fire.<distro>.<codename>.<arch>
#   macOS   : Cell2Fire_Darwin.macOS-<M>.<arch>-static
#             ojo: empieza con "_", en arm64 la version se FUERZA a 14, en Intel
#             `arch` da i386 y la version se recorta a 12-13, y el "-static" es
#             obligatorio (el fallback que lo quitaria esta detras de un
#             which("otool -L") que siempre da None, o sea nunca corre).
# -----------------------------------------------------------------------------
fat-name:
ifdef WINDOWS
	@echo "Cell2Fire.exe"
else
	@if [ "$(UNAME_S)" = "Darwin" ]; then \
		ARCH=$$(arch); \
		MAJ=$$(sw_vers -productVersion | cut -d. -f1); \
		if [ "$$ARCH" = "arm64" ]; then MAJ=14; fi; \
		if [ "$$ARCH" = "i386" ] && [ "$$MAJ" -gt 13 ]; then MAJ=13; fi; \
		if [ "$$ARCH" = "i386" ] && [ "$$MAJ" -lt 12 ]; then MAJ=12; fi; \
		echo "Cell2Fire_$$(uname -s).$$(sw_vers -productName)-$$MAJ.$$ARCH-static"; \
	else \
		echo "Cell2Fire.$$(lsb_release --short --id).$$(lsb_release --short --codename).$$(uname --machine)"; \
	fi
endif

fat-binary: $(TARGET)
	@NAME=$$($(MAKE) --no-print-directory fat-name); \
	if [ "$(TARGET)" != "$$NAME" ]; then cp $(TARGET) "$$NAME"; chmod +x "$$NAME"; fi; \
	echo "-> $$NAME"; \
	echo "   copialo a  <plugin>/simulator/C2F/Cell2Fire/"

info:
	@echo "VERSION       : $(VERSION)"
	@echo "OS            : $(UNAME_S)$(if $(WINDOWS), (Windows),)$(if $(CROSSING), [cross],)"
	@echo "CXX           : $(CXX)"
	@echo "TARGET        : $(TARGET)"
ifeq ($(UNAME_S),Darwin)
	@echo "OMP variant   : $(OMP)"
	@echo "brew prefix   : $(BREW)"
	@echo "macOS SDK     : $(if $(SDKPATH),$(SDKPATH),NO DETECTADO -- xcode-select --install)"
endif
ifdef WINDOWS
	@echo "link          : $(if $(SHARED),dinamico (necesita DLLs en PATH),estatico)"
	@echo "libtiff libs  : $(TIFFLIBS)"
endif
	@echo "CXXFLAGS      : $(CXXFLAGS)"
	@echo "LDFLAGS       : $(LDFLAGS)"
	@printf "FAT binary    : "; $(MAKE) --no-print-directory fat-name

DESTDIR = /usr/local/bin
install: all
	cp $(TARGET) $(DESTDIR)
uninstall:
	rm -f $(DESTDIR)/$(TARGET)

.PHONY: all tests clean info install uninstall fat-name fat-binary
