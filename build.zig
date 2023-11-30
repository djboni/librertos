const std = @import("std");

const cflags_c90 = [_][]const u8{
    "-std=c90",
    "-pedantic",
    "-Wall",
    "-Wextra",
    "-Wundef",
    "-Werror",
};

const cflags_c90_testing = cflags_c90 ++ [_][]const u8{
    "-Wno-unused-but-set-variable",
};

const cflags_cpp11 = [_][]const u8{
    "-std=c++11",
    "-pedantic",
    "-Wall",
    "-Wextra",
    "-Wundef",
    "-Werror",
};

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});
    const test_step = b.step("test", "Run tests");

    const lib = b.addStaticLibrary(.{
        .name = "librertos",
        .root_source_file = null,
        .target = target,
        .optimize = optimize,
    });
    b.installArtifact(lib);
    lib.linkLibC();
    lib.addIncludePath(std.Build.LazyPath{ .path = "./include" });
    lib.addIncludePath(std.Build.LazyPath{ .path = "./examples/linux" });
    lib.addCSourceFiles(&[_][]const u8{
        "./src/librertos.c",
    }, &cflags_c90);

    const cpputest = b.addStaticLibrary(.{
        .name = "CppUTest",
        .root_source_file = null,
        .target = target,
        .optimize = optimize,
    });
    b.installArtifact(cpputest);
    cpputest.linkLibC();
    cpputest.linkLibCpp();
    cpputest.addIncludePath(std.Build.LazyPath{ .path = "./tests/cpputest/include" });
    cpputest.addCSourceFiles(&[_][]const u8{
        "./tests/cpputest/src/CppUTest/CommandLineArguments.cpp",
        "./tests/cpputest/src/CppUTest/MemoryLeakWarningPlugin.cpp",
        "./tests/cpputest/src/CppUTest/TestHarness_c.cpp",
        "./tests/cpputest/src/CppUTest/TestRegistry.cpp",
        "./tests/cpputest/src/CppUTest/CommandLineTestRunner.cpp",
        "./tests/cpputest/src/CppUTest/SimpleString.cpp",
        "./tests/cpputest/src/CppUTest/SimpleStringInternalCache.cpp",
        "./tests/cpputest/src/CppUTest/TestMemoryAllocator.cpp",
        "./tests/cpputest/src/CppUTest/TestResult.cpp",
        "./tests/cpputest/src/CppUTest/JUnitTestOutput.cpp",
        "./tests/cpputest/src/CppUTest/TeamCityTestOutput.cpp",
        "./tests/cpputest/src/CppUTest/TestFailure.cpp",
        "./tests/cpputest/src/CppUTest/TestOutput.cpp",
        "./tests/cpputest/src/CppUTest/MemoryLeakDetector.cpp",
        "./tests/cpputest/src/CppUTest/TestFilter.cpp",
        "./tests/cpputest/src/CppUTest/TestPlugin.cpp",
        "./tests/cpputest/src/CppUTest/TestTestingFixture.cpp",
        "./tests/cpputest/src/CppUTest/SimpleMutex.cpp",
        "./tests/cpputest/src/CppUTest/Utest.cpp",
        "./tests/cpputest/src/Platforms/Gcc/UtestPlatform.cpp",
    }, &cflags_cpp11);

    const cpputest_ext = b.addStaticLibrary(.{
        .name = "CppUTestExt",
        .root_source_file = null,
        .target = target,
        .optimize = optimize,
    });
    b.installArtifact(cpputest_ext);
    cpputest_ext.linkLibC();
    cpputest_ext.linkLibCpp();
    cpputest_ext.addIncludePath(std.Build.LazyPath{ .path = "./tests/cpputest/include" });
    cpputest_ext.addCSourceFiles(&[_][]const u8{
        "./tests/cpputest/src/CppUTestExt/CodeMemoryReportFormatter.cpp",
        "./tests/cpputest/src/CppUTestExt/GTest.cpp",
        "./tests/cpputest/src/CppUTestExt/IEEE754ExceptionsPlugin.cpp",
        "./tests/cpputest/src/CppUTestExt/MemoryReporterPlugin.cpp",
        "./tests/cpputest/src/CppUTestExt/MockFailure.cpp",
        "./tests/cpputest/src/CppUTestExt/MockSupportPlugin.cpp",
        "./tests/cpputest/src/CppUTestExt/MockActualCall.cpp",
        "./tests/cpputest/src/CppUTestExt/MockSupport_c.cpp",
        "./tests/cpputest/src/CppUTestExt/MemoryReportAllocator.cpp",
        "./tests/cpputest/src/CppUTestExt/MockExpectedCall.cpp",
        "./tests/cpputest/src/CppUTestExt/MockNamedValue.cpp",
        "./tests/cpputest/src/CppUTestExt/OrderedTest.cpp",
        "./tests/cpputest/src/CppUTestExt/MemoryReportFormatter.cpp",
        "./tests/cpputest/src/CppUTestExt/MockExpectedCallsList.cpp",
        "./tests/cpputest/src/CppUTestExt/MockSupport.cpp",
    }, &cflags_cpp11);

    const test_librertos = b.addExecutable(.{
        .name = "test_librertos",
        .root_source_file = null,
        .target = target,
        .optimize = optimize,
    });
    b.installArtifact(test_librertos);
    const run_test_librertos = b.addRunArtifact(test_librertos);
    test_step.dependOn(&run_test_librertos.step);
    test_librertos.linkLibC();
    test_librertos.linkLibCpp();
    test_librertos.addIncludePath(std.build.LazyPath{ .path = "./include" });
    test_librertos.addIncludePath(std.build.LazyPath{ .path = "./tests/port" });
    test_librertos.addCSourceFiles(&[_][]const u8{
        "./src/librertos.c",
    }, &cflags_c90_testing);

    test_librertos.addIncludePath(std.Build.LazyPath{ .path = "./tests/cpputest/include" });
    test_librertos.linkLibrary(cpputest);
    test_librertos.linkLibrary(cpputest_ext);

    test_librertos.addIncludePath(std.build.LazyPath{ .path = "." });
    test_librertos.addCSourceFiles(&[_][]const u8{
        // Tests
        "./tests/librertos_delay_test.cpp",
        "./tests/librertos_event_test.cpp",
        "./tests/librertos_list_test.cpp",
        "./tests/librertos_sched_test.cpp",
        "./tests/librertos_tick_test.cpp",
        "./tests/librertos_timer_test.cpp",
        "./tests/concurrent_test.cpp",
        // Test Events
        "./tests/semaphore_test.cpp",
        "./tests/mutex_test.cpp",
        "./tests/queue_test.cpp",
        // Supporting files
        "./tests/port/librertos_port.cpp",
        "./tests/mocks/librertos_assert.cpp",
        "./tests/utils/librertos_test_utils.cpp",
        "./tests/utils/librertos_custom_tests.cpp",
        "./tests/utils/main.cpp",
    }, &cflags_cpp11);
}
