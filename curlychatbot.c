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

#include "./netcode.h"
#include "./curlychatbot.h"

#define PASSWORD_PROMPT_CHARCOUNT 21

char* g_username;
char* g_password;

int main(int argc, char **argv)
{
	printf("Hello\n");
	int res;
	res = parse_cmd_input(argc, argv);
	if (res)
	{
		exit(1);
	}
	res = login();
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
	return 0;
}