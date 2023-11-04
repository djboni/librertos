# Copyright (c) 2016-2023 Djones A. Boni - MIT License

all: run_tests

clean:
	tests/run_tests.sh --clean

run_tests:
	tests/run_tests.sh --all

run_coverage:
	tests/run_tests.sh --all --coverage
