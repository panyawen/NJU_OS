We know that "strace -c command" will display the time used by every system call.

In this experiment, we need to achieve similar features of "strace -c command".

You can get source code in perf.c

We fork a child process, and invoke execv("/usr/bin/strace", ....) to get the time used by every
system call. 

Then, we send the information about time to father process through pipe.


run:
	
	./perf command arg1 arg2 ...

Be careful:

	The command can also be a executable file.

	We will find the file in current path first.
	
	If program can't find file in current path, it will then find file in environment variables
	PATH.
