#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <unistd.h>

#include <sys/wait.h>

#define VALGRIND_ERROR_STATUS 205

/*
 * argv[1]: path to valgrind
 * argv[2]: path to netpbm2osbadgeicons
 * argv[3..n]: args to netpbm2osbadgeicons
 */
int main (int argc, char** argv) {
	pid_t childPid;

	childPid = fork();

	if (childPid == 0) {
		// child process
		execv (argv[1], &argv[1]);
	} else if (childPid > 0) {
		// parent process
		int childStatus = 0;
		int childExitStatus = VALGRIND_ERROR_STATUS;

		waitpid (childPid, &childStatus, 0);

		if (WIFEXITED (childStatus)) {
			childExitStatus = WEXITSTATUS (childStatus);
			printf ("Valgrind exited with status %i\n", childExitStatus);
		}

		return
			(childExitStatus != VALGRIND_ERROR_STATUS)
			? 0
			: 3;
	} else {
		// Failed to work
		fprintf (stderr, "Failed to fork!\n");
		return 2;
	}

	return 0;
}
