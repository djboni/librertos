# Copyright (c) 2022 Djones A. Boni - MIT License

all: cpputest run_tests

clean:
	misc/run_tests.sh --clean

run_tests:
	misc/run_tests.sh --all

run_coverage:
	misc/run_tests.sh --all --coverage

cpputest: misc/cpputest/cpputest_build/lib/libCppUTest.a
misc/cpputest/cpputest_build/lib/libCppUTest.a:
	cd "misc/cpputest/cpputest_build"; \
	autoreconf .. -i; \
	../configure; \
	make
