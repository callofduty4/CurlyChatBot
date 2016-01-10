#include "./login.h"

extern struct curl_slist *g_login_cookies;

typedef struct pid_list
{
	pid_t pid;
	struct pid_list *next;
	struct pid_list *prev;
}pid_list_t;

int parse_cmd_input(int argc, char **argv);
int start_clients(int argc, char **argv);
int wait_for_clients();
void add_client(pid_list_t *client);
void remove_client(pid_list_t *client);