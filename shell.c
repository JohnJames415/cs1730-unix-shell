#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define BUFFSIZE 4096

/* Retrieve the hostname and make sure that this program is not being run on the main odin server.
 * It must be run on one of the vcf cluster nodes (vcf0 - vcf3).
 */
void check()
{
        char hostname[10];
        gethostname(hostname, 9);
        hostname[9] = '\0';
        if (strcmp(hostname, "csci-odin") == 0) {
                fprintf(stderr, "WARNING: TO MINIMIZE THE RISK OF FORK BOMBING THE ODIN SERVER,\nYOU MUST RUN THIS PROGRAM ON ONE OF THE VCF CLUSTER NODES!!!\n");
                exit(EXIT_FAILURE);
        } // if
} // check

/**
 * This function is called every loop to print the current directory to the screen.
 *
 * @param workingDirectory is the current working directory of the program.
 */
void printcwd(char* workingDirectory) {
    char* home = getenv("HOME");
    int homeIsPresent = 1;
    for (int i = 0; i < strlen(home) && homeIsPresent; i++) {
        if (workingDirectory[i] == '\0' || workingDirectory[i] != home[i]) {
            homeIsPresent = 0;
        }
    }
    if (homeIsPresent) {
        char newPath[BUFFSIZE];
        newPath[0] = '~';
        newPath[1] = '\0';
        for (int i = 0; i < (strlen(workingDirectory) - strlen(home)); i++) {
            newPath[i+1] = workingDirectory[i + strlen(home)];
            if (i == ((strlen(workingDirectory) - strlen(home)) - 1)) {
                newPath[i+2] = '\0';
            }
        }
        printf("%s", newPath);
    } else {
        printf("%s", getcwd(workingDirectory, BUFFSIZE));
    }
}

/**
 * The main method will setup the user's home directory, and allow them to continually
 * enter shell commands until they enter the "exit" command.
 */
int main()
{
	check();
	setbuf(stdout, NULL); // makes printf() unbuffered
	int n;
	char cmd[BUFFSIZE];

	// Project 3 TODO: set the current working directory to the user home directory upon initial launch of the shell
	// You may use getenv("HOME") to retrive the user home directory
    chdir(getenv("HOME"));

	// inifite loop that repeated prompts the user to enter a command
	while (1) {
		printf("1730sh:");

		// Project 3 TODO: display the current working directory as part of the prompt
        // Writes the current working directory to the cwd variable and prints it.
        char cwd[BUFFSIZE];
        getcwd(cwd, BUFFSIZE);
        printcwd(cwd); //Prints the current working directory

		printf("$ ");
		n = read(STDIN_FILENO, cmd, BUFFSIZE);

		// if user enters a non-empty command
		if (n > 1) {
			cmd[n-1] = '\0'; // replaces the final '\n' character with '\0' to make a proper C string

			// Lab 06 TODO: parse/tokenize cmd by space to prepare the
			// command line argument array that is required by execvp().
			// For example, if cmd is "head -n 1 file.txt", then the
			// command line argument array needs to be
			// ["head", "-n", "1", "file.txt", NULL].

            /* To turn the command into a list of arguments, the "cmd" variable is sorted through,
             * and each word is assigned an index in a 2D char array. That 2D char array is then
             * converted into a char pointer array (which can be used by exec functions).
             *
             */
            char parsedCommands2DArray[n][n];
            char* parsedCommands[BUFFSIZE];
            int parsedCommandsIndex = 0;
            char word[n];
            int wordIndex = 0;
            //The loop sorts through cmd and assigns values to 2D array.
            for (int i = 0; i < n; i++) {
                if (cmd[i] == ' ' || i == n-1) {
                    word[wordIndex] = '\0';
                    for (int j = 0; j < strlen(word) + 1; j++) {
                        if (j == strlen(word)) {
                            parsedCommands2DArray[parsedCommandsIndex][j] = '\0';
                        } else {
                            parsedCommands2DArray[parsedCommandsIndex][j] = word[j];
                        }
                    }
                    parsedCommandsIndex++;
                    wordIndex = 0;
                } else {
                    word[wordIndex] = cmd[i];
                    wordIndex++;
                }
            }
            //Converts 2D array into char* array.
            for (int i = 0; i < parsedCommandsIndex; i++) {
                parsedCommands[i] = parsedCommands2DArray[i];
            }
            parsedCommands[parsedCommandsIndex] = NULL;


			// Lab 07 TODO: if the command contains input/output direction operators
			// such as "head -n 1 < input.txt > output.txt", then the command
			// line argument array required by execvp() needs to be
			// ["head", "-n", "1", NULL], while the "< input.txt > output.txt" portion
			// needs to be parsed properly to be used with dup2(2) inside the child process

            //The following code is used to redirect standard input/output.
            int redirectSTDIN = 0; //Boolean int var to detect if stdin has been redirected.
            int redirectSTDOUT = 0; //Boolean int var to detect if stdout has been redirected.
            int redirectSTDOUTAPPEND = 0; //Boolean int var to detect if std out had been redirected in append mode.
            char* inputFile = NULL;
            char* outputFile = NULL;
            //Loops through parsed command array
            for (int i = 0; i < parsedCommandsIndex; i++) {
                if (strcmp(parsedCommands[i], "<") == 0) {
                    redirectSTDIN = 1;
                    parsedCommands[i] = NULL; //Null-terminates array where redirect operator was found.
                    inputFile = parsedCommands[i+1]; //Sets the input file to the following array index after the operator.
                } else if (strcmp(parsedCommands[i], ">") == 0) {
                    redirectSTDOUT = 1;
                    parsedCommands[i] = NULL;
                    outputFile = parsedCommands[i+1];
                } else if (strcmp(parsedCommands[i], ">>") == 0) {
                    redirectSTDOUTAPPEND = 1;
                    parsedCommands[i] = NULL;
                    outputFile = parsedCommands[i+1];
                }
            }

			// Lab 06 TODO: if the command is "exit", quit the program

			// Project 3 TODO: else if the command is "cd", then use chdir(2) to
			// to support change directory functionalities

			// Lab 06 TODO: else, for all other commands, fork a child process and let
			// the child process execute user-specified command with its options/arguments.
			// NOTE: fork() only needs to be called once. DO NOT CALL fork() more than one time.
            if (strcmp(parsedCommands[0], "exit") == 0) {
                exit(0);
            } else if (strcmp(parsedCommands[0], "cd") == 0) { //Triggers when cd is the provided command
                if ((parsedCommands[1] == NULL) || (strcmp(parsedCommands[1], "~")) == 0) { //Triggers with "cd" or "cd ~"
                    chdir(getenv("HOME"));
                } else if (strcmp(parsedCommands[1], "..") == 0) { //Triggers with "cd .."
                    int slashCounter = 0;
                    int newSlashCounter = 0;
                    int hasBeenNullTerminated = 0;
                    char newPath[BUFFSIZE];
                    //Below loop counts the number of '/' in the current directory path.
                    for (int i = 0; i < strlen(cwd); i++) {
                        if (cwd[i] == '/') {
                            slashCounter++;
                        }
                    }
                    // Below loop sets newPath variable equal to the original path, but one directory higher.
                    for (int i = 0; i < strlen(cwd); i++) {
                            if(cwd[i] == '/') {
                                newSlashCounter++;
                            }
                            if(newSlashCounter < slashCounter) {
                                newPath[i] = cwd[i];
                            } else if (!hasBeenNullTerminated) {
                                newPath[i] = '\0';
                                hasBeenNullTerminated = 1;
                            }
                    }

                    chdir(newPath); //Sets the directory equal to the new path.
                } else if (strcmp(parsedCommands[1], "/usr/bin") == 0) { // Triggers with "cd /usr/bin"
                    chdir("/usr/bin");
                } else { //Triggers when provided with any other directory.
                    char tilde = parsedCommands[1][0];
                    if (tilde == '~') { //Triggers when the cd command is of the form "cd ~/mydir"
                        //Sets directory path equal to: home + '/' + additional directories.
                        char newPath[BUFFSIZE];
                        char* home = getenv("HOME");
                        for (int i = 0; i < strlen(home); i++) {
                            newPath[i] = home[i];
                        }
                        newPath[strlen(home)] = '/';
                        for (int i = 0; i < strlen(parsedCommands[1]) - 2; i++) {
                            newPath[i+1+strlen(home)] = parsedCommands[1][i+2];
                            if (i == strlen(parsedCommands[1]) - 3) {
                                newPath[i+2+strlen(home)] = '\0';
                            }
                        }
                        chdir(newPath);
                    } else { //Triggers when the cd command is of the form "cd mydir"
                        //Sets directory path equal to: current path + '/' + additional directories.
                        char newPath[BUFFSIZE];
                        for (int i = 0; i < strlen(cwd); i++) {
                            newPath[i] = cwd[i];
                        }
                        newPath[strlen(cwd)] = '/';
                        for (int i = 0; i < strlen(parsedCommands[1]); i++) {
                            newPath[i+1+strlen(cwd)] = parsedCommands[1][i];
                            if (i == strlen(parsedCommands[1]) - 1) {
                                newPath[i+2+strlen(cwd)] = '\0';
                            }
                        }
                        chdir(newPath);
                    }
                }

            } else { //Triggers when provided with a command (that is not exit or cd)
                int pid;
                int fileDescSTDIN;
                int fileDescSTDOUT;
                if ((pid = fork()) < 0) { //Creates child process to execute user command
                    perror("fork");
                } else if (pid == 0) { //Triggers for child processes
                    // Lab 07 TODO: inside the child process, use dup2(2) to redirect
                    // standard input and output as specified by the user command

                    //Redirects standard input if applicable.
                    if (redirectSTDIN) {
                        fileDescSTDIN = open(inputFile, O_RDONLY);
                        dup2(fileDescSTDIN, STDIN_FILENO);
                    }

                    //Redirects standard output if applicable.
                    if (redirectSTDOUT) {
                        fileDescSTDOUT = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
                        dup2(fileDescSTDOUT, STDOUT_FILENO);
                    } else if (redirectSTDOUTAPPEND) {
                        fileDescSTDOUT = open(outputFile, O_WRONLY | O_CREAT | O_APPEND, 0666);
                        dup2(fileDescSTDOUT, STDOUT_FILENO);
                    }
                    // Lab 06 TODO: inside the child process, invoke execvp().
                    // if execvp() returns -1, be sure to use exit(EXIT_FAILURE);
                    // to terminate the child process
                    if (execvp(parsedCommands[0], parsedCommands) == -1) {
                        perror("execvp");
                        return EXIT_FAILURE;
                    }
                } else { //Triggers for parent process
                    // Lab 06 TODO: inside the parent process, wait for the child process
                    // You are not required to do anything special with the child's
                    // termination status

                    //waits on child process
                    int status;
                    wait(&status);
                }
            }
		} // if
	} // while
} // main
