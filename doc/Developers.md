# Developers

This document is meant for LibreRTOS development.

## Installing Dependencies

For now we support testing and other development tooling in the latest
Ubuntu LTS (22.04). It may work in other versions, distros or OSs.

Install the dependencies:

```sh
sudo apt install git make gcc g++ dh-autoreconf gcovr pre-commit
```

## Cloning the Repository with Git

Cloning the Git repository enables us to use some useful tools, such as
submodules, pre-commit and unit-tests.

Clone the project with Git, initialize the submodules and enable pre-commit:

```sh
git clone https://github.com/djboni/librertos
cd librertos
git submodule update --init
pre-commit install
```

## Code Formatting

This project uses [pre-commit](https://clang.llvm.org/docs/ClangFormatStyleOptions.html)
and [Clang-Format](https://pre-commit.com/) to minimize issues with text files
and code formatting.

In the step [Installing Dependencies](Developers#Installing-Dependencies)
we enable pre-commit in the repository, so now when Git creates a commit it will
run the pre-commit hooks that check the text files and formats the code.

The code formatter is very liberal with line width, however:

- Manually keep the lines at most 80 characters wide
- Sometimes it is hard to break a long statements or equations, so:
  - Don't fight too much with the formatter (let him win a bit)
  - Wider lines are preferred if they keep the code flow (yes, yes, very subjective)
- Use only C-style `/* comments */`
- Prefer comments before the line
- A line with a trailing comment must not go beyond 80 characters wide

## Running Tests

This project contains unit tests using the
[CppUTest](https://github.com/cpputest/cpputest) test framework.

These tests are meant to be used while developing LibreRTOS, to test features,
reproduce bugs, and fix them.

We aim for a 100% test coverage:

- 100% line coverage
- 100% branch coverage

A few branches are never exercised in the tests:

- Null pointers checks on `memset` that the compiler inserts, and
- `assert`s that never fail during the tests.

1. One command to build and to run the unit-tests:

   ```sh
   make
   ```

   The end of the test log should be something like this (no failures):

   ```
   ...
   ==============================================
   11 Tests, 0 Inexistent, 0 Failures
   OK
   ```

2. It is also possible to run the tests and check the code coverage information:

   ```sh
   make run_coverage
   ```

3. If you want to run a specific test use the script `tests/run_tests.sh` and pass
   the **test** path. Example:

   ```sh
   tests/run_tests.sh tests/src/semaphore_test.cpp
   ```
