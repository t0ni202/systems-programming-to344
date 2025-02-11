1. In this assignment I suggested you use `fgets()` to get user input in the main while loop. Why is `fgets()` a good choice for this application?

    > **Answer**:  fgets() is a good choice because it safely reads input from stdin up to a specified limit, ensuring buffer overflows are avoided. Additionally it reads input until a newline \n or EOF is enncounterd, making it well-suited for handling command line input in a structured way.

2. You needed to use `malloc()` to allocte memory for `cmd_buff` in `dsh_cli.c`. Can you explain why you needed to do that, instead of allocating a fixed-size array?

    > **Answer**:  Using malloc() alllows for dynamic memory allocation, making it possible to adjust the buffer size based on requirements. A fixed size array might either be too small (truncation) or too large (wasting memory). Dynamic allocation provides flexibility and ensures efficient memory usage.

3. In `dshlib.c`, the function `build_cmd_list(`)` must trim leading and trailing spaces from each command before storing it. Why is this necessary? If we didn't trim spaces, what kind of issues might arise when executing commands in our shell?

    > **Answer**:  Trimming spcaes ensures that command parsing is clean and accurate. If spaces are not trimmed, commmands might not execute properly, extra empty arguments could be passed, or unneccesary errors might occur when matching command names.

4. For this question you need to do some research on STDIN, STDOUT, and STDERR in Linux. We've learned this week that shells are "robust brokers of input and output". Google _"linux shell stdin stdout stderr explained"_ to get started.

- One topic you should have found information on is "redirection". Please provide at least 3 redirection examples that we should implement in our custom shell, and explain what challenges we might have implementing them.

    > **Answer**:  1. command > output.txt (redirects STDOUT to a file): Requires modifying file descriptors to write output to a file.
                2. command < input.txt (redirects STDIN from a file): Requires modifying file descriptors to read input from a file.
                3. command 2> error.txt (redirects STDERR to a file): Requires handling STDERR separately from STDOUT in redirection.

                Challenges: Properly managing file descriptors, ensuring correct permissions for file operations, and handling cases where files don't exist or cannot be written.


- You should have also learned about "pipes". Redirection and piping both involve controlling input and output in the shell, but they serve different purposes. Explain the key differences between redirection and piping.

    > **Answer**:  Redirection modifies where input and output are read from or written to (files/devices). Piping connects the output of one command to the input of another, alll

- STDERR is often used for error messages, while STDOUT is for regular output. Why is it important to keep these separate in a shell?

    > **Answer**:  Keeping them separate alllows error messages to be processed independently from normal command output. This makes  debugging easier and prevents error messages from interfering with data that a command outputs.

- How should our custom shell handle errors from commands that fail? Consider cases where a command outputs both STDOUT and STDERR. Should we provide a way to merge them, and if so, how?

    > **Answer**:  The shell should detect and report command failures by checking return values. It should allow users choose whether to merhe STDERR and STDOUT (e.g 2>&1).
