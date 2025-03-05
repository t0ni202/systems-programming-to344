1. Your shell forks multiple child processes when executing piped commands. How does your implementation ensure that all child processes complete before the shell continues accepting user input? What would happen if you forgot to call waitpid() on all child processes?

My implementation waits for every child process by storing each child’s PID (or iterating over the forked processes) and then calling waitpid() for each one before returning to the main loop. This ensures that the shell does not prompt for new input until all the piped commands have completed. If waitpid() isn’t called on all child processes, they become zombie processes, consuming system resources, and the shell might start a new command sequence while previous processes are still running, leading to resource leaks and unpredictable behavior.

2. The dup2() function is used to redirect input and output file descriptors. Explain why it is necessary to close unused pipe ends after calling dup2(). What could go wrong if you leave pipes open?
After dup2() duplicates a file descriptor to STDIN or STDOUT, the original pipe end remains open. It is essential to close these unused ends to free system resources and to ensure that the pipe signals EOF properly. If unused pipe ends remain open, the receiving process might never detect an end-of-file (EOF) condition, causing it to hang waiting for more input, or it could lead to file descriptor leaks that eventually exhaust available descriptors.

3. Your shell recognizes built-in commands (cd, exit, dragon). Unlike external commands, built-in commands do not require execvp(). Why is cd implemented as a built-in rather than an external command? What challenges would arise if cd were implemented as an external process?

The cd command must change the current working directory of the shell process itself. If it were implemented as an external command, it would run in a child process and any directory change would only affect that child. Once the child exits, the parent shell’s directory remains unchanged. This would render the command ineffective and could confuse users expecting the shell’s state to change.

4. Currently, your shell supports a fixed number of piped commands (CMD_MAX). How would you modify your implementation to allow an arbitrary number of piped commands while still handling memory allocation efficiently? What trade-offs would you need to consider?

To support an arbitrary number of piped commands, i  would:
	•	Dynamically allocate (or resize) the array of command buffers and pipe file descriptors instead of using a fixed-size array.
	•	Use data structures like linked lists or dynamic arrays (via realloc) to store commands.

Trade-offs include:
	•	Increased complexity in managing dynamic memory (ensuring proper allocation, resizing, and cleanup).
	•	Potential performance overhead due to frequent memory reallocations.
	•	The need to consider system limits on the number of file descriptors and processes, as well as maintaining robust error handling.
