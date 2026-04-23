#Welcome to what must be the most terrible makefile ever (but hey, it works)
#Both clang & gcc work fine - clang seems to output faster code
#.NET 6 (and its dev tools) must be installed to compile the UI.
#The emulation core also requires SDL2.
#Run "make" to build, "make run" to run

NEXENFLAGS=
EXTRA_CXXFLAGS ?=

ifeq ($(USE_GCC),true)
	CXX := g++
	CC := gcc
	PROFILE_GEN_FLAG := -fprofile-generate
	PROFILE_USE_FLAG := -fprofile-use
else
	CXX := clang++
	CC := clang
	PROFILE_GEN_FLAG := -fprofile-instr-generate=$(CURDIR)/PGOHelper/pgo.profraw
	PROFILE_USE_FLAG := -fprofile-instr-use=$(CURDIR)/PGOHelper/pgo.profdata
endif

SDL2LIB := $(shell sdl2-config --libs)
SDL2INC := $(shell sdl2-config --cflags)

LINKCHECKUNRESOLVED := -Wl,-z,defs

LINKOPTIONS :=
NEXENOS :=
UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Linux)
	NEXENOS := linux
	SHAREDLIB := NexenCore.so
endif

ifeq ($(UNAME_S),Darwin)
	NEXENOS := osx
	SHAREDLIB := NexenCore.dylib
	LTO := false
	STATICLINK := false
	LINKCHECKUNRESOLVED :=
endif

NEXENFLAGS += -m64

MACHINE := $(shell uname -m)
ifeq ($(MACHINE),x86_64)
	NEXENPLATFORM := $(NEXENOS)-x64
endif
ifneq ($(filter %86,$(MACHINE)),)
	NEXENPLATFORM := $(NEXENOS)-x64
endif
# TODO: this returns `aarch64` on one of my machines...
ifneq ($(filter arm%,$(MACHINE)),)
	NEXENPLATFORM := $(NEXENOS)-arm64
endif
ifeq ($(MACHINE),aarch64)
	NEXENPLATFORM := $(NEXENOS)-arm64
	ifeq ($(USE_GCC),true)
		#don't set -m64 on arm64 for gcc (unrecognized option)
		NEXENFLAGS=
	endif
endif

DEBUG ?= 0

ifeq ($(DEBUG),0)
	NEXENFLAGS += -O3
	ifneq ($(LTO),false)
		NEXENFLAGS += -DHAVE_LTO
		ifneq ($(USE_GCC),true)
			NEXENFLAGS += -flto=thin
		else
			NEXENFLAGS += -flto=auto
		endif
	endif
else
	NEXENFLAGS += -O0 -g
	# Note: if compiling with a sanitizer, you will likely need to `LD_PRELOAD` the library `libNexenCore.so` will be linked against.
	ifneq ($(SANITIZER),)
		ifeq ($(SANITIZER),address)
			# Currently, `-fsanitize=address` is not supported together with `-fsanitize=thread`
			NEXENFLAGS += -fsanitize=address
		else ifeq ($(SANITIZER),thread)
			# Currently, `-fsanitize=address` is not supported together with `-fsanitize=thread`
			NEXENFLAGS += -fsanitize=thread
		else
$(warning Unrecognised $$(SANITIZER) value: $(SANITIZER))
		endif
		# `-Wl,-z,defs` is incompatible with the sanitizers in a shared lib, unless the sanitizer libs are linked dynamically; hence `-shared-libsan` (not the default for Clang).
		# It seems impossible to link dynamically against two sanitizers at the same time, but that might be a Clang limitation.
		ifneq ($(USE_GCC),true)
			NEXENFLAGS += -shared-libsan
		endif
	endif
endif

ifeq ($(PGO),profile)
	NEXENFLAGS += ${PROFILE_GEN_FLAG}
endif

ifeq ($(PGO),optimize)
	NEXENFLAGS += ${PROFILE_USE_FLAG}
endif

ifneq ($(STATICLINK),false)
	LINKOPTIONS += -static-libgcc -static-libstdc++
endif

ifeq ($(NEXENOS),osx)
	LINKOPTIONS += -framework Foundation -framework Cocoa -framework GameController -framework CoreHaptics -Wl,-rpath,/opt/local/lib
endif

CXXFLAGS = -fPIC -Wall --std=c++23 $(NEXENFLAGS) $(EXTRA_CXXFLAGS) $(SDL2INC) -I $(realpath ./) -I $(realpath ./Core) -I $(realpath ./Utilities) -I $(realpath ./Sdl) -I $(realpath ./Linux) -I $(realpath ./MacOS)
OBJCXXFLAGS = $(CXXFLAGS)
CFLAGS = -fPIC -Wall $(NEXENFLAGS) $(EXTRA_CXXFLAGS)

OBJFOLDER := obj.$(NEXENPLATFORM)
DEBUGFOLDER := bin/$(NEXENPLATFORM)/Debug
RELEASEFOLDER := bin/$(NEXENPLATFORM)/Release
ifeq ($(DEBUG), 0)
	OUTFOLDER = $(RELEASEFOLDER)
	BUILD_TYPE := Release
	OPTIMIZEUI := -p:OptimizeUi=true
else
	OUTFOLDER = $(DEBUGFOLDER)
	BUILD_TYPE := Debug
	OPTIMIZEUI :=
endif


ifeq ($(USE_AOT),true)
	PUBLISHFLAGS ?=  -r $(NEXENPLATFORM) -p:PublishSingleFile=false -p:PublishAot=true -p:SelfContained=true
else
	PUBLISHFLAGS ?=  -r $(NEXENPLATFORM) --self-contained true -p:PublishSingleFile=true -p:IncludeNativeLibrariesForSelfExtract=false
endif


CORESRC := $(shell find Core -name '*.cpp')
COREOBJ := $(CORESRC:.cpp=.o)

UTILSRC := $(shell find Utilities -name '*.cpp' -o -name '*.c')
UTILOBJ := $(addsuffix .o,$(basename $(UTILSRC)))

SDLSRC := $(shell find Sdl -name '*.cpp')
SDLOBJ := $(SDLSRC:.cpp=.o)

LUASRC := $(shell find Lua -name '*.c')
LUAOBJ := $(LUASRC:.c=.o)

ifeq ($(NEXENOS),linux)
	LINUXSRC := $(shell find Linux -name '*.cpp')
else
	LINUXSRC :=
endif
LINUXOBJ := $(LINUXSRC:.cpp=.o)

ifeq ($(NEXENOS),osx)
	MACOSSRC := $(shell find MacOS -name '*.mm')
else
	MACOSSRC :=
endif
MACOSOBJ := $(MACOSSRC:.mm=.o)

DLLSRC := $(shell find InteropDLL -name '*.cpp')
DLLOBJ := $(DLLSRC:.cpp=.o)

ifeq ($(SYSTEM_LIBEVDEV), true)
	LIBEVDEVLIB := $(shell pkg-config --libs libevdev)
	LIBEVDEVINC := $(shell pkg-config --cflags libevdev)
else
	LIBEVDEVSRC := $(shell find Linux/libevdev -name '*.c')
	LIBEVDEVOBJ := $(LIBEVDEVSRC:.c=.o)
	LIBEVDEVINC := -I../
endif

ifeq ($(NEXENOS),linux)
	X11LIB := -lX11
else
	X11LIB :=
endif

FSLIB := -lstdc++fs

ifeq ($(NEXENOS),osx)
	LIBEVDEVOBJ :=
	LIBEVDEVINC :=
	LIBEVDEVSRC :=
	FSLIB :=
	ifeq ($(USE_AOT),true)
		PUBLISHFLAGS := -t:BundleApp -p:UseAppHost=true -p:RuntimeIdentifier=$(NEXENPLATFORM) -p:PublishSingleFile=false -p:PublishAot=true -p:SelfContained=true
	else
		PUBLISHFLAGS := -t:BundleApp -p:UseAppHost=true -p:RuntimeIdentifier=$(NEXENPLATFORM) -p:SelfContained=true -p:PublishSingleFile=false -p:PublishReadyToRun=false
	endif
endif

all: ui

ui: InteropDLL/$(OBJFOLDER)/$(SHAREDLIB)
	mkdir -p $(OUTFOLDER)/Dependencies
	rm -fr $(OUTFOLDER)/Dependencies/*
	cp InteropDLL/$(OBJFOLDER)/$(SHAREDLIB) $(OUTFOLDER)/$(SHAREDLIB)
	#Called twice because the first call copies native libraries to the bin folder which need to be included in Dependencies.zip
	#Don't run with AOT flags the first time to reduce build duration
	cd UI && dotnet publish -c $(BUILD_TYPE) $(OPTIMIZEUI) -r $(NEXENPLATFORM)
	cd UI && dotnet publish -c $(BUILD_TYPE) $(OPTIMIZEUI) $(PUBLISHFLAGS)

core: InteropDLL/$(OBJFOLDER)/$(SHAREDLIB)

pgohelper: InteropDLL/$(OBJFOLDER)/$(SHAREDLIB)
	mkdir -p PGOHelper/$(OBJFOLDER) && cd PGOHelper/$(OBJFOLDER) && $(CXX) $(CXXFLAGS) $(LINKCHECKUNRESOLVED) -o pgohelper ../PGOHelper.cpp ../../bin/pgohelperlib.so -pthread $(FSLIB) $(SDL2LIB) $(LIBEVDEVLIB) $(X11LIB)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.mm
	$(CXX) $(OBJCXXFLAGS) -c $< -o $@

InteropDLL/$(OBJFOLDER)/$(SHAREDLIB): $(LUAOBJ) $(UTILOBJ) $(COREOBJ) $(SDLOBJ) $(LIBEVDEVOBJ) $(LINUXOBJ) $(DLLOBJ) $(MACOSOBJ)
	mkdir -p bin
	mkdir -p InteropDLL/$(OBJFOLDER)
	$(CXX) $(CXXFLAGS) $(LINKOPTIONS) $(LINKCHECKUNRESOLVED) -shared -o $(SHAREDLIB) $(DLLOBJ) $(LUAOBJ) $(LINUXOBJ) $(MACOSOBJ) $(LIBEVDEVOBJ) $(UTILOBJ) $(SDLOBJ) $(COREOBJ) $(SDL2INC) -pthread $(FSLIB) $(SDL2LIB) $(LIBEVDEVLIB) $(X11LIB)
	cp $(SHAREDLIB) bin/pgohelperlib.so
	mv $(SHAREDLIB) InteropDLL/$(OBJFOLDER)

pgo:
	./buildPGO.sh

run:
	$(OUTFOLDER)/$(NEXENPLATFORM)/publish/Nexen

clean:
	rm -r -f $(COREOBJ)
	rm -r -f $(UTILOBJ)
	rm -r -f $(LINUXOBJ) $(LIBEVDEVOBJ)
	rm -r -f $(SDLOBJ)
	rm -r -f $(LUAOBJ)
	rm -r -f $(MACOSOBJ)
	rm -r -f $(DLLOBJ)
