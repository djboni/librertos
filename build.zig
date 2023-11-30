// git submodules update --init
// mkdir -p tests/cpputest/cpputest_build; cd tests/cpputest/cpputest_build
// CC="zig cc" CXX="zig c++" CMAKE_GENERATOR=Ninja cmake ..
// cmake --build . --target src/CppUTest/libCppUTest.a --target src/CppUTestExt/libCppUTestExt.a
// zig build
// zig build test

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
        "src/librertos.c",
    }, &cflags_c90);

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
        "src/librertos.c",
    }, &cflags_c90_testing);

    test_librertos.addIncludePath(std.Build.LazyPath{ .path = "./tests/cpputest/include" });
    test_librertos.addLibraryPath(std.build.LazyPath{ .path = "./tests/cpputest/cpputest_build/src/CppUTest" });
    test_librertos.addLibraryPath(std.build.LazyPath{ .path = "./tests/cpputest/cpputest_build/src/CppUTestExt" });
    test_librertos.linkSystemLibrary("CppUTest");
    test_librertos.linkSystemLibrary("CppUTestExt");

    test_librertos.addIncludePath(std.build.LazyPath{ .path = "." });
    test_librertos.addCSourceFiles(&[_][]const u8{
        // Tests
        "tests/librertos_delay_test.cpp",
        "tests/librertos_event_test.cpp",
        "tests/librertos_list_test.cpp",
        "tests/librertos_sched_test.cpp",
        "tests/librertos_tick_test.cpp",
        "tests/librertos_timer_test.cpp",
        "tests/concurrent_test.cpp",
        // Test Events
        "tests/semaphore_test.cpp",
        "tests/mutex_test.cpp",
        "tests/queue_test.cpp",
        // Supporting files
        "tests/port/librertos_port.cpp",
        "tests/mocks/librertos_assert.cpp",
        "tests/utils/librertos_test_utils.cpp",
        "tests/utils/librertos_custom_tests.cpp",
        "tests/utils/main.cpp",
    }, &cflags_cpp11);
}
