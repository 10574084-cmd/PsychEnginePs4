TITLE := PsychEngine PS4
VERSION := 01.02
TITLE_ID := PSYH00001
CONTENT_ID := IV0000-PSYH00001_00-PSYCHENG00000000

TOOLCHAIN := $(OO_PS4_TOOLCHAIN)
PKGTOOLS := $(TOOLCHAIN)/bin/linux
COMMONDIR := $(TOOLCHAIN)/samples/_common
INTDIR := build
OUT_ELF := $(INTDIR)/psychengine_ps4_bootstrap.elf
OUT_OELF := $(INTDIR)/psychengine_ps4_bootstrap.oelf
PKG := $(CONTENT_ID).pkg
MENU_ASSETS := $(wildcard assets/psychmenu/*.png)

LIBS := -lc -lkernel -lc++ -lpthread -lSceVideoOut -lScePad -lSceUserService -lSceAudioOut
OBJS := $(INTDIR)/main.o $(INTDIR)/graphics.o $(INTDIR)/ps4png.o $(INTDIR)/audio_player.o $(INTDIR)/stb_vorbis.o
TARGET_FLAGS := --target=x86_64-pc-freebsd12-elf -fPIC -funwind-tables -c -isysroot $(TOOLCHAIN) -isystem $(TOOLCHAIN)/include
CXXFLAGS := $(TARGET_FLAGS) -isystem $(TOOLCHAIN)/include/c++/v1 -I$(COMMONDIR) -Isrc
LDFLAGS := -m elf_x86_64 -pie --script $(TOOLCHAIN)/link.x --eh-frame-hdr -L$(TOOLCHAIN)/lib $(LIBS) $(TOOLCHAIN)/lib/crt1.o

all: $(PKG)

$(INTDIR):
	mkdir -p $@

$(INTDIR)/main.o: src/main.cpp src/ps4png.h $(COMMONDIR)/graphics.h | $(INTDIR)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(INTDIR)/graphics.o: $(COMMONDIR)/graphics.cpp $(COMMONDIR)/graphics.h
	$(CXX) $(CXXFLAGS) -o $@ $<

$(INTDIR)/ps4png.o: src/ps4png.cpp src/ps4png.h $(COMMONDIR)/graphics.h
	$(CXX) $(CXXFLAGS) -o $@ $<

$(INTDIR)/audio_player.o: src/audio_player.cpp src/audio_player.h src/stb_vorbis.c
	$(CXX) $(CXXFLAGS) -o $@ $<

$(INTDIR)/stb_vorbis.o: src/stb_vorbis.c | $(INTDIR)
	$(CC) $(TARGET_FLAGS) -Isrc -o $@ $<

$(OUT_ELF): $(OBJS)
	$(LD) $(OBJS) -o $@ $(LDFLAGS)

eboot.bin: $(OUT_ELF)
	$(PKGTOOLS)/create-fself -in=$(OUT_ELF) -out=$(OUT_OELF) --eboot "eboot.bin" --paid 0x3100000000000001
	test -s eboot.bin

sce_sys/param.sfo: Makefile
	$(PKGTOOLS)/PkgTool.Core sfo_new $@
	$(PKGTOOLS)/PkgTool.Core sfo_setentry $@ APP_TYPE --type Integer --maxsize 4 --value 1
	$(PKGTOOLS)/PkgTool.Core sfo_setentry $@ APP_VER --type Utf8 --maxsize 8 --value '$(VERSION)'
	$(PKGTOOLS)/PkgTool.Core sfo_setentry $@ ATTRIBUTE --type Integer --maxsize 4 --value 0
	$(PKGTOOLS)/PkgTool.Core sfo_setentry $@ CATEGORY --type Utf8 --maxsize 4 --value 'gd'
	$(PKGTOOLS)/PkgTool.Core sfo_setentry $@ CONTENT_ID --type Utf8 --maxsize 48 --value '$(CONTENT_ID)'
	$(PKGTOOLS)/PkgTool.Core sfo_setentry $@ DOWNLOAD_DATA_SIZE --type Integer --maxsize 4 --value 0
	$(PKGTOOLS)/PkgTool.Core sfo_setentry $@ SYSTEM_VER --type Integer --maxsize 4 --value 0
	$(PKGTOOLS)/PkgTool.Core sfo_setentry $@ TITLE --type Utf8 --maxsize 128 --value '$(TITLE)'
	$(PKGTOOLS)/PkgTool.Core sfo_setentry $@ TITLE_ID --type Utf8 --maxsize 12 --value '$(TITLE_ID)'
	$(PKGTOOLS)/PkgTool.Core sfo_setentry $@ VERSION --type Utf8 --maxsize 8 --value '$(VERSION)'

pkg.gp4: eboot.bin sce_sys/param.sfo sce_sys/icon0.png sce_sys/about/right.sprx sce_module/libc.prx sce_module/libSceFios2.prx $(MENU_ASSETS)
	python3 tools/make_gp4.py --out $@ --content-id $(CONTENT_ID) $^

$(PKG): pkg.gp4
	$(PKGTOOLS)/PkgTool.Core pkg_build $< .
	test -s $@

clean:
	rm -rf $(INTDIR) eboot.bin pkg.gp4 sce_sys/param.sfo *.pkg

.PHONY: all clean
