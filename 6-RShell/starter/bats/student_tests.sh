#!/usr/bin/env bats

# File: student_tests.sh
# 
# Create your unit tests suit in this file
setup() {
    # Kill any existing dsh servers
    pkill -f "./dsh -s" || true
    sleep 1
}

teardown() {
    # Kill any dsh servers that might be running
    pkill -f "./dsh -s" || true
    sleep 1
}

# Test 1: Basic local shell functionality
@test "Local shell can execute basic commands" {
    run ./dsh <<EOF
ls
exit
EOF
    
    [ "$status" -eq 0 ]
    [[ "$output" == *"dsh_cli.c"* ]]
    [[ "$output" == *"rsh_cli.c"* ]]
}

# Test 2: Local shell piping
@test "Local shell can execute piped commands" {
    run ./dsh <<EOF
ls | grep .c
exit
EOF
    
    [ "$status" -eq 0 ]
    [[ "$output" == *".c"* ]]
}

# Test 3: Server starts and stops correctly
@test "Server starts and stops correctly" {
    # Start server in background
    ./dsh -s -p 12340 &
    sleep 2
    
    # Verify server is running by connecting a client
    echo "echo test" | ./dsh -c -p 12340
    [ $? -eq 0 ]
    
    # Stop server using stop-server command
    echo "stop-server" | ./dsh -c -p 12340
    sleep 2
    
    # Try to connect again - should fail
    echo "echo test" | ./dsh -c -p 12340
    [ $? -ne 0 ]
}

# Test 4: Client can connect to server and execute a simple command
@test "Client can connect to server and execute a simple command" {
    # Start server in background
    ./dsh -s -p 12341 &
    SERVER_PID=$!
    sleep 1
    
    # Run client and execute 'ls' command
    run bash -c "echo 'ls' | ./dsh -c -p 12341"
    
    # Server should be running
    ps -p $SERVER_PID
    [ $? -eq 0 ]
    
    # Command should execute successfully
    [ "$status" -eq 0 ]
    
    # Output should contain directory contents
    [[ "$output" == *"dsh_cli.c"* ]]
    
    # Clean up - stop server
    kill $SERVER_PID
}

# Test 5: Client can execute a pipeline command
@test "Client can execute a pipeline command" {
    # Start server in background
    ./dsh -s -p 12342 &
    SERVER_PID=$!
    sleep 1
    
    # Run client and execute pipeline command
    run bash -c "echo 'ls -l | grep .c' | ./dsh -c -p 12342"
    
    # Command should execute successfully
    [ "$status" -eq 0 ]
    
    # Output should contain .c files
    [[ "$output" == *".c"* ]]
    
    # Clean up - stop server
    kill $SERVER_PID
}

# Test 6: Built-in exit command
@test "Client can exit properly" {
    # Start server in background
    ./dsh -s -p 12343 &
    SERVER_PID=$!
    sleep 1
    
    # Run client and execute exit command
    run bash -c "echo 'exit' | ./dsh -c -p 12343"
    
    # Command should execute successfully
    [ "$status" -eq 0 ]
    
    # Server should still be running
    ps -p $SERVER_PID
    [ $? -eq 0 ]
    
    # Clean up - stop server
    kill $SERVER_PID
}

# Test 7: Built-in stop-server command
@test "Client can stop the server" {
    # Start server in background
    ./dsh -s -p 12344 &
    SERVER_PID=$!
    sleep 1
    
    # Run client and execute stop-server command
    run bash -c "echo 'stop-server' | ./dsh -c -p 12344"
    
    # Command should execute successfully
    [ "$status" -eq 0 ]
    
    # Wait for server to stop
    sleep 5

    # Instead of checking the process ID, check if the port is still in use
    if netstat -tuln | grep ":12344 " >/dev/null; then
        # Server is still running (test should fail)
        [ 1 -eq 0 ]  # This will always fail
    else
        # Server has stopped (test should pass)
        [ 1 -eq 1 ]  # This will always pass
    fi
}

# Test 8: Multiple clients can connect to server
@test "Multiple clients can connect to server" {
    # Start server in background
    ./dsh -s -p 12345 &
    SERVER_PID=$!
    sleep 1
    
    # Run first client
    run bash -c "echo 'echo Client 1' | ./dsh -c -p 12345"
    
    # First client should execute successfully
    [ "$status" -eq 0 ]
    [[ "$output" == *"Client 1"* ]]
    
    # Run second client
    run bash -c "echo 'echo Client 2' | ./dsh -c -p 12345"
    
    # Second client should execute successfully
    [ "$status" -eq 0 ]
    [[ "$output" == *"Client 2"* ]]
    
    # Clean up - stop server
    kill $SERVER_PID
}

# Test 9: Server handles invalid commands gracefully
@test "Server handles invalid commands gracefully" {
    # Start server in background
    ./dsh -s -p 12346 &
    SERVER_PID=$!
    sleep 1
    
    # Run client with invalid command
    run bash -c "echo 'nonexistentcommand' | ./dsh -c -p 12346"
    
    # Command should fail but client should handle it gracefully
    [[ "$output" == *"nonexistentcommand"* ]]
    [[ "$output" == *"not found"* ]] || [[ "$output" == *"command not found"* ]] || [[ "$output" == *"No such file"* ]]
    
    # Server should still be running
    ps -p $SERVER_PID
    [ $? -eq 0 ]
    
    # Clean up - stop server
    kill $SERVER_PID
}

# Test 10: Built-in cd command works
@test "Built-in cd command works" {
    # Start server in background
    ./dsh -s -p 12347 &
    SERVER_PID=$!
    sleep 1
    
    # Run client with cd command followed by pwd
    run bash -c "echo -e 'cd /tmp\npwd' | ./dsh -c -p 12347"
    
    # Output should show we've changed to /tmp
    [[ "$output" == *"/tmp"* ]]
    
    # Clean up - stop server
    kill $SERVER_PID
}

# Test 11: Test threaded server mode (extra credit)
@test "Threaded server mode works" {
    # Start server in threaded mode
    ./dsh -s -p 12348 -x &
    SERVER_PID=$!
    sleep 1
    
    # Start a background client that sleeps
    bash -c "echo 'sleep 5' | ./dsh -c -p 12348" &
    SLEEP_CLIENT=$!
    sleep 1
    
    # Start a second client that should work simultaneously
    run bash -c "echo 'echo Second client works while first sleeps' | ./dsh -c -p 12348"
    
    # Second client should work even though first is sleeping
    [ "$status" -eq 0 ]
    [[ "$output" == *"Second client works while first sleeps"* ]]
    
    # Wait for first client to finish
    wait $SLEEP_CLIENT
    
    # Clean up - stop server
    kill $SERVER_PID
}

# Test 12: Test handling large output
@test "Server handles large output correctly" {
    # Start server in background
    ./dsh -s -p 12349 &
    SERVER_PID=$!
    sleep 1
    
    # Run client with command that generates large output
    run bash -c "echo 'cat /proc/cpuinfo' | ./dsh -c -p 12349"
    
    # Command should execute successfully
    [ "$status" -eq 0 ]
    
    # Output should be substantial and contain expected parts
    [[ ${#output} -gt 500 ]]  # Output should be large
    
    # Clean up - stop server
    kill $SERVER_PID
}
@test "Example: check ls runs without errors" {
    run ./dsh <<EOF                
ls
EOF

    # Assertions
    [ "$status" -eq 0 ]
}
