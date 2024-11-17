
.PHONY: all dynamic-lib test install

BUILD_TYPE := debug
ifeq ($(BUILD_TYPE), release)
CARGO_ARGS = --release
else
CARGO_ARGS =
endif

VERSION = 0.2.0
SOVERSION = 2

TARGET_DIR = ../target/$(BUILD_TYPE)
CXXBRIDGE_DIR = $(TARGET_DIR)/../cxxbridge
STATIC_LIB = $(TARGET_DIR)/libvodozemac.a
ifeq ($(OS), Windows_NT)
SHARED_LIB_NAME = libvodozemac.dll
else
SHARED_LIB_NAME = libvodozemac.so
endif
SHARED_LIB_PREFIX = $(TARGET_DIR)/$(SHARED_LIB_NAME)
SHARED_LIBS = $(SHARED_LIB_PREFIX) $(SHARED_LIB_PREFIX).$(VERSION) $(SHARED_LIB_PREFIX).$(SOVERSION)

# The list of libraries are obtained by running
# `cargo rustc -- --print native-static-libs`.
# Unfortunately, the output format is really hostile to automatically generating the list on the fly.
ifeq ($(OS), Windows_NT)
SHARED_LIB_LDFLAGS = -lbcrypt -ladvapi32 -lstdc++ -lkernel32 -ladvapi32 -lntdll -luserenv -lws2_32 -lkernel32 -Wl,--out-implib,$(SHARED_LIB_PREFIX).a
else
SHARED_LIB_LDFLAGS = -lstdc++ -lgcc_s -lutil -lrt -lpthread -lm -ldl -lc
endif

HEADER = $(CXXBRIDGE_DIR)/vodozemac/src/lib.rs.h
SOURCES = src/account.rs src/group_sessions.rs src/lib.rs src/sas.rs src/session.rs src/types.rs src/maybe.rs build.rs vodozemac_maybe_derive/src/lib.rs

all: dynamic-lib

CXX ?= c++
LN ?= ln
INSTALL ?= install
CP ?= cp
PERL ?= perl
CARGO ?= cargo

# Packagers may need to override them all.
PREFIX = /usr/local
DESTDIR =
LIBDIR = $(PREFIX)/lib
INCLUDEDIR = $(PREFIX)/include

src/lib.rs: src/lib.rs.in inject-maybe
	$(PERL) ./inject-maybe < src/lib.rs.in > src/lib.rs

$(STATIC_LIB): $(SOURCES)
	$(CARGO) build $(CARGO_ARGS)

dynamic-lib: $(SHARED_LIB_PREFIX)

$(SHARED_LIB_PREFIX).$(SOVERSION): $(SHARED_LIB_PREFIX).$(VERSION)
	$(LN) -sfv $(SHARED_LIB_NAME).$(VERSION) $(SHARED_LIB_PREFIX).$(SOVERSION)

$(SHARED_LIB_PREFIX): $(SHARED_LIB_PREFIX).$(SOVERSION)
	$(LN) -sfv $(SHARED_LIB_NAME).$(SOVERSION) $(SHARED_LIB_PREFIX)

$(SHARED_LIB_PREFIX).$(VERSION): $(STATIC_LIB)
	$(CXX) -Wl,--whole-archive $(STATIC_LIB) -Wl,--no-whole-archive $(SHARED_LIB_LDFLAGS) -shared -fPIC -Wl,-soname=$(SHARED_LIB_NAME) -o $(SHARED_LIB_PREFIX).$(VERSION)

TEST_CXXFLAGS = -Wall -g -std=c++17 -I $(CXXBRIDGE_DIR)
TEST_LDFLAGS = -lCatch2Main -lCatch2 -lolm -lboost_json \
 -L $(TARGET_DIR) -l vodozemac -ldl -Wl,-rpath,$(TARGET_DIR) \
 -L /usr/local/lib -Wl,-rpath,/usr/local/lib

TEST_DEPS = $(SHARED_LIB_PREFIX).$(VERSION) tests/util.hpp
TESTS_EXCEPT = account-test session-test sas-test group-session-test
TESTS_NOEXCEPT = $(TESTS_EXCEPT:-test=-noexcept-test)
TESTS = $(TESTS_EXCEPT) $(TESTS_NOEXCEPT)

%-test: tests/%.cpp $(TEST_DEPS)
	$(CXX) $(TEST_CXXFLAGS) -DVODOZEMAC_TEST_EXCEPTIONS=1 -fexceptions $< $(TEST_LDFLAGS) -o $@

%-noexcept-test: tests/%.cpp $(TEST_DEPS)
	$(CXX) $(TEST_CXXFLAGS) -DVODOZEMAC_TEST_EXCEPTIONS=0 -fno-exceptions $< $(TEST_LDFLAGS) -o $@

test: $(TESTS)
	for i in $^; do ./$$i || exit 1; done

install:
	$(INSTALL) -d $(DESTDIR)$(INCLUDEDIR) $(DESTDIR)$(LIBDIR)
	$(INSTALL) -m644 $(STATIC_LIB) $(DESTDIR)$(LIBDIR)
	$(INSTALL) -m755 $(SHARED_LIB_PREFIX).$(VERSION) $(DESTDIR)$(LIBDIR)
ifeq ($(OS), Windows_NT)
	$(INSTALL) -m755 $(SHARED_LIB_PREFIX).a $(DESTDIR)$(LIBDIR)
endif
	$(LN) -sfv $(SHARED_LIB_NAME).$(VERSION) $(DESTDIR)$(LIBDIR)/$(SHARED_LIB_NAME).$(SOVERSION)
	$(LN) -sfv $(SHARED_LIB_NAME).$(SOVERSION) $(DESTDIR)$(LIBDIR)/$(SHARED_LIB_NAME)
	$(CP) $(HEADER) $(DESTDIR)$(INCLUDEDIR)/vodozemac.h
