# vim:ts=4

first: all

LC_ALL := C
export LC_ALL

# Override these variables in Makefile.local.

MAKE_CONFIG_PROJECT_NAME		:= project
# Show verbose compilation commands?
MAKE_CONFIG_VERBOSE				:= 0
# How many threads use for compilation?
MAKE_CONFIG_THREADS				:= $(shell nproc)
# Architecture:
MAKE_CONFIG_ARCH				:= $(shell uname -m)
# Enable profiling?
MAKE_CONFIG_PROFILING			:= 0
# Debugging enabled?
MAKE_CONFIG_DEBUG				:= 0
# Where Qt is installed?
MAKE_CONFIG_QT_PREFIX			:= /usr
# Profile name:
MAKE_CONFIG_PROFILE				?= release
# Version:
MAKE_CONFIG_VERSION				:= 0.0
# Release version? (no asserts, etc).
MAKE_CONFIG_RELEASE				:= 0
# Set to 1 to disable some UB-generating optimizations:
MAKE_CONFIG_UB_OPTS_DISABLE		:= 0
# Sanitizers?
MAKE_CONFIG_SANITIZERS			:=
MAKE_CONFIG_NO_SANITIZE			:=
# Coverage?
MAKE_CONFIG_COVERAGE			:= 0

AR								:= ar
CXX								:= g++
DEPCC							:= $(CXX)
LD								:= $(CXX)
MOC								:= $(MAKE_CONFIG_QT_PREFIX)/bin/moc

.PHONY: first all dep help clean depclean release doc

################ Helpers ################

ifeq ($(shell tput colors),-1)
 _s = ""
 _l = ""
else
 _s = "[1;34m"
 _l = "[0m"
endif

################ Makefiles ################

-include Makefile.local
-include Makefile.local.$(MAKE_CONFIG_PROFILE)

Makefile.local:
	false

Makefile.local.$(MAKE_CONFIG_PROFILE):
	false

ifeq ($(MAKE_CONFIG_DEBUG),1)
 CXXFLAGS += -O0 -ggdb3 -fno-inline -fno-eliminate-unused-debug-types -D_GLIBCXX_DEBUG
else
 CXXFLAGS += -O4 -DQT_NO_DEBUG
endif

ifneq ($(MAKE_CONFIG_VERBOSE),1)
 MAKEFLAGS += --silent
endif

ifneq ($(MAKE_CONFIG_THREADS),)
 MAKEFLAGS += -j$(MAKE_CONFIG_THREADS)
endif

ifeq ($(MAKE_CONFIG_PROFILING),1)
 CXXFLAGS += -pg -fno-omit-frame-pointer
 LDFLAGS += -pg
endif

ifeq ($(MAKE_CONFIG_RELEASE),1)
 CXXFLAGS += -DBOOST_DISABLE_ASSERTS -DNDEBUG
endif

ifneq ($(MAKE_CONFIG_PROFILE),)
 PROFILE_s = -$(MAKE_CONFIG_PROFILE)
else
 PROFILE_s =
endif

# Undefined-Behaviour handling:
ifeq ($(MAKE_CONFIG_UB_OPTS_DISABLE),1)
 CXXFLAGS += -fno-strict-overflow -fno-delete-null-pointer-checks -fno-strict-aliasing
endif

ifneq ($(MAKE_CONFIG_SANITIZERS),)
 CXXFLAGS += $(foreach sanitizer,$(MAKE_CONFIG_SANITIZERS), -fsanitize=$(sanitizer))
 LDFLAGS += $(foreach sanitizer,$(MAKE_CONFIG_SANITIZERS), -fsanitize=$(sanitizer))
endif

ifneq ($(MAKE_CONFIG_NO_SANITIZE),)
 CXXFLAGS += $(foreach sanitizer,$(MAKE_CONFIG_NO_SANITIZE), -fno-sanitize=$(sanitizer))
 LDFLAGS += $(foreach sanitizer,$(MAKE_CONFIG_NO_SANITIZE), -fno-sanitize=$(sanitizer))
endif

ifeq ($(MAKE_CONFIG_COVERAGE),1)
 CXXFLAGS += -fprofile-arcs -ftest-coverage
 LDFLAGS += -fprofile-arcs -ftest-coverage
endif

distdir				:= build/$(MAKE_CONFIG_ARCH)$(PROFILE_s)
depsdir				= $(distdir)/deps
objsdir				= $(distdir)/objects
mocsdir				= $(distdir)/mocs
targetsdir			= $(distdir)/targets
prepdir				= $(shell mkdir -p "$$(dirname "$(strip $(1))")")
mkmocs				= $(foreach file,$(1), $(mocsdir)/$(dir $(file))$(shell basename '$(file)' $(suffix $(file))).moc.cc)
mkdeps				= $(foreach file,$(filter %.cc, $(1)), $(depsdir)/$(dir $(file))$(shell basename '$(file)' $(suffix $(file))).dep)
mkobjs				= $(foreach file,$(filter %.cc, $(1)), $(objsdir)/$(dir $(file))$(shell basename '$(file)' $(suffix $(file))).o)
mktarget			= $(targetsdir)/$(1)
calculate_sources	= $(PROJECTS.$(1).files) $(call mkmocs,$(PROJECTS.$(1).files_moc))
calculate_objects	= $(call mkobjs,$(call calculate_sources,$(1)))

include Makefile.sources.neutrino
include Makefile.sources

SOURCES				+= $(foreach project,$(PROJECTS), $(call calculate_sources,$(project)))
TARGETS				+= $(foreach project,$(PROJECTS), $(if $(PROJECTS.$(project).executable),$(call mktarget,$(PROJECTS.$(project).executable))))

define TARGET_TEMPLATE =
CXXFLAGS			+= $$(foreach define,$$(PROJECTS.$(1).defines), -D$$(define))
CXXFLAGS			+= $$(foreach include,$$(PROJECTS.$(1).includes), -I$$(include))
# This subst-hack is to hide warnings from Qt that happen from time to time and obscure compiler's output:
CXXFLAGS			+= $$(if $$(PROJECTS.$(1).pkgconfigs),$$(subst -I,-isystem ,$$(shell pkg-config --cflags $$(PROJECTS.$(1).pkgconfigs))))
CXXFLAGS			+= $$(PROJECTS.$(1).cxxflags)
LDFLAGS				+= $$(foreach libdir,$$(PROJECTS.$(1).libdirs), -L$$(libdir))
LDFLAGS				+= $$(foreach lib,$$(PROJECTS.$(1).libraries), -l$$(lib))
LDFLAGS				+= $$(if $$(PROJECTS.$(1).pkgconfigs),$$(shell pkg-config --libs $$(PROJECTS.$(1).pkgconfigs)))
LDFLAGS				+= $$(PROJECTS.$(1).ldflags)
$$(targetsdir)/$$(PROJECTS.$(1).executable): $$(call calculate_objects,$(1))
endef

ifeq ($(CXX),clang++)
CLANG_NO_WARNINGS	= mismatched-tags disabled-macro-expansion c++98-compat-pedantic documentation-unknown-command
CXXFLAGS			+= $(foreach warning,$(CLANG_NO_WARNINGS), -Wno-$(warning))
endif

CXXFLAGS_s			:= $(CXXFLAGS)
CXXFLAGS			+= -DCXXFLAGS='"$(CXXFLAGS_s)"'
SOFLAGS				+=

$(foreach project,$(PROJECTS),$(eval $(call TARGET_TEMPLATE,$(project))))

################ Commit ID support ################

ifneq ($(VERSION_FILE),)
GIT_AVAILABLE	:= $(shell which git 1>/dev/null && echo OK)

ifeq ($(GIT_AVAILABLE),OK)
COMMIT			= $(shell cat .commit 2>/dev/null)
COMMIT_REAL		= $(shell git rev-parse HEAD 2>/dev/null)
BRANCH			= $(shell cat .branch 2>/dev/null)
BRANCH_REAL		= $(shell git branch --no-color 2>/dev/null | sed -e '/^[^*]/d' -e 's/* \(.*\)/\1/')

ifeq ($(COMMIT),$(COMMIT_REAL))
COMMIT_FILE		=
else
COMMIT_FILE		= .commit
endif

ifeq ($(BRANCH),$(BRANCH_REAL))
BRANCH_FILE		=
else
BRANCH_FILE		= .branch
endif

.PHONY: .commit .branch

.commit:
	@if [ "$(COMMIT)" != "$(COMMIT_REAL)" ]; then \
		echo " Saving commit ID"; \
		echo "$(COMMIT_REAL)" >$@; \
	 fi

.branch:
	@if [ "$(BRANCH)" != "$(BRANCH_REAL)" ]; then \
		echo " Saving branch name"; \
		echo "$(BRANCH_REAL)" >$@; \
	 fi

$(VERSION_FILE): $(COMMIT_FILE) $(BRANCH_FILE)
	@echo "// Do not edit - automatically generated." >$@
	@echo "namespace neutrino::version {" >>$@
	@echo "	const char* commit = \"$(COMMIT_REAL)\";" >>$@
	@echo "	const char* branch = \"$(BRANCH_REAL)\";" >>$@
	@echo "	const char* version = \"$(MAKE_CONFIG_VERSION)\";" >>$@
	@echo "}" >>$@

$(call mkobjs, $(VERSION_FILE)): $(VERSION_FILE)
endif # GIT_AVAILABLE == OK
endif # VERSION_FILE != ""

################ Rules ################

DEPFILES := $(call mkdeps, $(subst $(VERSION_FILE),,$(SOURCES)))
MAINDEPFILE := $(depsdir)/Makefile.dep

all: $(MAINDEPFILE) $(DEPFILES) $(TARGETS)

dep: $(DEPFILES)

help:
	@echo 'Available targets:'
	@echo '  all        Compile program.'
	@echo '  dep        Generate dependencies.'
	@echo '  clean      Clean source tree and dep files'
	@echo '  depclean   Clean dependency files'
	@echo '  help       Show this help.'

clean: depclean
	@rm -f $(call mkmocs,$(PROJECTS.$(1).files_moc))
	@rm -f $(call mkobjs,$(SOURCES))

depclean:
	@rm -f $(DEPFILES)
	@rm -f $(MAINDEPFILE)

doc:
	@cd doc && doxygen doxygen-conf/doxygen.conf

$(MAINDEPFILE): Makefile.sources $(DEPFILES)
	$(call prepdir, $@)
	@echo $(_s) "DEP     " $(_l) $@
	@rm -f $@
	@echo "# DO NOT EDIT!!! (all changed will be lost!)" >> $@
	@echo "# This file has been generated automatically by Makefile from source files." >> $@
	@for file in $(DEPFILES); do \
		cat "$$file" >> $@; \
	 done;
	@for file in $(call mkmocs,$(PROJECTS.$(1).files_moc)); do \
		echo "$$(dirname "$$file")/$$(basename "$$file" .cc).o: $$file" >> $@; \
	 done;

$(depsdir)/%.dep: %.cc %.h
	$(call prepdir, $@)
	@echo $(_s) "DEP     " $(_l) $@
	$(DEPCC) $(CXXFLAGS) -M -MP -MG -MQ "$(objsdir)/$$(dirname "$<")/$$(basename "$<" $$(expr "$<" : '.*\(\..*\)$$')).o" $< > $@

$(depsdir)/%.dep: %.cc
	$(call prepdir, $@)
	@echo $(_s) "DEP     " $(_l) $@
	$(DEPCC) $(CXXFLAGS) -M -MP -MG -MQ "$(objsdir)/$$(dirname "$<")/$$(basename "$<" $$(expr "$<" : '.*\(\..*\)$$')).o" $< > $@

$(depsdir)/%.dep: %.h
	$(call prepdir, $@)
	@echo $(_s) "DEP     " $(_l) $@
	$(DEPCC) $(CXXFLAGS) -M -MP -MG -MQ "$(objsdir)/$$(dirname "$<")/$$(basename "$<" $$(expr "$<" : '.*\(\..*\)$$')).o" $< > $@

$(depsdir)/%.moc.dep: %.moc.cc
	$(call prepdir, $@)
	@echo $(_s) "DEP     " $(_l) $@
	$(DEPCC) $(CXXFLAGS) -M -MP -MG -MQ "$$(dirname "$<")/$$(basename "$<" $$(expr "$<" : '.*\(\..*\)$$')).o" $< > $@

%.o:
	$(call prepdir, $@)
	@echo $(_s) "C++     " $(_l) $@
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(distdir)/objects/%.moc.o: %.moc.cc
	$(call prepdir, $@)
	@echo $(_s) "C++ MOC " $(_l) $@
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.a:
	$(call prepdir, $@)
	@echo $(_s) "AR      " $(_l) $@
	@rm -f $@
	$(AR) cr $@ $^

%.so:
	$(call prepdir, $@)
	@echo $(_s) "LD SO   " $(_l) $@
	$(CXX) $(SOFLAGS) -shared $^ -o $@

$(mocsdir)/%.moc.cc: %.h
	$(call prepdir, $@)
	@echo $(_s) "MOC     " $(_l) $@
	$(MOC) $< -o $@

$(TARGETS):
	$(call prepdir, $@)
	@echo $(_s) "LD      " $(_l) $@
	$(LD) -o $@ $^ $(LDFLAGS)

-include $(MAINDEPFILE)

