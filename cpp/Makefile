all: rust-lib dynamic-lib

.PHONY: all rust-lib dynamic-lib test

BUILD_TYPE := debug
ifeq ($(BUILD_TYPE), release)
CARGO_ARGS = --release
else
CARGO_ARGS =
endif
VERSION = 0.1.0
SOVERSION = 1
AR = ar
CXX = c++
LN = ln

rust-lib:
	cargo build $(CARGO_ARGS)

dynamic-lib: rust-lib
	rm -rf objects && \
	mkdir objects && \
	cd objects && \
	$(AR) x ../../target/$(BUILD_TYPE)/libvodozemac.a && \
	$(CXX) *.o -shared -o ../../target/$(BUILD_TYPE)/libvodozemac.so.$(VERSION) && \
	$(LN) -sfv libvodozemac.so.$(VERSION) ../../target/$(BUILD_TYPE)/libvodozemac.so.$(SOVERSION) && \
	$(LN) -sfv libvodozemac.so.$(SOVERSION) ../../target/$(BUILD_TYPE)/libvodozemac.so

TEST_CXXFLAGS = -Wall -g -pthread -std=c++17 -lgtest_main \
 -lgtest -lolm -lboost_json -lpthread -I ../target/cxxbridge \
 -L ../target/$(BUILD_TYPE)/ -l vodozemac -ldl -Wl,-rpath,../target/$(BUILD_TYPE)

account-test: dynamic-lib tests/account.cpp
	$(CXX) $(TEST_CXXFLAGS) \
	tests/account.cpp -o account-test


session-test: dynamic-lib tests/session.cpp
	$(CXX) $(TEST_CXXFLAGS) \
	tests/session.cpp -o session-test

sas-test: dynamic-lib tests/sas.cpp
	$(CXX) $(TEST_CXXFLAGS) \
	tests/sas.cpp -o sas-test

group-session-test: dynamic-lib tests/group_session.cpp
	$(CXX) $(TEST_CXXFLAGS) \
	tests/group_session.cpp -o group-session-test

test: account-test session-test sas-test group-session-test
	./account-test --gtest_color=yes
	./session-test --gtest_color=yes
	./sas-test --gtest_color=yes
	./group-session-test --gtest_color=yes
