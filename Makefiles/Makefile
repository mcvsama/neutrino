# vim:ts=4
# TODO allow using individual cpp flags .$1.per_source.cc

.PHONY: first
first: all

LC_ALL := C
export LC_ALL

# Override these variables in Makefile.local.

# Show verbose compilation commands?
MIHAU_CONFIG_VERBOSE			:= 0
# How many threads use for compilation?
MIHAU_CONFIG_THREADS			:= $(shell nproc)
# Architecture:
MIHAU_CONFIG_ARCH				:= $(shell uname -m)
# Enable profiling?
MIHAU_CONFIG_PROFILING			:= 0
# Debugging enabled?
MIHAU_CONFIG_DEBUG				:= 0
# Where Qt is installed?
MIHAU_CONFIG_QT_PREFIX			:= /usr
# Profile name:
MIHAU_CONFIG_PROFILE			?= release
# Version:
MIHAU_CONFIG_VERSION			:= 0.0
# Release version? (no asserts, etc).
MIHAU_CONFIG_RELEASE			:= 0
# Set to 1 to disable some UB-generating optimizations:
MIHAU_CONFIG_UB_OPTS_DISABLE	:= 0
# Sanitizers?
MIHAU_CONFIG_SANITIZERS			:=
MIHAU_CONFIG_NO_SANITIZE		:=
# Coverage?
MIHAU_CONFIG_COVERAGE			:= 0
MIHAU_CONFIG_BUILD_DIRECTORY	:= build

AR								:= ar
CXX								:= g++
DEPCC							:= $(CXX)
LD								:= $(CXX)
MOC								:= $(MIHAU_CONFIG_QT_PREFIX)/bin/moc
COMMON_CPP_FLAGS				:= -ftabstop=1

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
-include Makefile.local.$(MIHAU_CONFIG_PROFILE)

Makefile.local:
	false

Makefile.local.$(MIHAU_CONFIG_PROFILE):
	false

ifeq ($(MIHAU_CONFIG_DEBUG),1)
 COMMON_CPP_FLAGS += -O0 -ggdb3 -fno-inline -fno-eliminate-unused-debug-types -D_GLIBCXX_DEBUG
else
 COMMON_CPP_FLAGS += -O4 -DQT_NO_DEBUG
endif

ifneq ($(MIHAU_CONFIG_VERBOSE),1)
 MAKEFLAGS += --silent
endif

ifneq ($(MIHAU_CONFIG_THREADS),)
 MAKEFLAGS += -j$(MIHAU_CONFIG_THREADS)
endif

ifeq ($(MIHAU_CONFIG_PROFILING),1)
 COMMON_CPP_FLAGS += -pg -fno-omit-frame-pointer
 COMMON_LINKER_FLAGS += -pg
endif

ifeq ($(MIHAU_CONFIG_RELEASE),1)
 COMMON_CPP_FLAGS += -DBOOST_DISABLE_ASSERTS -DNDEBUG
endif

ifneq ($(MIHAU_CONFIG_PROFILE),)
 PROFILE_s = -$(MIHAU_CONFIG_PROFILE)
else
 PROFILE_s =
endif

# Undefined-Behaviour handling:
ifeq ($(MIHAU_CONFIG_UB_OPTS_DISABLE),1)
 COMMON_CPP_FLAGS += -fno-strict-overflow -fno-delete-null-pointer-checks -fno-strict-aliasing
endif

ifneq ($(MIHAU_CONFIG_SANITIZERS),)
 COMMON_CPP_FLAGS += $(foreach sanitizer,$(MIHAU_CONFIG_SANITIZERS), -fsanitize=$(sanitizer))
 COMMON_LINKER_FLAGS += $(foreach sanitizer,$(MIHAU_CONFIG_SANITIZERS), -fsanitize=$(sanitizer))
endif

ifneq ($(MIHAU_CONFIG_NO_SANITIZE),)
 COMMON_CPP_FLAGS += $(foreach sanitizer,$(MIHAU_CONFIG_NO_SANITIZE), -fno-sanitize=$(sanitizer))
 COMMON_LINKER_FLAGS += $(foreach sanitizer,$(MIHAU_CONFIG_NO_SANITIZE), -fno-sanitize=$(sanitizer))
endif

ifeq ($(MIHAU_CONFIG_COVERAGE),1)
 COMMON_CPP_FLAGS += -fprofile-arcs -ftest-coverage
 COMMON_LINKER_FLAGS += -fprofile-arcs -ftest-coverage
endif

################ Common functions ################

prepdir							= $(shell mkdir -p "$$(dirname '$(strip $1)')")
mkexecutable					= $(MODULE_PRODUCTS_DIRECTORY.$1)/$2

################ Modules and rules ################

include Makefile.sources.neutrino
include Makefile.sources

define MODULE_TEMPLATE =
# $1 is module name

MODULE_BUILD_DIRECTORY.$1		= $$(MIHAU_CONFIG_BUILD_DIRECTORY)/$$(MIHAU_CONFIG_ARCH)$$(PROFILE_s)/$1
MODULE_DEPS_DIRECTORY.$1		= $$(MODULE_BUILD_DIRECTORY.$1)/dependencies
MODULE_OBJS_DIRECTORY.$1		= $$(MODULE_BUILD_DIRECTORY.$1)/objects
MODULE_MOCS_DIRECTORY.$1		= $$(MODULE_BUILD_DIRECTORY.$1)/moc_sources
MODULE_PRODUCTS_DIRECTORY.$1	= $$(MODULE_BUILD_DIRECTORY.$1)/products

mkdeps.$1						= $$(foreach file,$$(filter %.cc, $$1), $$(MODULE_DEPS_DIRECTORY.$1)/$$(dir $$(file))$$(shell basename '$$(file)' $$(suffix $$(file))).dep)
mkmocs.$1						= $$(foreach file,$$1, $$(MODULE_MOCS_DIRECTORY.$1)/$$(dir $$(file))$$(shell basename '$$(file)' $$(suffix $$(file))).moc.cc)
mkobjs.$1						= $$(foreach file,$$(filter %.cc, $$1), $$(MODULE_OBJS_DIRECTORY.$1)/$$(dir $$(file))$$(shell basename '$$(file)' $$(suffix $$(file))).o)

MODULE_DEP_FILES.$1				= $$(call mkdeps.$1,$$(subst $$(MIHAU_VERSION_FILE),,$$(MODULE_SOURCES.$1)))
MODULE_MAIN_DEP_FILE.$1			= $$(MODULE_DEPS_DIRECTORY.$1)/Makefile.dep
# $(sort) is used to remove duplicates here:
MODULE_SOURCES_MOC.$1			= $$(sort $$(foreach product,$$(MIHAU.modules[$1].products),$$(MIHAU.modules[$1].products[$$(product)].sources_moc)))
# $(sort) is used to remove duplicates here:
MODULE_SOURCES.$1				= $$(sort $$(foreach product,$$(MIHAU.modules[$1].products),$$(MIHAU.modules[$1].products[$$(product)].sources))) $$(MODULE_SOURCES_MOC.$1)
MODULE_OBJECTS.$1				= $$(call mkobjs.$1,$$(MODULE_SOURCES.$1))
MODULE_PREPARED_MOC_SOURCES.$1	= $$(call mkmocs.$1,$$(MODULE_SOURCES_MOC.$1))
MODULE_PREPARED_MOC_OBJECTS.$1	= $$(call mkobjs.$1,$$(MODULE_PREPARED_MOC_SOURCES.$1))
#MODULE_MOC_OBJECTS.$1?
# This subst-hack is to hide warnings from Qt that happen from time to time and obscure compiler's output:
MODULE_CPP_FLAGS.$1				+= $$(COMMON_CPP_FLAGS)
MODULE_CPP_FLAGS.$1				+= $$(foreach flag,$$(MIHAU.modules[$1].cpp_flags), $$(flag))
MODULE_CPP_FLAGS.$1				+= $$(foreach define,$$(MIHAU.modules[$1].cpp_defines), -D$$(define))
MODULE_CPP_FLAGS.$1				+= $$(foreach include,$$(MIHAU.modules[$1].cpp_includes), -I$$(include))
MODULE_CPP_FLAGS.$1				+= $$(if $$(MIHAU.modules[$1].pkgconfigs),$$(subst -I,-isystem ,$$(shell pkg-config --cflags $$(MIHAU.modules[$1].pkgconfigs))))
MODULE_CPP_FLAGS.$1.static		:= $$(MODULE_CPP_FLAGS.$1)
MODULE_CPP_FLAGS.$1.selfdefine	+= -DCXXFLAGS='"$$(MODULE_CPP_FLAGS.$1.static)"'

.PHONY: info-$1
info-$1:
	@echo -n $$(_s); printf '%*s\n' "$$$$(tput cols)" '' | tr ' ' '='; echo -n $$(_l)
	@echo    $$(_s)'Module:              '$$(_l) $1
	@echo    $$(_s)'Products:            '$$(_l) $$(MIHAU.modules[$1].products)
	@echo    $$(_s)'Dependency Makefile: '$$(_l) $$(MODULE_MAIN_DEP_FILE.$1)
	@echo    $$(_s)'Compiler flags:      '$$(_l) $$(MIHAU.modules[$1].cpp_flags)
	@echo    $$(_s)'Defines:             '$$(_l) $$(MIHAU.modules[$1].cpp_defines)
	@echo    $$(_s)'Includes:            '$$(_l) $$(MIHAU.modules[$1].cpp_includes)
	@echo    $$(_s)'All compiler flags:  '$$(_l) $$(MODULE_CPP_FLAGS.$1)
	@echo    $$(_s)'Sources:             '$$(_l)
	@for file in $$(MODULE_SOURCES.$1); do echo '  '$$$$file; done
	@echo    $$(_s)'MOC sources:         '$$(_l)
	@for file in $$(MODULE_PREPARED_MOC_SOURCES.$1); do echo '  '$$$$file; done
	@echo    $$(_s)'Individual dependency files:    '$$(_l)
	@for file in $$(MODULE_DEP_FILES.$1); do echo '  '$$$$file; done

build-$1: $$(foreach product,$$(MIHAU.modules[$1].products),$$(call mkexecutable,$1,$$(product)))

clean::
	@echo $$(_s)'CLEAN     '$$(_l) module $1
	@rm -f $$(MODULE_OBJECTS.$1)
	@rm -f $$(MODULE_PREPARED_MOC_SOURCES.$1)

$$(MODULE_MAIN_DEP_FILE.$1): Makefile.sources $$(MODULE_DEP_FILES.$1)
	$$(call prepdir, $$@)
	@echo $$(_s)'DEP     '$$(_l) '$$@'
	@rm -f '$$@'
	@echo '# DO NOT EDIT!!! (all changed will be lost!)' >> '$$@'
	@echo '# This file has been generated automatically by Makefile from source files.' >> '$$@'
	@for file in $$(MODULE_DEP_FILES.$1); do \
		cat "$$$$file" >> '$$@'; \
	 done;
	@for file in $$(call mkmocs.$1,$$(SOURCES_MOC.$1)); do \
		echo "$$$$(dirname "$$$$file")/$$$$(basename "$$$$file" .cc).o: $$$$file" >> '$$@'; \
	 done;

$$(MODULE_DEPS_DIRECTORY.$1)/%.dep: %.cc %.h
	$$(call prepdir, $$@)
	@echo $$(_s)'DEP     '$$(_l) '$$@'
	$$(DEPCC) $$(MODULE_CPP_FLAGS.$1) -M -MP -MG -MQ "$$(MODULE_OBJS_DIRECTORY.$1)/$$$$(dirname '$$<')/$$$$(basename "$$<" $$$$(expr "$$<" : '.*\(\..*\)$$$$')).o" '$$<' > '$$@'

$$(MODULE_DEPS_DIRECTORY.$1)/%.dep: %.cc
	$$(call prepdir, $$@)
	@echo $$(_s)'DEP     '$$(_l) '$$@'
	$$(DEPCC) $$(MODULE_CPP_FLAGS.$1) -M -MP -MG -MQ "$$(MODULE_OBJS_DIRECTORY.$1)/$$$$(dirname "$$<")/$$$$(basename "$$<" $$$$(expr "$$<" : '.*\(\..*\)$$$$')).o" '$$<' > '$$@'

$$(MODULE_DEPS_DIRECTORY.$1)/%.dep: %.h
	$$(call prepdir, $$@)
	@echo $$(_s)'DEP     '$$(_l) '$$@'
	$$(DEPCC) $$(MODULE_CPP_FLAGS.$1) -M -MP -MG -MQ "$$(MODULE_OBJS_DIRECTORY.$1)/$$$$(dirname "$$<")/$$$$(basename "$$<" $$$$(expr "$$<" : '.*\(\..*\)$$$$')).o" '$$<' > '$$@'

$$(MODULE_DEPS_DIRECTORY.$1)/%.moc.dep: %.moc.cc
	$$(call prepdir, $$@)
	@echo $$(_s)'DEP     '$$(_l) '$$@'
	$$(DEPCC) $$(MODULE_CPP_FLAGS.$1) -M -MP -MG -MQ "$$$$(dirname "$$<")/$$$$(basename "$$<" $$$$(expr "$$<" : '.*\(\..*\)$$$$')).o" '$$<' > '$$@'

$$(MODULE_MOCS_DIRECTORY.$1)/%.moc.cc: %.h
	$$(call prepdir, $$@)
	@echo $$(_s)'MOC     '$$(_l) '$$@'
	$$(MOC) '$$<' -o '$$@'

$$(MODULE_BUILD_DIRECTORY.$1)/objects/%.moc.o: %.moc.cc
	$$(call prepdir, $$@)
	@echo $$(_s)'C++ MOC '$$(_l) '$$@'
	$$(CXX) $$(MODULE_CPP_FLAGS.$1) $$(MODULE_CPP_FLAGS.$1.selfdefine) -c '$$<' -o '$$@'

$$(MODULE_OBJECTS.$1):
	$$(call prepdir, $$@)
	@echo $$(_s)'C++     '$$(_l) '$$@'
	$$(CXX) $$(MODULE_CPP_FLAGS.$1) $$(MODULE_CPP_FLAGS.$1.selfdefine)  -c '$$<' -o '$$@'

$$(call mkobjs.$1, $$(MIHAU_VERSION_FILE)): $$(MIHAU_VERSION_FILE)

endef

define PRODUCT_TEMPLATE =
# $1 is module name
# $2 is product name

PRODUCT_FILENAME.$1.$2				= $$(call mkexecutable,$1,$2)

PRODUCT_BASE_SOURCES.$1.$2			= $$(MIHAU.modules[$1].products[$2].sources) $$(MIHAU.modules[$1].products[$2].sources_moc)
PRODUCT_PROCESSED_MOC_SOURCES.$1.$2	= $$(call mkmocs.$1,$$(MIHAU.modules[$1].products[$2].sources_moc))

PRODUCT_BASE_OBJECTS.$1.$2			= $$(call mkobjs.$1,$$(PRODUCT_BASE_SOURCES.$1.$2))
PRODUCT_PROCESSED_MOC_OBJECTS.$1.$2	= $$(call mkobjs.$1,$$(PRODUCT_PROCESSED_MOC_SOURCES.$1.$2))
PRODUCT_OBJECTS.$1.$2				= $$(PRODUCT_BASE_OBJECTS.$1.$2) $$(PRODUCT_PROCESSED_MOC_OBJECTS.$1.$2)

PRODUCT_LINKER_FLAGS.$1.$2			+= $$(COMMON_LINKER_FLAGS)
PRODUCT_LINKER_FLAGS.$1.$2			+= $$(foreach lib,$$(MIHAU.modules[$1].products[$2].linker_libraries),-l$$(lib))
PRODUCT_LINKER_FLAGS.$1.$2			+= $$(if $$(MIHAU.modules[$1].pkgconfigs),$$(shell pkg-config --libs $$(MIHAU.modules[$1].pkgconfigs)))
PRODUCT_LINKER_FLAGS.$1.$2			+= $$(MIHAU.modules[$1].products[$2].linker_flags)

.PHONY: info-$1-$2
info-$1-$2: info-$1
	@echo -n $$(_s); printf '%*s\n' "$$$$(tput cols)" '' | tr ' ' '-'; echo -n $$(_l)
	@echo    $$(_s)'Product:             '$$(_l) $2 [module $1]
	@echo    $$(_s)'Executable:          '$$(_l) $$(PRODUCT_FILENAME.$1.$2)
	@echo    $$(_s)'Linker flags:        '$$(_l) $$(MIHAU.modules[$1].products[$2].linker_flags)
	@echo    $$(_s)'Linker libraries:    '$$(_l) $$(MIHAU.modules[$1].products[$2].linker_libraries)
	@echo    $$(_s)'All linker flags:    '$$(_l) $$(PRODUCT_LINKER_FLAGS.$1.$2)
	@echo    $$(_s)'Base sources:'$$(_l)
	@for file in $$(PRODUCT_BASE_SOURCES.$1.$2); do echo '  '$$$$file; done
	@echo    $$(_s)'Processed MOC sources:'$$(_l)
	@for file in $$(PRODUCT_PROCESSED_MOC_SOURCES.$1.$2); do echo '  '$$$$file; done
	@echo    $$(_s)'Base objects:'$$(_l)
	@for file in $$(PRODUCT_BASE_OBJECTS.$1.$2); do echo '  '$$$$file; done
	@echo    $$(_s)'MOC objects:'$$(_l)
	@for file in $$(PRODUCT_PROCESSED_MOC_OBJECTS.$1.$2); do echo '  '$$$$file; done

build-$1-$2: $$(PRODUCT_FILENAME.$1.$2)

info:: info-$1-$2

$$(PRODUCT_FILENAME.$1.$2): $$(PRODUCT_OBJECTS.$1.$2) $$(PRODUCT_PROCESSED_MOC_OBJECTS.$1.$2)
	$$(call prepdir, $$@)
	@echo $$(_s)'LINK    '$$(_l) '$$@'
	$$(LD) -o '$$@' $$^ $$(PRODUCT_LINKER_FLAGS.$1.$2)

endef

# Evaluate templates:
$(foreach module,$(MIHAU.modules),$(eval $(call MODULE_TEMPLATE,$(module))))
$(foreach module,$(MIHAU.modules),$(foreach product,$(MIHAU.modules[$(module)].products),$(eval $(call PRODUCT_TEMPLATE,$(module),$(product)))))

ALL_PRODUCTS		+= $(foreach module,$(MIHAU.modules),$(foreach product,$(MIHAU.modules[$(module)].products),$(PRODUCT_FILENAME.$(module).$(product))))
ALL_MAIN_DEP_FILES	+= $(foreach module,$(MIHAU.modules),$(MODULE_MAIN_DEP_FILE.$(module)))
# $(sort) is used to remove duplicates here:
ALL_DEP_FILES		+= $(sort $(foreach module,$(MIHAU.modules),$(MODULE_DEP_FILES.$(module))))

################ Commit ID support ################

ifneq ($(MIHAU_VERSION_FILE),)
GIT_AVAILABLE		:= $(shell which git 1>/dev/null && echo OK)

ifeq ($(GIT_AVAILABLE),OK)
COMMIT				= $(shell cat .commit 2>/dev/null)
COMMIT_REAL			= $(shell git rev-parse HEAD 2>/dev/null)
BRANCH				= $(shell cat .branch 2>/dev/null)
BRANCH_REAL			= $(shell git branch --no-color 2>/dev/null | sed -e '/^[^*]/d' -e 's/* \(.*\)/\1/')

ifeq ($(COMMIT),$(COMMIT_REAL))
COMMIT_FILE			=
else
COMMIT_FILE			= .commit
endif

ifeq ($(BRANCH),$(BRANCH_REAL))
BRANCH_FILE			=
else
BRANCH_FILE			= .branch
endif

.PHONY: .commit
.commit:
	@if [ '$(COMMIT)' != '$(COMMIT_REAL)' ]; then \
		echo $(_s)'Saving commit ID'$(_l); \
		echo '$(COMMIT_REAL)' >'$@'; \
	 fi

.PHONY: .branch
.branch:
	@if [ '$(BRANCH)' != '$(BRANCH_REAL)' ]; then \
		echo $(_s)'Saving branch name'$(_l); \
		echo '$(BRANCH_REAL)' >'$@'; \
	 fi

$(MIHAU_VERSION_FILE): $(COMMIT_FILE) $(BRANCH_FILE)
	@echo '// Do not edit - automatically generated.' >'$@'
	@echo 'namespace neutrino::version {' >>'$@'
	@echo '	const char* commit = "$(COMMIT_REAL)";' >>'$@'
	@echo '	const char* branch = "$(BRANCH_REAL)";' >>'$@'
	@echo '	const char* version = "$(MIHAU_CONFIG_VERSION)";' >>'$@'
	@echo '}' >>'$@'

endif # GIT_AVAILABLE == OK
endif # MIHAU_VERSION_FILE != ""

################ Common rules ################

.PHONY: all
all: $(ALL_MAIN_DEP_FILES) $(ALL_PRODUCTS)

.PHONY: dep
dep: $(ALL_DEP_FILES)

.PHONY: help
help:
	@echo 'Usage:'
	@echo $(_s)'make all                        '$(_l)'Generate all products from all modules:'
	@for product in $(ALL_PRODUCTS); do echo -e "                                    • $$product"; done
	@echo $(_s)'make dep                        '$(_l)'Generate dependencies.'
	@echo $(_s)'make clean                      '$(_l)'Clean source tree and dep files'
	@echo $(_s)'make depclean                   '$(_l)'Clean dependency files'
	@echo $(_s)'make info                       '$(_l)'Show info about configured modules and products'
	@echo $(_s)'make info-<module>              '$(_l)'Show info about given module'
	@echo $(_s)'make info-<module>-<product>    '$(_l)'Show info about given module and its product'
	@echo $(_s)'make build-<module>             '$(_l)'Build all products of the module'
	@echo $(_s)'make build-<module>-<product>   '$(_l)'Build given product of the module'
	@echo $(_s)'make help                       '$(_l)'Show this help.'
	@echo
	@echo 'Available info targets:'
	@echo $(foreach module,$(MIHAU.modules),$(_s)'make info-$(module)'$(_l) $(foreach product,$(MIHAU.modules[$(module)].products),$(_s)'make info-$(module)-$(product)'$(_l))) | sed -E 's/(make [^ ]+) /\1\n/g'
	@echo
	@echo 'Available build targets:'
	@echo $(foreach module,$(MIHAU.modules),$(_s)'make build-$(module)'$(_l) $(foreach product,$(MIHAU.modules[$(module)].products),$(_s)'make build-$(module)-$(product)'$(_l))) | sed -E 's/(make [^ ]+) /\1\n/g'

.PHONY: clean
clean:: depclean

.PHONY: depclean
depclean:
	@rm -f $(ALL_DEP_FILES)
	@rm -f $(ALL_MAIN_DEP_FILES)

-include $(ALL_MAIN_DEP_FILES)

