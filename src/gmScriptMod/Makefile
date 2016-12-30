#
# GMSM_QMM Makefile
# Adapted from the Quake3 Makefile from http://tremulous.net/Q3A-source/
#

include Config.mak

CC=g++

BASE_CFLAGS=-pipe -m32

BROOT=linux
BR=$(BROOT)/release
BD=$(BROOT)/debug

B_Q3A=
B_RTCWMP=_rtcwmp
B_JKA=_jka
B_JK2=_jk2
B_RTCWSP=_rtcwsp
B_RTCWET=_rtcwet

OBJR_Q3A=$(SRC_FILES:%.cpp=$(BR)$(B_Q3A)/%.o)
OBJD_Q3A=$(SRC_FILES:%.cpp=$(BD)$(B_Q3A)/%.o)
OBJR_RTCWMP=$(SRC_FILES:%.cpp=$(BR)$(B_RTCWMP)/%.o)
OBJD_RTCWMP=$(SRC_FILES:%.cpp=$(BD)$(B_RTCWMP)/%.o)
OBJR_JKA=$(SRC_FILES:%.cpp=$(BR)$(B_JKA)/%.o)
OBJD_JKA=$(SRC_FILES:%.cpp=$(BD)$(B_JKA)/%.o)
OBJR_JK2=$(SRC_FILES:%.cpp=$(BR)$(B_JK2)/%.o)
OBJD_JK2=$(SRC_FILES:%.cpp=$(BD)$(B_JK2)/%.o)
OBJR_RTCWSP=$(SRC_FILES:%.cpp=$(BR)$(B_RTCWSP)/%.o)
OBJD_RTCWSP=$(SRC_FILES:%.cpp=$(BD)$(B_RTCWSP)/%.o)
OBJR_RTCWET=$(SRC_FILES:%.cpp=$(BR)$(B_RTCWET)/%.o)
OBJD_RTCWET=$(SRC_FILES:%.cpp=$(BD)$(B_RTCWET)/%.o)

DEBUG_CFLAGS=$(BASE_CFLAGS) -g -pg
RELEASE_CFLAGS=$(BASE_CFLAGS) -DNDEBUG -O2 -fomit-frame-pointer -pipe -ffast-math -falign-loops=2 -falign-jumps=2 -falign-functions=2 -fno-strict-aliasing -fstrength-reduce 

SHLIBCFLAGS=-fPIC
SHLIBLDFLAGS=-shared -m32

help:
	@echo GMSM_QMM supports the following make rules:
	@echo all - builds release versions for all supported games
	@echo all_d - builds debug versions for all supported games
	@echo rtcwet - builds release version for RtCW: Enemy Territory
	@echo rtcwmp - builds debug version for RtCW: Enemy Territory
	@echo clean - cleans all output files and directories
	
all: rtcwet rtcwmp

all_d: rtcwet_d

rtcwet: $(BR)$(B_RTCWET)/$(BINARY).so

rtcwet_d: $(BD)$(B_RTCWET)/$(BINARY).so

rtcwmp: $(BR)$(B_RTCWMP)/$(BINARY).so

#return to castle wolfenstein: enemy territory
$(BR)$(B_RTCWET)/$(BINARY).so: $(BR)$(B_RTCWET) $(OBJR_RTCWET)
	$(CC) $(RELEASE_CFLAGS) $(SHLIBLDFLAGS) -o $@ $(OBJR_RTCWET)
  
$(BD)$(B_RTCWET)/$(BINARY).so: $(BD)$(B_RTCWET) $(OBJD_RTCWET)
	$(CC) $(DEBUG_CFLAGS) $(SHLIBLDFLAGS) -o $@ $(OBJD_RTCWET)

$(BR)$(B_RTCWMP)/$(BINARY).so: $(BR)$(B_RTCWMP) $(OBJR_RTCWMP)	$(CC) $(RELEASE_CFLAGS) $(SHLIBLDFLAGS) -o $@ $(OBJR_RTCWMP)
   
$(BD)$(B_RTCWMP)/$(BINARY).so: $(BD)$(B_RTCWMP) $(OBJD_RTCWMP)	$(CC) $(DEBUG_CFLAGS) $(SHLIBLDFLAGS) -o $@ $(OBJD_RTCWMP)

$(BR)$(B_RTCWMP)/%.o: %.cpp $(HDR_FILES)
	$(CC) $(RELEASE_CFLAGS) $(RTCWMP_FLAGS) $(SHLIBCFLAGS) -o $@ -c $<



$(BR)$(B_RTCWET)/%.o: %.cpp $(HDR_FILES)
	$(CC) $(RELEASE_CFLAGS) $(RTCWET_FLAGS) $(SHLIBCFLAGS) -o $@ -c $<
  
$(BD)$(B_RTCWET)/%.o: %.cpp $(HDR_FILES)
	$(CC) $(DEBUG_CFLAGS) $(RTCWET_FLAGS) $(SHLIBCFLAGS) -o $@ -c $<
	
$(BR)$(B_RTCWET):
	@if [ ! -d $(BROOT) ];then mkdir $(BROOT);fi
	@if [ ! -d $(@) ];then mkdir $@;fi

$(BD)$(B_RTCWET):
	@if [ ! -d $(BROOT) ];then mkdir $(BROOT);fi
	@if [ ! -d $(@) ];then mkdir $@;fi

$(BD)$(B_RTCWMP):
	@if [ ! -d $(BROOT) ];then mkdir $(BROOT);fi
	@if [ ! -d $(@) ];then mkdir $@;fi


	
clean:
	@rm -rf $(BD)$(B_RTCWET) $(BR)$(B_RTCWET)
