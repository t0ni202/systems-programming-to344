#!/usr/bin/env bats

# File: student_tests.sh
# 
# Create your unit tests suit in this file

@test "Example: check ls runs without errors" {
    run ./dsh <<EOF                
ls
EOF

    # Assertions
    [ "$status" -eq 0 ]
}

@test "Command with arguments works" {
    run ./dsh <<EOF
ls -l
EOF
    [ "$status" -eq 0 ]
}

# Built-in cd command tests
@test "cd to new directory and verify with pwd" {
    # Create test directory
    mkdir -p test_dir

    run ./dsh <<EOF
cd test_dir
pwd
EOF

    # Clean up
    rmdir test_dir

    [ "$status" -eq 0 ]
    [[ "$output" =~ "test_dir" ]]
}

@test "cd with no arguments stays in same directory" {
    initial_dir=$(pwd)

    run ./dsh <<EOF
cd
pwd
EOF

    stripped_output=$(echo "$output" | tr -d '[:space:]')
    [[ "$stripped_output" =~ "$initial_dir" ]]
}

# Quoted string handling tests
@test "echo with quoted spaces preserves spacing" {
    run ./dsh <<EOF
echo "  hello    world  "
EOF

    stripped_output=$(echo "$output" | tr -d '\n\r\f\v')
    [[ "$stripped_output" =~ "  hello    world  " ]]
}

@test "echo with multiple quoted arguments" {
    run ./dsh <<EOF
echo "first quote" "second quote"
EOF

    stripped_output=$(echo "$output" | tr -d '\n\r\f\v')
    [[ "$stripped_output" =~ "first quote second quote" ]]
}

# Error handling tests
@test "Nonexistent command shows error" {
    run ./dsh <<EOF
nonexistentcommand
EOF

    [ "$status" -eq 0 ]  # Shell should continue running
    [[ "$output" =~ "Command not found in PATH" ]]
}

@test "cd to nonexistent directory shows error" {
    run ./dsh <<EOF
cd /nonexistent/directory
EOF

    [ "$status" -eq 0 ]  # Shell should continue running
    [[ "$output" =~ "No such file or directory" ]]
}

# Exit command test
@test "Exit command works" {
    run ./dsh <<EOF
exit
EOF

    [ "$status" -eq 0 ]
}

# Multiple command sequence test
@test "Multiple commands execute in sequence" {
    run ./dsh <<EOF
pwd
cd ..
pwd
cd -
EOF

    [ "$status" -eq 0 ]
    [ "${#lines[@]}" -gt 4 ]  # Should have multiple lines of output
}
