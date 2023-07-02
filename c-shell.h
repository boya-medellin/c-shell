#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#define clear() printf("\033[H\033[J")


void init_shell()
{
	/* display info at launch */
	clear();
	printf("\n\n************************************");
	printf("\n\nC-SHELL");
	printf("\n************************************");
	printf("\n");
	sleep(1);
	clear();
}

void get_current_working_dir( char *string )
{
	/* reads the current working directory */
	errno = 0;
	char* path = getcwd(NULL, 0);

	if (path == NULL)
	{
		perror("getcwd");
		exit(EXIT_FAILURE);
	}

	strcpy(string, path);
}

void get_current_user( char* string )
{
	/* reads the current user */
	errno = 0;
	char* buf = getlogin();
	if (buf == NULL)
	{
		perror("getlogin");
		exit(EXIT_FAILURE);
	}
	strcpy(string, buf);
}

void declare_variable(char *buf)
{
	/* stores user-defined variable */
	char *token, name[20], value[20];
	const char delim[2] = "=";
	int overwrite = 1;

	token = strtok(buf, delim);
	strcpy( name, token );

	token = strtok( NULL, delim );
	strcpy( value, token );

	setenv(name, value, overwrite);

	printf("Variable declared!\n");
}


void exec_cd( char *param[20] )
{
	/* built-in shell command */

	char user[100], path[20] = "/home/";
	get_current_user( user );

	if ( ( param[1] == NULL ) || ( strcmp( param[1], "~" ) == 0 ) )
	{
		/* cd home */
		strcat(path, user);
		chdir(path);
	}
	else 
	{
		chdir( param[1] );
	}
}

void exec_echo( char *param[20] )
{
	/* built-in shell command */

	if ( param[1] == NULL )
		return;
	printf("%s\n", param[1]);
}


void type_prompt(void)
{
	/* display the prompt */

	char user[100], dir[100];
	get_current_user( user );
	get_current_working_dir( dir );
	printf("%s@csd345sh%s$ ", user, dir);
}

void get_var(char *param[20], char *token, int i)
{
	/* if a parameter is a variable,
	 * replace it with the variable value */

	if ( strchr(token, '$') != NULL )
	{
		memmove(token, token+1, strlen(token)); 	// remove '$'
		param[i] = getenv( token );
	}
	else
	{
		param[i] = token;
	}
}

void get_param(char *param[20], char *buf)
{
	/* read parameters 
	 * from null-temrinated array of tokens*/

	const char delim[2] = " ";
	int i = 1;
	char * token;
	token = strtok(buf, delim);
	param[0] = token;

	while ( token != NULL )
	{
		token = strtok(NULL, delim);
		if ( token != NULL)
			get_var(param, token, i);
		else
			param[i] = NULL;
		i++;
	}
}

void format_bin(char *command, char *param[20] )
{
	/* format command with /usr/bin/ path prefix */

	char *prefix = "/usr/bin/";
	char *temp = strdup( command );
	strcpy( command, prefix );
	strcat( command, temp );
}

void handle_prompt(char *buf, char *command, char *param[20])
{
	/* parses input into commands and command parameters */

	char *tokens[100];

	if ( strchr(buf, '=') != NULL )
	{
		declare_variable(buf);
		command[0] = 0;
		return;
	}

	get_param(param, buf);
	strcpy(command,  param[0]);


	/* shell built in commands */
	if ( strcmp( command, "cd" ) == 0 )
		exec_cd( param );
	else if ( strcmp( command, "exit" ) == 0)
		exit(EXIT_SUCCESS);
	else if ( strcmp( command, "echo" ) == 0)
		exec_echo( param );
	else if ( strcmp( command, "clear" ) == 0)
		clear();
	else 
		/* is not a shell built-in command */
		format_bin( command, param );

}

void read_prompt(char *command, char *param[20])
{
	/* receives input from stdin and calls the handler */
	char *buf = NULL;
	size_t l = 0, len;
	int lineSize;

	/* read input */
	lineSize = getline(&buf, &l, stdin);

	/* remove trailing characters */
	len = strlen(buf);
	if (len > 0 && buf[len-1] == '\n')
		buf[len-1] = 0;

	handle_prompt(buf, command, param);
}

void signal_callback_handler(int signum)
{
	/* UNIX signal handler */
	printf("Caught signal %d\n", signum);
	fflush(stdout);

	if ( signum == 2 ) 			// SIGINT
	{
		printf("Will now exit\n");
		exit(signum);
	}
	if ( signum == 18 ) 		// SIGCONT
	{
		printf("Process continued.\n");
	}
	if ( signum == 19 ) 		// SIGSTOP
	{
		printf("Process stopped.\n");
		wait(NULL);
	}
	if ( signum == 20 ) 		// SIGTSTP
	{
		printf("Sent process to background\n");
		//wait(NULL);
	}
}

void run_loop(void)
{
	/* main loop of the program */
	char command[100], *param[20];
	int status = 0;

	struct sigaction sa;
	sa.sa_handler = &signal_callback_handler;
	sa.sa_flags = SA_RESTART;
	sigaction(SIGTSTP, &sa, NULL);
	sigaction(SIGCONT, &sa, NULL);
	sigaction(SIGSTOP, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);

	init_shell();

	while(1)
	{
		type_prompt();
		read_prompt( command, param );

		int pid = fork();

		if ( pid == 0 )
		{ 	
			execve( command, param, NULL );
			exit( pid );
		} 
		if ( pid > 0 ) 
		{ 			
			waitpid(pid, NULL, 0);

			if (WIFSIGNALED(status))
				printf("\nError child process\n");
			else if (WEXITSTATUS(status))
				printf("\nExited Normally\n");

		} 
		if ( pid < 0) 
		{
			printf("\nFork failed.\n");
		}
	}
}

