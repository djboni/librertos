#!/bin/sh
set -e

File="tests/concurrent_test.cpp"
BuildDir="build"
Gdb="$BuildDir/${File%.[cC]*}.gdb"
Exec="$BuildDir/${File%.[cC]*}.elf"

find_line_on_fileX_afterY_with_codeZ() {
    # Find first Z after Y in File.
    # Output: File:LineZ
    #
    # To find the first "foo" after in the function "bar" in file.c
    # call "find_line_on_fileX_afterY_with_codeZ file.c bar foo".
    # The output will be "file.c:42.

    File="$1"
    Y="$2"
    Z="$3"

    LineY="$(grep -Fn "$Y" "$File" | cut -f1 -d: | head -n1)"
    LineZ="$(grep -Fn "$Z" "$File" | cut -f1 -d:)"

    DidBreak=
    for Line in $LineZ; do
        if [ $Line -gt $LineY ]; then
            DidBreak=1
            break
        fi
    done

    if [ -z $DidBreak ]; then
        # Did not break
        echo "find_line_on_fileX_afterY_with_codeZ: error: could not find Z" >&2
        exit 1
    fi

    echo "$File:$Line"
}

echo "
set verbose off
set pagination off

############################################################################
# Event_PosTaskRemovedFromEventWhileSuspending_LowerOrEqualPriority
############################################################################

tbreak func_test1_task1
commands
    tbreak $(
        find_line_on_fileX_afterY_with_codeZ "src/librertos.c" \
        "event_find_priority_position(struct list_t *list, int8_t priority)" \
        "if (priority > pos_priority)"
    )
    commands
        call func_test1_interrupt_resume_suspend_task0()
        continue
    end
    continue
end

############################################################################
# Event_PosTaskRemovedFromEventWhileSuspending_HigherPriority
############################################################################

tbreak func_test2_task1
commands
    tbreak $(
        find_line_on_fileX_afterY_with_codeZ "src/librertos.c" \
        "event_find_priority_position(struct list_t *list, int8_t priority)" \
        "if (priority > pos_priority)"
    )
    commands
        call func_test2_interrupt_resume_suspend_task0()
        continue
    end
    continue
end

############################################################################
# Event_PosTaskRemovedFromEventWhileSuspending_HigherPriority_2
############################################################################

tbreak func_test3_task1
commands
    tbreak $(
        find_line_on_fileX_afterY_with_codeZ "src/librertos.c" \
        "event_find_priority_position(struct list_t *list, int8_t priority)" \
        "if (priority > pos_priority)"
    )
    commands
        call func_test3_interrupt_resume_suspend_task0()
        continue
    end
    continue
end

############################################################################
# Event_TaskResumedWhileSuspending
############################################################################

tbreak func_test4_task0
commands
    tbreak $(
        find_line_on_fileX_afterY_with_codeZ "src/librertos.c" \
        "event_find_priority_position(struct list_t *list, int8_t priority)" \
        "if (priority > pos_priority)"
    )
    commands
        call func_test4_interrupt_resume_task0()
        continue
    end
    continue
end

############################################################################
# Delay_PosTaskRemoved_GreaterOrEqualTick
############################################################################

tbreak func_test5_task1
commands
    tbreak $(
        find_line_on_fileX_afterY_with_codeZ "src/librertos.c" \
        "static struct node_t *delay_find_tick_position(struct list_t *list, tick_t tick)" \
        "if (tick < pos_tick)"
    )
    commands
        call func_test5_interrupt_resume_suspend_task0()
        continue
    end
    continue
end

############################################################################
# Delay_PosTaskRemoved_LowerTick
############################################################################

tbreak func_test6_task1
commands
    tbreak $(
        find_line_on_fileX_afterY_with_codeZ "src/librertos.c" \
        "static struct node_t *delay_find_tick_position(struct list_t *list, tick_t tick)" \
        "if (tick < pos_tick)"
    )
    commands
        call func_test6_interrupt_resume_suspend_task0()
        continue
    end
    continue
end

############################################################################
# Delay_TaskResumedWhileSuspending
############################################################################

tbreak func_test7_task1
commands
    tbreak $(
        find_line_on_fileX_afterY_with_codeZ "src/librertos.c" \
        "static struct node_t *delay_find_tick_position(struct list_t *list, tick_t tick)" \
        "if (tick < pos_tick)"
    )
    commands
        call func_test7_interrupt_resume_task1()
        continue
    end
    continue
end

############################################################################
# Run
############################################################################

run
quit
" > "$Gdb"

tests/run_tests.sh --no-custom-test "$File"

if [ ! -f "$Exec" ]; then
    echo "$0: error: could not find '$Exec'" >&2
    exit 1
elif [ ! -f "$Gdb" ]; then
    echo "$0: error: could not find '$Gdb'" >&2
    exit 2
fi

ASAN_OPTIONS="detect_leaks=0" \
gdb -nx --command="$Gdb" --batch-silent --return-child-result "$Exec"
