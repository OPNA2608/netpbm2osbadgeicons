/*
 * netpbm2osbadgeicons - Convert PPM & PGM files into PowerMac <OS-BADGE-ICONS> bootinfo data
 * Copyright (C) 2026  Cosima Neidahl

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#define _XOPEN_SOURCE 700
#include <stdio.h>

#include <sys/wait.h>
#include <unistd.h>

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

		return (childExitStatus != VALGRIND_ERROR_STATUS) ? 0 : 3;
	} else {
		// Failed to work
		fprintf (stderr, "Failed to fork!\n");
		return 2;
	}

	return 0;
}
