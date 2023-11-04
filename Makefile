# Copyright (c) 2022 Djones A. Boni - MIT License

all: cpputest run_tests clang-tidy

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

clang-tidy:
	clang-tidy \
	    src/librertos.c \
	    --format-style=file \
	    --warnings-as-errors=* \
	    --header-filter=.* \
	    --checks=*,\
	-cppcoreguidelines-init-variables,\
	-*-braces-around-statements,\
	-misc-redundant-expression,\
	-llvm-header-guard,\
	    -- -std=c90 -I include -I ports/linux
