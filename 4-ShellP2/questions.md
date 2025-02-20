1. Can you think of why we use `fork/execvp` instead of just calling `execvp` directly? What value do you think the `fork` provides?

    > **Answer**: fork() creates a new child process allowing the parent to continue running after execvp replaces the child. Without fork, the parent process would terminate.

2. What happens if the fork() system call fails? How does your implementation handle this scenario?

    > **Answer**:  if fork() fails, it returns -1. The implementation  checks for failure, reports an error (e.g, "fork failed"), and avoid proceeding to execvp or wait

3. How does execvp() find the command to execute? What system environment variable plays a role in this process?

    > **Answer**:  execvp uses the PATH environemnt variable to locate the executable, searching each directory in PATH sequentially if the command is not an absolute path

4. What is the purpose of calling wait() in the parent process after forking? What would happen if we didn’t call it?

    > **Answer**:  wait() ensures the parent pauses until the child exits, preventing zombie processes. Without it, zombies persist until the parent terminates.

5. In the referenced demo code we used WEXITSTATUS(). What information does this provide, and why is it important?

    > **Answer**:  WEXITSTATUS() extracts the childs exit code (returned by wait()), critical for determining success (e.g 0) or failure (non-zero) of the executed command.

6. Describe how your implementation of build_cmd_buff() handles quoted arguments. Why is this necessary?

    > **Answer**:  It parses quoted arguments as single tokens. necessary to preserve arguments containg spaces or special characters.

7. What changes did you make to your parsing logic compared to the previous assignment? Were there any unexpected challenges in refactoring your old code?

    > **Answer**:  Instead of parsing multiple commands separated by pipes, this version simplifies the parsing logic to handle a single command. This refactoring required modifying how arguments are extracted and stored in cmd_buff_t. Handling spaces and quoted arguments was a challenge.

8. For this quesiton, you need to do some research on Linux signals. You can use [this google search](https://www.google.com/search?q=Linux+signals+overview+site%3Aman7.org+OR+site%3Alinux.die.net+OR+site%3Atldp.org&oq=Linux+signals+overview+site%3Aman7.org+OR+site%3Alinux.die.net+OR+site%3Atldp.org&gs_lcrp=EgZjaHJvbWUyBggAEEUYOdIBBzc2MGowajeoAgCwAgA&sourceid=chrome&ie=UTF-8) to get started.

- What is the purpose of signals in a Linux system, and how do they differ from other forms of interprocess communication (IPC)?

    > **Answer**:  Signals are asynchronous, lightweight notifications (e.g interrupts) unlike structured IPC (e.g pipes)

- Find and describe three commonly used signals (e.g., SIGKILL, SIGTERM, SIGINT). What are their typical use cases?

    > **Answer**:  SIGKILL(9): force kills a process (cannot be caught or ignored)
                   SIGTERM(15): Requests termination (handled gracefully, unlike SIGKILL)
                   SIGINT(2): Triggered by Ctrl+C; terminates processes gracefully (can be caught), allowing cleanup

- What happens when a process receives SIGSTOP? Can it be caught or ignored like SIGINT? Why or why not?

    > **Answer**:  SIGSTOP pauses a process indefinitely and it cannot be caught/ignored as it is for job control (e.g ctrl+Z)

