# Compiler
CXX = g++
# bookworm:12, trixie:14, ubuntu24.04:13

# Compiler and linker flags
CXXFLAGS = -m64 -fPIC -fno-strict-aliasing -fexceptions -fopenmp -DNDEBUG -DIL_STD -std=c++14 -O3
LDFLAGS = -m64 -fPIC -fno-strict-aliasing -fexceptions -fopenmp -DNDEBUG -DIL_STD -lm -lpthread -ltiff

# Test-specific linker flags (for Catch2 v3 - optional, will be ignored if not available)
# Detect Catch2 version from package (for info only)
CATCH2_PKG_VERSION := $(shell dpkg-query -W -f='$${Version}' catch2 2>/dev/null || dpkg-query -W -f='$${Version}' libcatch2-dev 2>/dev/null || echo "unknown")
# Auto-detect if Catch2 v3 libraries are available by trying to link
CATCH2_V3_LIBS := $(shell echo "int main(){}" | $(CXX) -x c++ - -lCatch2Main -lCatch2 -o /dev/null 2>/dev/null && echo "-lCatch2Main -lCatch2" || echo "")
TEST_LDFLAGS = $(LDFLAGS) $(CATCH2_V3_LIBS)

# Source files
SRCS = Cell2Fire.cpp Cells.cpp FuelModelUtils.cpp FuelModelSpain.cpp FuelModelPortugal.cpp FuelModelKitral.cpp FuelModelFBP.cpp Spotting.cpp ReadCSV.cpp ReadArgs.cpp Lightning.cpp WriteCSV.cpp DataGenerator.cpp

# Object files
OBJS = $(SRCS:.cpp=.o)

# Target executable
TARGET = Cell2Fire
TEST_TARGET = test_cell2fire

# Test source files
TEST_DIR = ../test/unit_tests
TEST_FILES = $(TEST_DIR)/*.cpp FuelModelKitral.cpp FuelModelUtils.cpp

# Build rules
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LDFLAGS)

tests: $(TEST_TARGET)
	./$(TEST_TARGET)

# Automatically uses Catch2 v3 libs if available, otherwise header-only v2
$(TEST_TARGET): $(TEST_FILES) $(SRCS)
	$(CXX) $(CXXFLAGS) -o $(TEST_TARGET) $(TEST_FILES) $(TEST_LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET) $(TEST_TARGET)

# Show build configuration
info:
	@echo "Compiler: $(CXX)"
	@echo "Catch2 Package Version: $(CATCH2_PKG_VERSION)"
	@echo "Catch2 v3 Libraries: $(if $(CATCH2_V3_LIBS),YES ($(CATCH2_V3_LIBS)),NO (header-only v2))"
	@echo "Test LDFLAGS: $(TEST_LDFLAGS)"

DESTDIR = /usr/local/bin
install: all
	cp $(TARGETS) $(DESTDIR)
uninstall: all
	rm $(DESTDIR)/$(TARGETS)
