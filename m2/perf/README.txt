We know that "strace -c command" will display the time used by every system call.

In this experiment, we need to achieve similar features of "strace -c command".

You can get source code in perf.c

We fork a child process, and child process invokes execv("/usr/bin/strace", ....) to get the time used by every system call. 

Then, child process sends the information of time to father process with pipe.


run:
	
	./perf command arg1 arg2 ...

Be careful:

	The command can also be a executable file.

	The program will find the executable file in current path first.
	
	If program can't find file in current path, it will then find file with environment variable PATH.
