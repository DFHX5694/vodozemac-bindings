all: dynamic-lib

.PHONY: all dynamic-lib test install

BUILD_TYPE := debug
ifeq ($(BUILD_TYPE), release)
CARGO_ARGS = --release
else
CARGO_ARGS =
endif

VERSION = 0.1.0
SOVERSION = 1

TARGET_DIR = ../target/$(BUILD_TYPE)
CXXBRIDGE_DIR = $(TARGET_DIR)/../cxxbridge
STATIC_LIB = $(TARGET_DIR)/libvodozemac.a
SHARED_LIB_NAME = libvodozemac.so
SHARED_LIB_PREFIX = $(TARGET_DIR)/$(SHARED_LIB_NAME)
SHARED_LIBS = $(SHARED_LIB_PREFIX) $(SHARED_LIB_PREFIX).$(VERSION) $(SHARED_LIB_PREFIX).$(SOVERSION)
HEADER = $(CXXBRIDGE_DIR)/vodozemac/src/lib.rs.h
SOURCES = src/account.rs src/group_sessions.rs src/lib.rs src/sas.rs src/session.rs src/types.rs src/maybe.rs build.rs vodozemac_maybe_derive/src/lib.rs

AR ?= ar
CXX ?= c++
LN ?= ln
INSTALL ?= install
CP ?= cp

# Packagers may need to override them all.
PREFIX = /usr/local
DESTDIR =
LIBDIR = $(PREFIX)/lib
INCLUDEDIR = $(PREFIX)/include

src/lib.rs: src/lib.rs.in inject-maybe
	./inject-maybe < src/lib.rs.in > src/lib.rs

$(STATIC_LIB): $(SOURCES)
	cargo build $(CARGO_ARGS)

dynamic-lib: $(SHARED_LIB_PREFIX)

$(SHARED_LIB_PREFIX).$(SOVERSION): $(SHARED_LIB_PREFIX).$(VERSION)
	$(LN) -sfv libvodozemac.so.$(VERSION) $(SHARED_LIB_PREFIX).$(SOVERSION)

$(SHARED_LIB_PREFIX): $(SHARED_LIB_PREFIX).$(SOVERSION)
	$(LN) -sfv libvodozemac.so.$(SOVERSION) $(SHARED_LIB_PREFIX)

$(SHARED_LIB_PREFIX).$(VERSION): $(STATIC_LIB)
	rm -rf objects && \
	mkdir objects && \
	cd objects && \
	$(AR) x ../$(STATIC_LIB) && \
	cd .. && \
	$(CXX) objects/*.o -shared -fPIC -Wl,-soname=$(SHARED_LIB_NAME) -o $(SHARED_LIB_PREFIX).$(VERSION)

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
	$(LN) -sfv $(SHARED_LIB_NAME).$(VERSION) $(DESTDIR)$(LIBDIR)/$(SHARED_LIB_NAME).$(SOVERSION)
	$(LN) -sfv $(SHARED_LIB_NAME).$(SOVERSION) $(DESTDIR)$(LIBDIR)/$(SHARED_LIB_NAME)
	$(CP) $(HEADER) $(DESTDIR)$(INCLUDEDIR)/vodozemac.h
