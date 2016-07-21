/*********************************************************************************************************
* Tatyana Vlaskin (vlaskint@onid.oregonstate.edu)
* OregonState EECS
* Filename: smallsh.c
*
* Description: This program is a simplified unix shell capable of both executing built in commands:
* 1. exit
* 2. cd
* 3. status
* 4. The program  also supports comments, which are line beginning with the # character
* 5. The general syntax of command line: command [arg1 arg2 ...] [< input_file] [> output_file] [&] , where items in square brackets are optional
* 6. The special symbols <, >, and & are recognized, but they must be surrounded by spaces like other words.
* 7. If the command is to be executed in the background, the last word must be &.
* 8. If standard input or output is to be redirected, the > or < words followed by a filename word must appear after all the arguments.  Input redirection can appear before or after output redirection.
* 9. Shell support command lines with a maximum length of 2048 characters, and a maximum of 512 arguments.
*10. Shell does not  support any quoting; so arguments with spaces inside them are not possible.
*11. There is no error checking on the syntax of the command line.
* Motivation: This program is an assignment for the
* operating systems course at OSU. The goal is to create
* an interactive shell with basic functionality in the C
* programming language. This assignment teaches several
* unix api functions, and basics of OS interaction.
* ADOPTED FROM: https://github.com/joelpet/SmallShell/blob/master/smallshell.c
* and https://github.com/mold/SmallShell/blob/master/smallshell.c
* and see other sources in the code comments
******************************************************************************************************************/
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <termios.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#define MAX_ARGUMENTS 512
#define MAX_CHARACTERS 2048
#define MAX_STATUS_CHARACTERS 2048

 /*************************************************************************************************************
 * Function:  int substring_Finder(char *search_String, char *substering_Of_Interst)
 * Description: Function to search for a specific substring in a string
 * Parameters: sting and substring
 * returns 1 - if substring was found
 * returns 2 - if substring was not founf\d
 * taken from http://articles.leetcode.com/2010/10/implement-strstr-to-find-substring-in.html
 ***************************************************************************************************************/
int substring_Finder(char *search_String, char *substering_Of_Interst);

 /*************************************************************************************************************
 * Function:  void file_Name(char *input_Command, char *file_Name_Entered)
 * Description: Function that takes a user entered command and returns a file name
 * that the user entered in case there is a redirection
 * we use user defined helper function: (substring_Finder(string, substring),to check if the entered command
 * has < or > substrings
 * Parameters: users command and variable that will store the file name
 ***************************************************************************************************************/
void file_Name(char *input_Command, char *file_Name_Entered);

/*************************************************************************************************************
 * Function:  void split_Users_Command_into_Arguments(char *input_Command, char **command_Arguments)
 * Description: Function break users input into individual words and stores those words in the array
 * The function also checks for the presence of the <, >, and $ charactesr and does not include
 * them in the array. The function uses strtok() C function and uses white space as a delimiter.
 * code adopted from  http://codereview.stackexchange.com/questions/42990/tokenizing-string-using-strtok
 * http://faq.cprogramming.com/cgi-bin/smartfaq.cgi?answer=1061423302&id=1044780608
 ***************************************************************************************************************/
void split_Users_Command_into_Arguments(char *input_Command, char **command_Arguments);

 /******************************************************************************************************************
 * Function:  int foreground_Command(char *input_Command, char *status_Message)
 * Description: Function that will run specified foreground command
 * Function will return 0 if the command executed without any error.
 * Parameters: command that the user have entered and that does NOT have an & sign
 *  *  ***************************************************************************************************************/
int foreground_Command(char *input_Command, char *status_Message);

 /***************************************************************************************************************
 * Function:  int background_Command(char *input_Command)
 * Description: Function that will run specified background command
 * Function will return 0 if the command executed without any error.
 * Parameters: command that the user have entered and that has an & sign
 *  ***************************************************************************************************************/
int background_Command(char *input_Command);

 /*************************************************************************************************************
 * Function:  static void signal_Child_Handler (int sig){
 * Description: Function that will monitor the child signals(when SIGCHLD signal is received by parent) and
 * determining which termination method or completion exit value to display. This is the void handler function for the
 *
 *       struct sigaction {
 *              void     (*sa_handler)(int);
 *              void     (*sa_sigaction)(int, siginfo_t *, void *);
 *              sigset_t   sa_mask;
 *              int        sa_flags;
 *              void     (*sa_restorer)(void);
 *          };
 * this function is required for int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact)
 * function, which will be used in the main function to change actions taken by a process on receipts of specific signal
 * Function will return 0 if the command executed without any error.
 *  function adopted from https://github.com/swanyriver/small-shell/blob/master/prepare.c
 * https://github.com/ianks/shell-lab/blob/master/tsh.cc
 * http://man7.org/tlpi/code/online/diff/procexec/multi_SIGCHLD.c.html
 * http://www.linuxprogrammingblog.com/code-examples/SIGCHLD-handler
 * https://www.youtube.com/watch?v=ls5cGi12kGw
 * https://github.com/mptcp-nexus/android_system_netd/blob/master/main.cpp
 * http://openverse.com/~andy/devel/old/eat/src/pty.c
 * http://www.cs.rit.edu/usr/local/pub/wrc/courses/sp1/textbooks/tlpi-dist/procexec/multi_SIGCHLD.c
 * https://www.youtube.com/watch?v=M-qtkcLQJG0
 * https://github.com/esmil/lem/blob/master/lem/signal/core.c
 * ***************************************************************************************************************/
static void signal_Child_Handler (int sig);


/******************************************************************************************************************
MAIN FUNCTION
 * ****************************************************************************************************************/
int main(){
    int exit_Shell_Request = 0; // 0-run shell, 1-exit shell
	int status_Exit_Value = 0; //for status exit value
	char user_Input[MAX_CHARACTERS] = ""; //for users command
	char status_Message[MAX_STATUS_CHARACTERS] = "";
	// Set up a signal handler to deal with signals from child processes
	// this code is taken from http://pubs.opengroup.org/onlinepubs/009695399/functions/sigaction.html
	struct sigaction act; //creating a structure variable, which will be called in sigaction function with the conrol signal variable
	act.sa_handler = signal_Child_Handler; // this is declaring which handler is used if controll signal is passed to the structure
	//this command used to change the actions taken by a process on receipt of specific signal
	//int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact);
	//signum specifies the signal and can be any valid signal except SIGKILL and SIGSTOP
	//act is a non_NULL, the new actions for signal signum is snstalled from act.{action to be taken}
	//the signacture structure definition can be found on http://man7.org/linux/man-pages/man2/sigaction.2.html
	//The structure sigaction, used to describe an action to be taken, is defined in the <signal.h> header
    //https://www.youtube.com/watch?v=nj6r47F6cNQ -- evenhandling lecture
    //https://www.youtube.com/watch?v=ahRBRGVTi5w ---sigaction() lecture
    //https://www.youtube.com/watch?v=rggw61JtGz0
    //Catching SIGCHLD
    //When a child process stops or terminates, SIGCHLD is
    //sent to the parent process. The default response to the signal
    //is to ignore it. The signal can be caught and the exit status
    //from the child process can be obtained by immediately calling wait
    //if our case will will catch a child that is terminated and call a child handling function
	sigaction(SIGCHLD, &act, NULL);
    while (exit_Shell_Request == 0){
         fflush(stdin);//had to add this, see canvas discussion
		// Clear stdin
		//int tcflush(int fileDescriptor, int queue);
		//tcflush is called by a process to flush all input that has received but not yet been read by a terminal,
		//or all output written but not transmitted to the terminal. Flushed data are discarded and cannot be retrieved.
		//TCIFLUSH: flushes data received but not read.
		//http://www.qnx.com/developers/docs/6.5.0/index.jsp?topic=%2Fcom.qnx.doc.neutrino_lib_ref%2Ft%2Ftcflush.html
		//this step is neccessary to make sure that we do not receive or send data from previous usage of the port
		tcflush(0, TCIFLUSH);
		//The C library function char *strncpy(char *dest, const char *src, size_t n) copies up
		//to n characters from the string pointed to, by src to dest. In a case where the length
		//of src is less than that of n, the remainder of dest will be padded with null bytes.
		// at this point we clear the users input variable and make it an empty string for they type a new command
		strncpy(user_Input, "", MAX_CHARACTERS - 1);
		//we use fflush(stdout) to ensure that whatever you just wrote in a file/the console is indeed written out on disk/the console right away
        //The reason is that actually writing, whether to disk, to the terminal, or pretty much anywhere else, is pretty slow.
        //Further, writing 1 byte takes roughly the same time as writing, say, a few hundred bytes[1]. Because of this, data you write to
        //a stream is actually stored in a buffer which is flushed when it is full or when you call  fflush.
        // Calling fflush means  that you flush the buffer when you need the output right away..
        //on canvas per Benjamin Brewster Before you display your prompt, call fflush(). :)
		fflush(stdout);
        // Get user input and sends formated output to the screen
		printf(": ");
		//Reads characters from stream and stores them as a C string into str until (num-1) characters have been
		//read or either a newline or the end-of-file is reached, whichever happens first.
		//in our case, we take string that the user entered using the keyboard and store it in the variable user_Input
		fgets(user_Input, 100, stdin);
		fflush(stdout);
        // remove new line character from the string and replace it with NUll character
        size_t ln = strlen(user_Input) - 1;
        if (user_Input[ln] == '\n') {
            user_Input[ln] = '\0';
        }
		// If you at the end of an input file, then exit.
		//Check End-of-File indicator
        //Checks whether the End-of-File indicator associated with stream is set, returning a value different from zero if it is.
        //if the end of the file was reached we would want to exit
		//taken from discussion board
		  if (feof(stdin)) {       // if end-of-file reached, then we are in a script
            // http://stackoverflow.com/questions/23978444/c-redirect-stdin-to-keyboard
            //printf("End of the file!\n");
            if (!freopen("/dev/tty", "r", stdin)) {        // re-open stdin as a terminal
            perror("/dev/tty");
            exit(1);
            }
        }
		// if the user accidentally pressed enters without entering any commands
		//we would need to restart the loop, so we set the user_input variable back to zero
		//https://github.com/smd519/Networking_Basics/blob/eb8f299a7302f3aeca48162bec9c71c88bfe632c/My_FTP_Protocol/create_command.c
		if (strcmp(user_Input, "") == 0)	{
		   // printf("Blank line!\n");
			continue;
		}
		///if the user enters CD, which is the indication that they want to go to home directory
		// check for the cd command. If the user entered cd and
		// did not indicate which directory they want to do to, it would
		//take them to the home directory
		//so we need to figure out how to get to get a home directroy path
		if (strcmp(user_Input, "cd") == 0){
		    //printf("you typed cd!\n");
        //the following code was found on the https://github.com/joelpet/SmallShell/blob/master/smallshell.c#L216
        //i had no idea what that function meant
        //http://www.tutorialspoint.com/c_standard_library/c_function_getenv.htm
        //The getenv() function shall search the environment of the calling process for the environment
         //variable name if it exists and return a pointer to the value of the environment variable.
        char* home_Path = getenv("HOME");
  		// once the home directory is found, we change the current directory to the home directory
        chdir(home_Path);
        continue;
		}
		///if the user enters CD DIRECTORY_NAME for the command this is an indication that they want to go to a specific directory
		// in case the user does not want to go to the current directory, but specifies the name of the
		//directory that they want to
		if ((user_Input[0] == 'c') && (user_Input[1] == 'd') && (user_Input[2] == ' ')){
		   // printf("you typed cd plus something else\n");
			char *arguments_Array[MAX_ARGUMENTS];
			//initialize array to null
            int i;
            for(i = 0; i < MAX_ARGUMENTS; i++){
                arguments_Array[i] = 0;
            }
		// Get the users input and split it into words, see inforamtion on user defined functin for additional information
			split_Users_Command_into_Arguments(user_Input, arguments_Array);
			// the name of the directory will be stored at the index = 1 in the array that was
			//derived from string tokanization into words, thus we change a directory to the name that is sroted under the
			//index = 1. Index 0 will be the word cd.
			chdir(arguments_Array[1]);
			continue;
		}
		/// if the user enters word STATUS for the command
		if (strcmp(user_Input, "status") == 0){
		   // printf("you typed status\n");
			if (strncmp(status_Message, "", MAX_STATUS_CHARACTERS) == 0){
				printf("exit value %d\n", status_Exit_Value);
			}
			else{
				// If there was an error message, then show that instead of the regular status message
				printf("%s\n", status_Message);
			}
			// Clear the status
			strncpy(status_Message, "", MAX_STATUS_CHARACTERS);
			status_Exit_Value = 0;
			continue;
		}
        ///if the user enters # SING for commenting
		// if the user entered a comment we would want to restart a loop as well
		//this will let the user to enter a command
		if (user_Input[0] == '#'){
		   // printf("Comment sign #!\n");
			continue;
		}
		///if the user enters EXIT command
		// check for the exit command
		if (strcmp(user_Input, "exit") == 0){
		   // printf("You wanted to exit!\n");
		    exit_Shell_Request = 1;
			exit(0);
		}
		else{
			// Clean up the status if we didn't want to display it
			strncpy(status_Message, "", MAX_STATUS_CHARACTERS);
			status_Exit_Value = 0;
		}

		/// if the user enters & at the end of their command this is an indication that they want to
		///do a background process
		if (substring_Finder(user_Input, "&")){
            //printf("bachground process\n");
			background_Command(user_Input);
			continue;
		}
		//  foreground command
		//printf("foregroud process!\n");
		status_Exit_Value = foreground_Command(user_Input, status_Message);
	}
	return 0;
}
/*************************************************************************************************************
 * Function:  int foreground_Command(char *input_Command, char *status_Message)
 * Description: Function that will run specified foreground command
 * Function will return 0 if the command executed without any error.
 * Parameters: command that the user have entered and that does NOT have an & sign
 * function adopted from or error message
 * https://www.youtube.com/watch?v=l64ySYHmMmY ---- use this for a makefile as well
 *  ***************************************************************************************************************/

int foreground_Command(char *input_Command, char *status_Message){
	int status = 0;
	int status_Value = 0;
	pid_t pid_After_Fork = -5; // per lecture notes, set it to -5, why? who knows- I really do not get this why -5?
	int fd = -1;
	// signal handler for the child, see main method for explanation
	struct sigaction act;
	//variable for the filename
	char fileName[MAX_CHARACTERS] = "";
	// Determine if we have any redirects
	int output_Redirect = substring_Finder(input_Command, ">");
	int input_Redirect = substring_Finder(input_Command, "<");
	char *argv[MAX_ARGUMENTS];
	int i;
	//initialize array to null
	for(i = 0; i < MAX_ARGUMENTS; i++){
		argv[i] = 0;
	}
	// Get the file descriptor if we have to redirect output
	if (output_Redirect == 1)	{
		file_Name(input_Command, fileName);// this will give us the name of the file
        //the redirected output file should be opened for write only
		//it should be truncated if it already exists or created if it does not exist.
		//if the shell cannot open the output file, it should print an error message and set exit status to 1
		//O_TRUNC mode if you want to truncate an existing file and overwrite it with new data.
		//O_WRONLY   Open for writing only
		//O_CREAT   If the file exists, this flag has no effect except as noted under O_EXCL below. Otherwise, the file shall be created;
		//0644 will create a file that is Read/Write for owner, and Read Only for everyone else..
		fd = open(fileName, O_WRONLY|O_TRUNC|O_CREAT, 0644);
	}

	// Get the file descriptor if we have to redirect input
	if (input_Redirect == 1)	{
		file_Name(input_Command, fileName);// this will give us the name of the file
        // Get the file descriptor if we have to redirect input
        //if there is an input redirect character " <"
        //O_RDONLY the redirected input file will be opened for reading only
        //0644 will create a file that is Read/Write for owner, and Read Only for everyone else..
		fd = open(fileName, O_RDONLY, 0644);
	}
	// Get all the args from the user entered command
	split_Users_Command_into_Arguments(input_Command, argv);
	// Start the child process for command execution
	pid_After_Fork = fork();
	//see lecture https://www.youtube.com/watch?v=EqndHT606Tw
	//0-- stdin
	//1---stdout
	//2---srderror
	switch (pid_After_Fork){
		case -1:
			// Fork failed
			perror("fork()error"); //not sure if we need this
			exit(1);
			break;
		case 0:
			/// CHILD
			// Establish the std output redirect, exit if error is found
			//http://pubs.opengroup.org/onlinepubs/009695399/functions/dup.html
			//int dup2 (int old, int new)---This function copies the descriptor old to descriptor number new.
			if ((output_Redirect) && (dup2(fd, 1) < 0)){
				exit(1);
			}

			// Establish the std input redirect, exit if error is found.
			if ((input_Redirect) && (dup2(fd, 0) < 0)){
				printf("smallsh: cannot open %s for input\n", fileName);
				exit(1);
			}
			// Set up the signal handler for the child process to not ignore termination signals
			//SIG_DFL specifies the default action for the particular signal
			act.sa_handler = SIG_DFL;
            //Catching SIGCHLD
            //When a child process stops or terminates, SIGCHLD is
            //sent to the parent process. The default response to the signal
            //is to ignore it. The signal can be caught and the exit status
            //from the child process can be obtained by immediately calling wait
            //if our case will will catch a child that is terminated and call a child handling function
			sigaction(SIGINT, &act, NULL);
 	 		close(fd);
        // Try to execute the user command
        //first elements of the array is the name of the file
        //http://www.cs.ecu.edu/karl/4630/sum01/example1.html
        //http://stackoverflow.com/questions/14301407/how-does-execvp-run-a-command
        //http://www.xinotes.net/notes/note/1829/
        //http://www.opennet.ru/man.shtml?topic=execvp&category=2&russian=4
        //The first argument, by convention, should point to the filename associated with the file being executed. The array of pointers must be terminated by a NULL pointer.
  		execvp(argv[0], argv);
  		printf("%s: no such file or directory\n", argv[0]);
  		exit(1);
        break;
		default:
			///PARENT
			close(fd);
			// Set up the signal handler for the parent process to ignore termination messages
			//SIG_DFL specifies the default action for the particular signal
			//SIG_IGN specifies that the signal should be ignored.
			act.sa_handler = SIG_DFL;
			act.sa_handler = SIG_IGN;
			sigaction(SIGINT, &act, NULL);
			// Wait for child process to finish
            //Catching SIGCHLD
            //When a child process stops or terminates, SIGCHLD is
            //sent to the parent process. The default response to the signal
            //is to ignore it. The signal can be caught and the exit status
            //from the child process can be obtained by immediately calling wait
            //if our case will will catch a child that is terminated and call a child handling function
			waitpid(pid_After_Fork, &status, 0);
			//http://www-01.ibm.com/support/knowledgecenter/SSB23S_1.1.0.11/com.ibm.ztpf-ztpfdf.doc_put.11/gtpc2/cpp_wexitstatus.html?cp=SSB23S_1.1.0.11%2F0-3-8-1-0-17-3
			//we obtain exit stutus of the child
			status_Value = WEXITSTATUS(status);
            //Query status to see if a child process ended abnormally
			// Save the appropriate signal error message
			//http://www.qnx.com/developers/docs/6.5.0/index.jsp?topic=%2Fcom.qnx.doc.neutrino_lib_ref%2Fs%2Fsnprintf.html
			if(WIFSIGNALED(status)) {
                //Determine which signal caused the child process to exit.
				int signal_Number = WTERMSIG(status);
				char terminateMsg[MAX_STATUS_CHARACTERS];
				char signal_Number_String[25];
                //Write formatted output to a character array, up to a given maximum number of characters
   				snprintf(signal_Number_String, sizeof(signal_Number_String), "%d", signal_Number);

                // Output the termination message and save it for the status command
				strncpy(terminateMsg, "terminated by signal ", MAX_STATUS_CHARACTERS);
				strcat(terminateMsg, signal_Number_String);
				printf("%s\n", terminateMsg);
				strncpy(status_Message, terminateMsg, MAX_STATUS_CHARACTERS);
			}

			break;
	}

	return status_Value;
}

 /*************************************************************************************************************
 * Function:  int background_Command(char *input_Command)
 * Description: Function that will run specified background command
 * Function will return 0 if the command executed without any error.
 * Parameters: command that the user have entered and that has an & sign
 * function adopted from https://github.com/swanyriver/small-shell/blob/master/prepare.c
 * https://www.youtube.com/watch?v=xVSPv-9x3gk
 *  ***************************************************************************************************************/

int background_Command(char *input_Command){
	int status_Value = 0;
	pid_t pid_After_Fork = -5; //taken from lecture
	char pid_After_Fork_String[25];
	int fd = -1;
	//variable for filename
	char fileName[MAX_CHARACTERS] = "";
	// Check if we have any redirects
	int output_Redirect = substring_Finder(input_Command, ">");
	int input_Redirect = substring_Finder(input_Command, "<");

	char *argv[MAX_ARGUMENTS];
    //initialize array to null
	int i;
	for(i = 0; i < MAX_ARGUMENTS; i++){
		argv[i] = 0;
	}
	// Get the file descriptor if we have to redirect output
	//first we check if there is a output redirect character ">"
	if (output_Redirect == 1)	{
		file_Name(input_Command, fileName);// we take the name that the user indicated and store it in the filename variable
		//the redirected output file should be opened for write only
		//it should be truncated if it already exists or created if it does not exist.
		//if the shell cannot open the output file, it should print an error message and set exit status to 1
		//O_TRUNC mode if you want to truncate an existing file and overwrite it with new data.
		//O_WRONLY   Open for writing only
		//O_CREAT   If the file exists, this flag has no effect except as noted under O_EXCL below. Otherwise, the file shall be created;
		//0644 will create a file that is Read/Write for owner, and Read Only for everyone else..
		fd = open(fileName, O_WRONLY|O_TRUNC|O_CREAT, 0644);
	}

	// Get the file descriptor if we have to redirect input
	//if there is an input redirect character " <"
	//O_RDONLY the redirected input file will be opened for reading only
	if (input_Redirect == 1){
		file_Name(input_Command, fileName);//we take the name of the file that the user indicated and store it in the filename variable
		fd = open(fileName, O_RDONLY, 0644);
	}
	//if the user did not specify redirection, redirect stdin to dev/null
	//http://unix.stackexchange.com/questions/163352/what-does-dev-null-21-mean-in-this-article-of-crontab-basics
	//dev/null is a black hole where any data sent, will be discarded
	else{
		fd = open("/dev/null", O_RDONLY);
		fflush(stdin);
	}

	// Get the args from the user entered string
	split_Users_Command_into_Arguments(input_Command, argv);

	// Start the child process for command execution
	//System call fork() is used to create processes. It takes no arguments and returns a process ID.
	//The purpose of fork() is to create a new process, which becomes the child process of the caller.
	// After a new child process is created, both processes will execute the next instruction following the fork() system call
	//code taken from http://stackoverflow.com/questions/23036475/program-of-forking-processes-using-switch-statement-in-c
	pid_After_Fork = fork();
	switch (pid_After_Fork){
		case -1:// Error in forking, we will exit.
			exit(1);
			break;
        //https://www.cs.rutgers.edu/~pxk/416/notes/c-tutorials/dup2.html
        //We get a file name from the command line and create it as a new file, getting a file descriptor for it.
        //We then write something to the standard output using printf and the stdio library.
        //After that, we use dup2 to copy the file descriptor for the new file (newfd) onto the standard output
        //file descriptor (1). Any printf functions will continue to go to the standard output,
        //but that has now been changed to the file we just opened.
		case 0:// This code will be executed by the child process
		    //if there is an output redirect and if and the redirect standard output to the file using dup2 is successful
			if ((output_Redirect) && (dup2(fd, 1) < 0)){
				// Establish the std output redirect, exit if error is found.
				exit(1);
			}
			else if ((dup2(fd, 0) < 0)){// Establish the std input redirect, exit if error is found.
				printf("smallsh: cannot open %s for input\n", fileName);
				exit(1);
			}
   	 		close(fd);// Execute the user's command
            //execution. As a result, once the specified program file starts its execution, the original program in the
            //caller's address space is gone and is replaced by the new program.
            //this is done to make sure that child does not run the same proccess of the parent
            //http://www.csl.mtu.edu/cs4411.ck/www/NOTES/process/fork/exec.html
	  		execvp(argv[0], argv);
	  		// if the name indicated by the user does not exist, the error message is displayed letting the user know that file does not exist
	  		printf("%s: no such file or directory\n", argv[0]);
	  		exit(1);
			break;
		default:// This code will be run by the parent process.
			close(fd);// Output the process ID message for background processes
			//when a background process terminates, a message showing the process id and exit status will be printed
			//snprintf is essentially a function that redirects the output of printf to a buffer.
			snprintf(pid_After_Fork_String, sizeof(pid_After_Fork_String), "%d", pid_After_Fork);
			//message is printed
			printf("background pid is %s\n", pid_After_Fork_String);
			break;
	}
	return status_Value;
}

/*************************************************************************************************************
 * Function:  static void signal_Child_Handler (int sig){
 * Description: Function that will monitor the child signals(when SIGCHLD signal is received by parent) and
 * determining which termination method or completion exit value to display. This is the void handler function for the
 *
 *       struct sigaction {
 *              void     (*sa_handler)(int);
 *              void     (*sa_sigaction)(int, siginfo_t *, void *);
 *              sigset_t   sa_mask;
 *              int        sa_flags;
 *              void     (*sa_restorer)(void);
 *          };
 * this function is required to user int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact)
 * function, which will be used in the main function to change actions taken by a process on receipts of specific signal
 * Function will return 0 if the command executed without any error.
 * Parameters: command that the user have entered and that has an & sign
 * function adopted from https://github.com/swanyriver/small-shell/blob/master/prepare.c
 * https://github.com/ianks/shell-lab/blob/master/tsh.cc
 * http://man7.org/tlpi/code/online/diff/procexec/multi_SIGCHLD.c.html
 * http://www.linuxprogrammingblog.com/code-examples/SIGCHLD-handler
 * https://www.youtube.com/watch?v=ls5cGi12kGw
 * https://github.com/mptcp-nexus/android_system_netd/blob/master/main.cpp
 * http://openverse.com/~andy/devel/old/eat/src/pty.c
 * http://www.cs.rit.edu/usr/local/pub/wrc/courses/sp1/textbooks/tlpi-dist/procexec/multi_SIGCHLD.c
 * https://www.youtube.com/watch?v=M-qtkcLQJG0
 * ***************************************************************************************************************/
static void signal_Child_Handler (int sig){
	int status;
	//The pid_t data type represents process IDs
	pid_t pid_Child;
	// Close all zombie child processes by waiting for it.
    // Wait for all dead processes.
    // We use a non-blocking call to be sure this signal handler will not
	// block if a child was cleaned up in another part of the program.
	//this will wait untill no more dead children are found

	while ((pid_Child = waitpid(-1, &status, WNOHANG)) > 0){
		// Convert the child pid to a string
		char pid_After_Fork_String[10];
		snprintf(pid_After_Fork_String, sizeof(pid_After_Fork_String), "%d", pid_Child);
		// Declare the message variable
		char child_Terminating_Message[MAX_STATUS_CHARACTERS];
		///When background process terminates, a message showing the proccess id and exit status is displayed
		///CODE TO GET AND DISPLAY PROCCESS ID
		// Construct the child terminate message
		//  Example: "background pid 5253 is done: exit value is displayed"
		strncpy(child_Terminating_Message, "\nbackground pid ", MAX_STATUS_CHARACTERS);
		//concatenate background message with the pdi and the rest of the background message
		strcat(child_Terminating_Message, pid_After_Fork_String);
		strcat(child_Terminating_Message, " is done: ");
		///CODE TO GET AND DISPLAY EXIT STATUS
		//we need to determine the reason of the termination and display appropriate message
		// If child was terminated by a signal, then display the correct message
		//this is taken from the Eric Anderson on the discussion board
		//http://man7.org/linux/man-pages/man2/wait.2.html
		//http://manpages.ubuntu.com/manpages/hardy/man2/wait.2.html
		//  WIFSIGNALED(status)  returns true if the child process was terminated by a signal.
		if(WIFSIGNALED(status)) {
            ///GET A NUMBER OF SIGNAL THAT TERMINATED THE PROCESS
            // WTERMSIG(status)
            // returns the number of the signal that caused the child process
            // to terminate.  This macro should be employed only if
            // WIFSIGNALED returned true
            int signal_Number = WTERMSIG(status);
            char signal_Number_String[10]; //variable that will store the string once the signal number is converted to a string
            snprintf(signal_Number_String, sizeof(signal_Number_String), "%d", signal_Number); // Convert signal number to a string
   			//concatination of everything to display a final message
            strcat(child_Terminating_Message, "terminated by signal ");
            strcat(child_Terminating_Message, signal_Number_String);
            strcat(child_Terminating_Message, "\n");
            write(1, child_Terminating_Message, sizeof(child_Terminating_Message));
		}
		///IF COMPLETION WAS NORMAL, WE NEED TO GET EXIT VALUE
		else{
			// Child exited normally, write out the normal child process end message
			char status_Number_String[5];
			strcat(child_Terminating_Message, "exit value ");
            // WEXITSTATUS(status)
            // returns the exit status of the child.  This consists of the
            //least significant 8 bits of the status argument that the child
            //specified in a call to exit(3) or _exit(2) or as the argument
            // for a return statement.
            //http://man7.org/linux/man-pages/man2/wait.2.html
			snprintf(status_Number_String, sizeof(status_Number_String), "%d", WEXITSTATUS(status)); // Convert status number to a string
			strcat(child_Terminating_Message, status_Number_String);
			strcat(child_Terminating_Message, "\n");
			// Write out the message of the termination
			write(1, child_Terminating_Message, sizeof(child_Terminating_Message));
		}
		continue;
	}
}

 /*************************************************************************************************************
 * Function:  void split_Users_Command_into_Arguments(char *input_Command, char **command_Arguments)
 * Description: Function break users input into individual words and stores those words in the array
 * The function also checks for the presence of the <, >, and $ charactesr and does not include
 * them in the array. The function uses strtok() C function and uses white space as a delimiter.
 * code adopted from  http://codereview.stackexchange.com/questions/42990/tokenizing-string-using-strtok
 * http://faq.cprogramming.com/cgi-bin/smartfaq.cgi?answer=1061423302&id=1044780608
 ***************************************************************************************************************/

void split_Users_Command_into_Arguments(char *input_Command, char **command_Arguments){
    char *current_Token;
    int token_Number = 0;
  // Determine if we have any redirect symbols
    int output_Redirection_exit = substring_Finder(input_Command, ">");
	int input_Redirection_Exist = substring_Finder(input_Command, "<");
  //strtok will tokenize a string i.e. convert it into a series of substrings.
  //It does that by searching for delimiters that separate these tokens (or substrings).
  //And you specify the delimiters. In your case, you want ' ' or ',' or '.' or '-' to be the delimiter.
  //in our case the delimiter will be " " ---whitespace
  //we will take users input command and break it into words based on the whitespace delimiter.
  //the output will be stored in the output array.
    current_Token = strtok(input_Command, " ");
    while (current_Token != NULL) {
  	//if we find an output redirect symbol, exist the function
  	if ((output_Redirection_exit == 1) && (strcmp(current_Token, ">") == 0)){
    	break;
    }
  	// if we find an input redirect symbol, exit the function
    if ((input_Redirection_Exist == 1) && (strcmp(current_Token, "<") == 0)) {
    	break;
    }
    // if we find the & symbol which is used for the background processes, exit the function
    if (strcmp(current_Token, "&") == 0){
    	break;
    }
    //if none of the above mentioned symbols were encountered (<, >, and &) we store the word in the array
  	command_Arguments[token_Number] = current_Token;
    current_Token = strtok (NULL, " ");
    //increment the tokes, which are words in our case
    token_Number++;
    // if the user extered to many arguments in the commend, we will exit the function
    if (token_Number == (MAX_ARGUMENTS - 1)) {
  		command_Arguments[MAX_ARGUMENTS - 1] = 0;
    	break;
    }
  }
  // Add the null terminator to the end of the array
  command_Arguments[token_Number] = 0;

}

 /*************************************************************************************************************
 * Function:  void file_Name(char *input_Command, char *file_Name_Entered)
 * Description: Function that takes a user entered command and returns a file name
 * that the user entered in case there is a redirection
 * we use user defined helper function: (substring_Finder(string, substring),to check if the entered command
 * has < or > substrings
 * Parameters: users command and variable that will store the file name
 ***************************************************************************************************************/
void file_Name(char *input_Command, char *file_Name_Entered){
	// 1st we check if there is a redirection
	//if there is no redirection, we just can exit function
	if ((substring_Finder(input_Command, "<") == 0) && (substring_Finder(input_Command, ">") == 0))	{
		return;
	}
  	// Break all the commands into an array and find the first element
	//  after the redirection signs
	char *temporary_Array[MAX_ARGUMENTS];
	char *current_Token;
    int token_Number = 0;
    int redirection_Position = 0;
    //initiazile array to null
	int i;
	for(i = 0; i < MAX_ARGUMENTS; i++){
		temporary_Array[i] = 0;
	}
    //we will increment though the user command and break it using the strtok() command
    //strtok will tokenize a string i.e. convert it into a series of substrings.
    //It does that by searching for delimiters that separate these tokens (or substrings).
    //And you specify the delimiters. In your case, you want ' ' or ',' or '.' or '-' to be the delimiter.
    //in our case the delimiter will be " " ---whitespace
    current_Token = strtok(input_Command, " ");
    //loop through the command until NUll character is encountered
    while (current_Token != NULL) {
  	// when the redirection character was encountered, save the position of that character
  	  	if ((strcmp(current_Token, "<") == 0) || (strcmp(current_Token, ">") == 0)){
  		redirection_Position = token_Number;// position is stored in the variable
  	}
    // Add the command arguments to the array one by one
  	temporary_Array[token_Number] = current_Token;
  	//tokens are separated based on the whitespace delimiter
    current_Token = strtok (NULL, " ");
    //incrementing of the tokens
    token_Number++;
    // in case the user entered to many arguments, we need to exist the function
    if (token_Number == (MAX_ARGUMENTS - 1)) {
  		temporary_Array[MAX_ARGUMENTS - 1] = 0;
    	break;
    }
  }
  // if get to this point, this is the indication that command size did not acccded the
  //max allowed # of arguments, so we will get the file name
  //first we need to make sure that the name was indicated, thus there is a value that is not equal to null after the < or > delimiter
  if (temporary_Array[redirection_Position + 1] != 0) {
    //name of the file is copied from the temporary array to the return value of this fucntion
  	strncpy(file_Name_Entered, temporary_Array[redirection_Position + 1], MAX_CHARACTERS);
  }
}


 /*************************************************************************************************************
 * Function:  int substring_Finder(char *search_String, char *substering_Of_Interst)
 * Description: Function to search for a specific substring in a string
 * Parameters: sting and substring
 * returns 1 - if substring was found
 * returns 2 - if substring was not founf\d
 * taken from http://articles.leetcode.com/2010/10/implement-strstr-to-find-substring-in.html
 ***************************************************************************************************************/
int substring_Finder(char *search_String, char *substering_Of_Interst){
    // Search for the specified string
  //Returns a pointer to the first occurrence of str2 in str1, or a null pointer if str2 is not part of str1.
  char *found_Substring = strstr(search_String, substering_Of_Interst);
  if (found_Substring == 0) {
  	return 0; // False, did not find the string.
  }
  else {
  	return 1; // True, found the string.
  }
}
