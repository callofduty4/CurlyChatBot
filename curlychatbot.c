/*************************************
/   ___ _   _  _____  _    __     __ /
/  / _|| | | ||  _  || |   \ \   / / /
/ | |  | | | || |_| || |    \ \ / /  /
/ | |  | | | ||    _/| |     \ \ /   /
/ | |_ | |_| || |\ \ | |__   /  /    /
/  \__| \___/ |_| \_\|____| /__/     /
/                                    /
/************************************/
/*
 * A chat bot programmed in C with cURL and Jansson libraries.
 * Licensed with GPL, please see LICENSE for more info.
 */

#include <sys/types.h>
#include <sys/wait.h>
#include "./curlychatbot.h"

#define PASSWORD_PROMPT_CHARCOUNT 21

char* g_username;
char* g_password;
pid_list_t* g_pid_list_start;

int main(int argc, char **argv)
{
	printf("Hello\n");
	int res;
	Login_result login_res;
	while (1)
	{
		res = parse_cmd_input(argc, argv);
		if (res)
		{
			exit(1);
		}
		login_res = login();
		if (login_res == FATALERROR)
		{
			exit(1);
		}
		if (login_res == SUCCESS)
		{
			break;
		}
		curl_slist_free_all(g_login_cookies);
		free(g_username);
	}
	res = start_clients(argc, argv);
	/*
	switch (res)
	{
		case -1:
		{
			exit(1);
		}
		case 0:
		{
			// client process ended. let the process end.
			return 0;
		}
		case 1:
		{
			// 
			int i;
			printf("Connected to: ");
			for (i = 2; i < argc; i++)
			{
				printf("%s ", argv[i]);
			}
			printf("\n");
		}
	}
	*/
	if (res)
	{
		exit(1);
	}
	res = wait_for_clients();
	if (res)
	{
		exit(1);
	}
	curl_global_cleanup();
	return 0;
}

/* 
 * Parse the command line input and ask for password
 * Input:	argc, the number of arguments
 *			argv, the argument array
 * Output: 0 on success, 1 on failure
 */
int parse_cmd_input(int argc, char **argv)
{
	if (argc < 3)
	{
		printf("Usage: ./curlychatbot <username> <space-separated list of wikis>\n");
		printf("Example: ./curlychatbot Icecreambot callofduty mlp runescape\n");
		return 1;
	}
	char *username = argv[1];
	size_t numchars = strlen(username);
	size_t actualsize = sizeof(char)*(strlen(username)+1);
	g_username = (char *)malloc(actualsize);
	memcpy(g_username, username, actualsize);
	g_username[numchars] = 0;
	numchars = numchars + PASSWORD_PROMPT_CHARCOUNT;
	char prompt[numchars];
	snprintf(prompt, sizeof(prompt), "Enter password for %s:", g_username);
	prompt[numchars-1] = 0;
	g_password = getpass(prompt);
	if (*g_password == 0)
	{
		printf("Exiting\n");
		return 1;
	}
	return 0;
}

/*
 * Starts all of the clients
 * Input:	argc and argv
 * Output:	1 on success, >1 on error. Client processes return 0
 */
int start_clients(int argc, char **argv)
{
	/*
	This will be made to work with multiple wikis, but gdb doesn't like forked processes
	right now.
	int i;
	for (i = 2; i < argc; i++)
	{
		// TODO: start client processes
		pid_t new_pid = fork();
		if (new_pid == 0)
		{
			// in the client process
			// TODO: startup the client
			start_connection(argv[i])
			return 0;
		}
		if (new_pid < 0)
		{
			// there was an error
			perror("fork");
			return -1;
		}
		else
		{
			pid_list_t *client = (pid_list_t *)malloc(sizeof(pid_list_t));
			client->pid = new_pid;
			add_client(client);
			return 1;
		}
	}
	*/
	return start_connection(argv[2]);
}

/*
 * Wait for all clients to finish before exiting
 * Inputs:	None
 * Output:	1 on error, 0 on success
 */
int wait_for_clients()
{
	while (g_pid_list_start)
	{
		pid_list_t *client = g_pid_list_start;
		int status;
		pid_t pid = waitpid(-1, &status, 0);
		while (client)
		{
			if (client->pid == pid)
			{
				remove_client(client);
				break;
			}
		}
		if (!client)
		{
			// did not find the client in the list.
			printf("Error: could not find client process id in client list\n");
			return 1;
		}
	}
	// all clients accounted for. We are all good
	return 0;
}
/*
 * Adds a client to the linked list
 * Input:	client, the new client to add
 * Output:	nothing
 */
void add_client(pid_list_t *client)
{
	// set the next of the new client to the (old) head of the list
	client->next = g_pid_list_start;
	// set the previous of the client to null; there is no previous
	client->prev = 0;
	if (g_pid_list_start != 0)
	{
		// if the list was not empty, set the previous of the old head to the new client
		g_pid_list_start->prev = client;
	}
	// set the head of the list to the new client
	g_pid_list_start = client;
}

/*
 * Removes a client from the linked list
 * Input:	client, the clinent to remove
 */
void remove_client(pid_list_t *client)
{
	pid_list_t *nextclient = client->next;
	pid_list_t *prevclient = client->prev;
	// case 1: prevclient = 0, nextclient != 0
	// this means this is the first client in the list
	if (!prevclient && nextclient)
	{
		// we just need to set the start of the free list to the next client, and 
		// set the next client's previous pointer to 0.
		g_pid_list_start = nextclient;
		nextclient->prev = 0;
	}
	// case 2: prevclient != 0, nextclient = 0
	// this means this is the last client in the list
	if (prevclient && !nextclient)
	{
		// we just need to set the previous client's next pointer to 0.
		prevclient->next = 0;
	}
	// case 3: prevclient != 0, nextclient != 0
	// this means this client is in the middle of the list somewhere
	if (prevclient && nextclient)
	{
		// set the previous block's next pointer to the next client, and the next client's
		// previous pointer to the previous client.
		prevclient->next = nextclient;
		nextclient->prev = prevclient;
	}
	// case 4: prevclient = 0, nextclient = 0
	// this means this is the only client in the list
	if (!prevclient && !nextclient)
	{
		// just need to set the free_start pointer to 0.
		g_pid_list_start = 0;
	}
	free(client);
}