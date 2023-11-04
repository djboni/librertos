#!/bin/sh
# Copyright (c) 2016-2023 Djones A. Boni - MIT License

# Constants

BuildScript="tests/utils/build.mk"
CPPUTEST_DIR="tests/cpputest"

BuildDir="build"
SrcDir="src"
PortDir="tests/port"
TestsDir="tests"
TestsEnd="_test.[cC]*"
TestsObjDir="$BuildDir/obj_tests"
ObjDir="$BuildDir/obj"

OptionRunTest=1
OptionRunCustomTest=

# Variables
TestTotal=0
TestError=0
TestInexistent=0
ExitStatus=0

DoUpdateResults() {
    # == 0 Success
    # != 0 Error
    TestResult=$1

    # == 0 Test present
    # != 0 No test
    TestPresent=$2

    if [ $TestResult -ne 0 ]; then
        ExitStatus=1
        TestError=$((TestError + 1))
    fi

    if [ $TestPresent -ne 0 ]; then
        TestInexistent=$((TestInexistent + 1))
    fi

    TestTotal=$((TestTotal + 1))
}

DoPrintResults() {
    echo "=============================================="
    echo -n "$TestTotal Tests, "
    echo -n "$TestInexistent Inexistent, "
    echo -n "$TestError Failures"
    echo
    if [ $TestError -eq 0 ]; then
        echo "OK"
    else
        echo "FAIL"
    fi
    echo
}

DoBuildCppUTestIfNecessary() {

    # Build CppUTest if necessary
    if [ ! -f "$CPPUTEST_DIR/cpputest_build/lib/libCppUTest.a" ]; then
        echo "Building CppUTest in the directory '$CPPUTEST_DIR'..."
        (
            # Show commands and exit on any error
            set -xe

            # Build CppUTest
            mkdir -p "$CPPUTEST_DIR/cpputest_build"
            cd "$CPPUTEST_DIR/cpputest_build"
            autoreconf .. -i
            ../configure
            make
        )
    fi
}

DetermineMainFile() {
    MainFile="$(
        sed -En '
            s/(^|.*\s)(Main file): (\S+(\s+\S+)*)(.*|$)/\3/p
        ' "$Test"
    )"

    echo "$MainFile"
}

DetermineAdditionalFiles() {
    Test="$1"
    TestDir="${Test%/*}"

    ListOfFiles="$(
        sed -En '
            s/(^|.*\s)(Also compile|Main file): (\S+(\s+\S+)*)(.*|$)/\3/p
        ' "$Test"
    )"

    AdditionalFiles=""

    for F in $ListOfFiles; do
        case "$F" in
        /*)
            # Absolute path
            AdditionalFiles="$AdditionalFiles $F"
            ;;
        *)
            # Relative path
            if [ -f "$F" ]; then
                # Found file based on current directory
                AdditionalFiles="$AdditionalFiles $F"
            elif [ -f "$TestDir/$F" ]; then
                # Found file based on test directory
                AdditionalFiles="$AdditionalFiles $TestDir/$F"
            else
                echo "Error: could not find file '$F'."
                exit 1
            fi
            ;;
        esac
    done
}

DoRunTest() {

    # Arguments
    Test="$1"

    TestSH="${Test%.[cC]*}.sh"
    if [ -z $OptionRunCustomTest ] && [ -f "$TestSH" ]; then
        # Run the custom test

        sh "$TestSH"
        TestResult=$?

        # Update results
        DoUpdateResults $TestResult 0

        return
    fi

    # Determine file names and directories

    Exec="$BuildDir/${Test%.[cC]*}.elf"
    ExecDir="${Exec%/*}"

    MainFile="$(DetermineMainFile "$Test")"
    MainObject="$ObjDir/${MainFile%.[cC]*}.o"
    MainObjectDir="${MainObject%/*}"

    # Create directories

    if [ ! -d "$ExecDir" ]; then
        mkdir -p "$ExecDir"
    fi

    if [ ! -d "$MainObjectDir" ]; then
        mkdir -p "$MainObjectDir"
    fi

    if [ ! -f "$Test" ]; then
        # The test does NOT exist

        # Update results
        DoUpdateResults 0 1
    else
        # The test exists

        # Build test

        ASAN="-fsanitize=address" # Address sanitizer
        UBSAN="-fsanitize=undefined -fno-sanitize-recover" # Undefined behavior sanitizer

        CC="g++"
        CFLAGS="-g -O0 -std=c++98 -pedantic -Wall -Wextra -Wno-long-long --coverage $ASAN $UBSAN"
        CFLAGS="$CFLAGS -include tests/utils/MemoryLeakDetector.h"
        CXX="g++"
        CXXFLAGS="-g -O0 -std=c++11 -pedantic -Wall -Wextra -Wno-long-long --coverage $ASAN $UBSAN"
        CXXFLAGS="$CXXFLAGS -include tests/utils/MemoryLeakDetector.h"
        CPPFLAGS="-I include -I $PortDir -I . -I $CPPUTEST_DIR/include"
        LD="g++"
        LDFLAGS="--coverage $ASAN $UBSAN -l CppUTest -l CppUTestExt -L $CPPUTEST_DIR/cpputest_build/lib"

        # Create test runner
        # Do nothing

        DoBuildCppUTestIfNecessary

        DetermineAdditionalFiles "$Test"

        make -f $BuildScript \
            EXEC="$Exec" \
            OBJ_DIR="$TestsObjDir" \
            INPUTS="$Test $AdditionalFiles tests/utils/main.cpp $PortDir/librertos_port.cpp" \
            CC="$CC" \
            CFLAGS="$CFLAGS" \
            CXX="$CXX" \
            CXXFLAGS="$CXXFLAGS" \
            CPPFLAGS="$CPPFLAGS" \
            LD="$LD" \
            LDFLAGS="$LDFLAGS" \
            "$Exec"
        BuildResult=$?

        # Update results and return if building fails
        if [ $BuildResult -ne 0 ]; then
            DoUpdateResults $BuildResult 0
            return
        fi

        if [ ! -z $OptionRunTest ]; then
            # Run test

            "$Exec"
            TestResult=$?

            # Update results
            DoUpdateResults $TestResult 0

            # Return if testing fails
            if [ $TestResult -ne 0 ]; then
                return
            fi
        else
            # Do not run test and do not build file
            return
        fi
    fi

    # Build file
    if [ -z $MainFile ]; then
        # No main file to be build for the test
        return
    elif [ ! -f "$MainFile" ]; then
        # Main file does not exist
        echo "Error: Main file '$MailFile' does not exist"
        DoUpdateResults 1 0
        return
    else
        # Main file exists

        CC="gcc"
        CFLAGS="-g -O0 -std=c90 -pedantic -Wall -Wextra -Werror -Wno-long-long"
        CXX="g++"
        CXXFLAGS="-g -O0 -std=c++98 -pedantic -Wall -Wextra -Werror -Wno-long-long"
        CPPFLAGS="-I include -I $PortDir"
        LD="gcc"
        LDFLAGS=""

        make -f $BuildScript \
            OBJ_DIR="$ObjDir" \
            INPUTS="$MainFile" \
            CC="$CC" \
            CFLAGS="$CFLAGS" \
            CXX="$CXX" \
            CXXFLAGS="$CXXFLAGS" \
            CPPFLAGS="$CPPFLAGS" \
            LD="$LD" \
            LDFLAGS="$LDFLAGS" \
            "$MainObject"
        BuildResult=$?

        # Update results and return if building fails
        if [ $BuildResult -ne 0 ]; then
            DoUpdateResults $BuildResult 0
            return
        fi
    fi
}

DoCoverageIfRequested() {
    FilterXML="--root=."
    FilterTXT="--filter=$SrcDir/"
    gcovr $FilterXML --xml cov.xml \
            --exclude-unreachable-branches \
            --exclude-throw-branches
    if [ ! -z $FlagCoverage ]; then
        gcovr $FilterTXT  --branch \
            --exclude-unreachable-branches \
            --exclude-throw-branches
        gcovr $FilterTXT  | sed '1,4d'
    fi
}

DoPrintUsage() {
    echo "Usage: ${0##*/} [--exec|--error|--clean|--coverage|--all] [FILEs...]"
}

DoProcessCommandLineArguments() {

    # No arguments: invalid
    if [ $# -eq 0 ]; then
        DoPrintUsage
        exit 1
    fi

    # One or more arguments
    while [ $# -gt 0 ]; do
        Arg="$1"
        shift

        case "$Arg" in
        -h|--help)
            DoPrintUsage
            exit 0
            ;;
        -x|--exec)
            # In case there is need to see the commands that are executed
            set -x
            ;;
        -e|--error)
            # Set error flag to stop on first error
            set -e
            ;;
        --only-build)
            OptionRunTest=
            ;;
        --no-custom-test)
            OptionRunCustomTest=1
            ;;
        -c|--clean)
            rm -fr "$BuildDir"
            ;;
        -r|--coverage)
            FlagCoverage=1
            ;;
        -a|--all)
            for File in $(find $TestsDir/ -name '*_test.[cC]*'); do
                DoRunTest "$File"
            done
            ;;
        *)
            DoRunTest "$Arg"
            ;;
        esac
    done

    DoCoverageIfRequested
    DoPrintResults
    exit $ExitStatus
}

DoProcessCommandLineArguments "$@"
