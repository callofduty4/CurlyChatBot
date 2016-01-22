#include <pthread.h>
#include <jansson.h>
#include "./netcode.h"

extern struct curl_slist *g_login_cookies;
extern char *g_username;

typedef struct 
{
	char* c_key;
	char* c_server;
	long long c_server_id;
	char* c_port;
	long long c_room_id;
	char* c_session;
	char* c_URL;
} chat_info_t;

int start_connection(char* wiki_name);
chat_info_t *get_chat_info();
chat_info_t *get_chat_session(chat_info_t *chat_info);
chat_info_t *parse_chat_info(char* data);