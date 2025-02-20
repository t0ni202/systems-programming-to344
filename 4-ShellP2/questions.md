1. Can you think of why we use `fork/execvp` instead of just calling `execvp` directly? What value do you think the `fork` provides?

    > **Answer**: The fork/execvp pattern allows the shell to continue by creating a child process that can be replaced
	 > with a new program while the the paren shell process remains intact. Without fork() executing execvp() directly
	 > would replace the shell process entirely, terminating it. The two step process also enables important shell
	 > features like maintaining stat,handling background processes and recovering from command failures.


2. What happens if the fork() system call fails? How does your implementation handle this scenario?

    > **Answer**:  if fork() fails, it returns -1 and sets errno to indicate the specific error, typically due to
	 > resource exhaustion (e.g too many processes or insufficient memory).My implementation handles this by printing the
	 > error with perror("fork") and cleaning up command resources. The shell then continues its command loop rather than
	 > terminating, maintaining stability even under resource constraints.

3. How does execvp() find the command to execute? What system environment variable plays a role in this process?

    > **Answer**: Execvp() first checks if the command contains a slash (/) to try executing it as a direct path. If no
	 > slash is present , it searches each directory listed in the PATH environment variable in order until it finds and
	 > executable matching the command name. This differs from execv() which requires absolute aths and allows users to
	 > run commands like ls without specifying their full path.

4. What is the purpose of calling wait() in the parent process after forking? What would happen if we didn’t call it?

    > **Answer**: wait() prevents zombie processes by properly cleaning up terminated child processes and their
	 > resources from the system. It also provides synchronization by blocking the parent until the child completes,
	 > which enables proper sequential cpmmand execution. Additionally wait() retrieves the childs exit status , allowing
	 > the shell to detect and handle command failures appropriately.

5. In the referenced demo code we used WEXITSTATUS(). What information does this provide, and why is it important?

    > **Answer**:  WEXITSTATUS() extracts the lower 8 bits fomr the status value returned by wait(),but should ontly be
	 > used when WIFEXITED() confirms notmal termination. The exit code (0-255) indicates command success or failure and
	 > can carry error information from failed execvp() calls. The status is crucial for shell features like error
	 > detection, conditional execution and implementing the return code builtin.

6. Describe how your implementation of build_cmd_buff() handles quoted arguments. Why is this necessary?

    > **Answer**: it uses a state machine to track quote context which preserves spaces and special characters within
	 > quotes strings while stripping the quotes themselves from the final arguments. Ir proprly handles both single and
	 > double quotes ensuring proper parsing of complex arguments like 'echo "hello world"'. This functionality is
	 > neccessary because commands often need to handle arguments containing spaces, such as file paths or multi word
	 > strings

7. What changes did you make to your parsing logic compared to the previous assignment? Were there any unexpected challenges in refactoring your old code?

    > **Answer**: I removed pipe handling completely and switched from a commmand_list_t structure to working with a
	 > single cmd_buff for simpler command processing. Challenges included mamnging memory without pipes, this required
	 > careful attention to cleanup and allocation. 

8. For this quesiton, you need to do some research on Linux signals. You can use [this google search](https://www.google.com/search?q=Linux+signals+overview+site%3Aman7.org+OR+site%3Alinux.die.net+OR+site%3Atldp.org&oq=Linux+signals+overview+site%3Aman7.org+OR+site%3Alinux.die.net+OR+site%3Atldp.org&gs_lcrp=EgZjaHJvbWUyBggAEEUYOdIBBzc2MGowajeoAgCwAgA&sourceid=chrome&ie=UTF-8) to get started.

- What is the purpose of signals in a Linux system, and how do they differ from other forms of interprocess communication (IPC)?

    > **Answer**:  Signals are asynchronous, lightweight notifications (e.g interrupts) used for process control and
	 > system events inLinux. Unlike other IPC methods such as pipes or shared memory, signals dont carry data beyond
	 > their signal number and can be sent by both processes and rthe kernel. They provid a non queued mechanism for
	 > process  communicaiton making them ideal for simpe control operations but less suitable for data transfer. 

- Find and describe three commonly used signals (e.g., SIGKILL, SIGTERM, SIGINT). What are their typical use cases?

    > **Answer**:  SIGKILL(9): force terminates a process immediately and cannot be caught or ignored making it the last
	 > resort when SIGTERM fails
                   SIGTERM(15): Requests graceful termination allowinf processes to clean up before exiting and is the
						 default signal for the kill command. 
                   SIGINT(2): Triggered by Ctrl+C it can be caught or ignored and typically allows programs to terminate
						 gracefully with proper cleanup. 

- What happens when a process receives SIGSTOP? Can it be caught or ignored like SIGINT? Why or why not?

    > **Answer**:  SIGSTOP pauses a process indefinitely, suspending its execution until a SIGCONT signal is received
	 > and is commonly triggered by ctrl+Z in terminals. Unlike most siognals SIGSTOP can not be caught because it is a
	 > fundamental kernel-level process control mechanism needed for job control and debugging. This uncatchable nature
	 > ensures that systems always have a reliable way to suspend processes, which is essential for process management
	 > and debugging tools.

