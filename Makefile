# Copyright (c) 2022 Djones A. Boni - MIT License

all: run_tests

clean:
	misc/run_tests.sh --clean

run_tests:
	misc/run_tests.sh --all

run_coverage:
	misc/run_tests.sh --all --coverage
