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
SOURCES = src/account.rs src/group_sessions.rs src/lib.rs src/sas.rs src/session.rs src/types.rs build.rs

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
	$(CXX) objects/*.o -shared -Wl,-soname=$(SHARED_LIB_NAME) -o $(SHARED_LIB_PREFIX).$(VERSION)

TEST_CXXFLAGS = -Wall -g -pthread -std=c++17 -lgtest_main \
 -lgtest -lolm -lboost_json -lpthread -I $(CXXBRIDGE_DIR) \
 -L $(TARGET_DIR) -l vodozemac -ldl -Wl,-rpath,$(TARGET_DIR)

TEST_DEPS = $(SHARED_LIB_PREFIX).$(VERSION) tests/util.hpp

account-test: $(TEST_DEPS) tests/account.cpp
	$(CXX) $(TEST_CXXFLAGS) \
	tests/account.cpp -o account-test


session-test: $(TEST_DEPS) tests/session.cpp
	$(CXX) $(TEST_CXXFLAGS) \
	tests/session.cpp -o session-test

sas-test: $(TEST_DEPS) tests/sas.cpp
	$(CXX) $(TEST_CXXFLAGS) \
	tests/sas.cpp -o sas-test

group-session-test: $(TEST_DEPS) tests/group_session.cpp
	$(CXX) $(TEST_CXXFLAGS) \
	tests/group_session.cpp -o group-session-test

test: account-test session-test sas-test group-session-test
	./account-test --gtest_color=yes
	./session-test --gtest_color=yes
	./sas-test --gtest_color=yes
	./group-session-test --gtest_color=yes

install:
	$(INSTALL) -d $(DESTDIR)$(INCLUDEDIR) $(DESTDIR)$(LIBDIR)
	$(INSTALL) -m644 $(STATIC_LIB) $(DESTDIR)$(LIBDIR)
	$(INSTALL) -m755 $(SHARED_LIB_PREFIX).$(VERSION) $(DESTDIR)$(LIBDIR)
	$(LN) -sfv $(SHARED_LIB_NAME).$(VERSION) $(DESTDIR)$(LIBDIR)/$(SHARED_LIB_NAME).$(SOVERSION)
	$(LN) -sfv $(SHARED_LIB_NAME).$(SOVERSION) $(DESTDIR)$(LIBDIR)/$(SHARED_LIB_NAME)
	$(CP) $(HEADER) $(DESTDIR)$(INCLUDEDIR)/vodozemac.h
