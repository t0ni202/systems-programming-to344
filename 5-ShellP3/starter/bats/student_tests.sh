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

@test "Built-in command: dragon" {
  run ./dsh <<EOF
dragon
exit
EOF
  # Expect the built-in dragon command message
  [ "$status" -eq 0 ]
  echo "$output" | grep -q "Roar! Dragon command executed!"
}

@test "Built-in command: cd" {
  run ./dsh <<EOF
cd ..
pwd
exit
EOF
  [ "$status" -eq 0 ]
  # This test simply checks that pwd output is present;
  # you may adjust the expected output if you know the exact parent directory name.
  echo "$output" | grep -q "/"
}

@test "I/O Redirection: echo to file and cat file" {
  # Remove out.txt if it exists first
  rm -f out.txt
  run ./dsh <<EOF
echo "hello world" > out.txt
cat out.txt
exit
EOF
  [ "$status" -eq 0 ]
  echo "$output" | grep -q "hello world"
}

@test "Pipeline: ls | grep dshlib.c" {
  run ./dsh <<EOF
ls | grep dshlib.c
exit
EOF
  [ "$status" -eq 0 ]
  echo "$output" | grep -q "dshlib.c"
}
