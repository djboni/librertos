# Copyright (c) 2016-2023 Djones A. Boni - MIT License

all: run_tests

clean:
	misc/run_tests.sh --clean

run_tests:
	misc/run_tests.sh --all

run_coverage:
	misc/run_tests.sh --all --coverage
