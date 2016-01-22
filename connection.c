#include "./connection.h"

#define CHAT_GET_INFO_URL_SIZE 56
#define CHAT_GET_SESSION_URL_SIZE 75
#define CHAT_POLLING_URL_SIZE_ADDITION 5
#define MAX_SERVER_ID 4
#define MAX_ROOM_ID 6

char* s_wiki_name;

/*
 * Starts the connection by getting the chatroom info, the chat session ID
 * and then creates a thread to handle the REPL
 * Returns 0 on success (REPL exited cleanly, i.e. bot has quit)
 * Returns 1 if an error occurred anywhere
 * Input:	the wiki name
 * Output:	1 or 0 as above
 */
int start_connection(char* wiki_name)
{
	s_wiki_name = wiki_name;
	chat_info_t *chat_info;
	chat_info = get_chat_info();
	if (!chat_info)
	{
		return 1;
	}
	chat_info = get_chat_session(chat_info);
	if (!chat_info)
	{
		return 1;
	}
	return 0;
}

/*
 * Gets the required information for chat and returns it
 * Input:	None
 * Output:	A pointer to the struct where the data is now stored
 *			NULL if an error occurred.
 */
chat_info_t *get_chat_info()
{
	chat_info_t *result;
	// size the URL properly
	size_t URL_size = CHAT_GET_INFO_URL_SIZE + strlen(s_wiki_name);
	char URL[URL_size];
	snprintf(URL, sizeof(URL), "http://%s.wikia.com/wikia.php?controller=Chat&format=json", s_wiki_name);
	URL[URL_size - 1] = 0;
	HTTP_response_t info_response = make_POST_request(URL, "", g_login_cookies);
	if (!info_response.success)
	{
		return NULL;
	}
	result = parse_chat_info(info_response.memory);
	destroy_HTTP_response(&info_response, 0);
	return result;
}

chat_info_t *get_chat_session(chat_info_t *chat_info)
{
	// unroll the addition so we can get a semblance of efficiency out of this
	size_t URL_size1 = strlen(chat_info->c_server) + strlen(chat_info->c_port);
	size_t URL_size2 = strlen(g_username) + strlen(chat_info->c_key);
	// we are assuming the roomID will never be longer than 6 bytes, and that the serverID will never be longer than 4 bytes
	size_t URL_size3 = MAX_ROOM_ID + MAX_SERVER_ID;
	size_t URL_size4 = URL_size1 + URL_size2;
	size_t URL_size = URL_size4 + URL_size3;
	URL_size += CHAT_GET_SESSION_URL_SIZE;
	char URL[URL_size];
	snprintf(URL, sizeof(URL), "http://%s:%s/socket.io/1/?name=%s&key=%s&roomId=%lld&serverId=%lld&EIO=2&transport=polling",
		chat_info->c_server, chat_info->c_port, g_username, chat_info->c_key, chat_info->c_room_id, chat_info->c_server_id);
	HTTP_response_t session_response = make_GET_request(URL, g_login_cookies);
	json_t *root;
	json_error_t error;
	// we have an offset of 5 bytes when receiving data from the server. I don't know why this is there
	root = json_loads(session_response.memory+5, 0, &error);
	if (!root)
	{
		// not successfully made. Cleanup and exit
		fprintf(stderr, "Error parsing JSON: line %d: %s\n", error.line, error.text);
		free(chat_info);
		return NULL;
	}
	// check if root is an object
	if (!json_is_object(root))
	{
		// not an object. Cleanup and exit
		fprintf(stderr, "JSON root is not an object.\n");
		free(chat_info);
		json_decref(root);
		return NULL;
	}
	const char *c_session = json_string_value(json_object_get(root, "sid"));
	if (!c_session)
	{
		free(chat_info);
		json_decref(root);
		return NULL;
	}
	size_t size = strlen(c_session) + 1;
	chat_info->c_session = (char *)malloc(size * sizeof(char));
	memcpy(chat_info->c_session, c_session, size);
	destroy_HTTP_response(&session_response, 0);
	size_t URL_size5 = CHAT_POLLING_URL_SIZE_ADDITION + strlen(chat_info->c_session);
	URL_size += URL_size5;
	chat_info->c_URL = (char *)malloc(URL_size * sizeof(char));
	snprintf(chat_info->c_URL, URL_size, "%s&sid=%s", URL, chat_info->c_session);
	json_decref(root);
	return chat_info;
}


/*
 * Parses out the required information and puts it into a chat_info_t struct
 * Input:	The data string downloaded by make_POST_request
 * Output:	A pointer to the struct where the data is now stored
 *			NULL if an error occured
 */
chat_info_t *parse_chat_info(char* data)
{
	json_t *root;
	json_error_t error;
	root = json_loads(data, 0, &error);
	// check if root was successfully made
	if (!root)
	{
		// not successfully made. Cleanup and exit
		fprintf(stderr, "Error parsing JSON: line %d: %s\n", error.line, error.text);
		return NULL;
	}
	// check if root is an object
	if (!json_is_object(root))
	{
		// not an object. Cleanup and exit
		fprintf(stderr, "JSON root is not an object.\n");
		json_decref(root);
		return NULL;
	}
	chat_info_t *chat_info = (chat_info_t *)malloc(sizeof(chat_info_t));
	if (!chat_info)
	{
		return NULL;
	}
	// get key
	const char *c_key = json_string_value(json_object_get(root, "chatkey"));
	if (!c_key)
	{
		free(chat_info);
		return NULL;
	}
	size_t size = strlen(c_key) + 1;
	chat_info->c_key = (char *)malloc(size * sizeof(char));
	memcpy(chat_info->c_key, c_key, size);
	// get server
	const char *c_server = json_string_value(json_object_get(root, "nodeHostname"));
	if (!c_server)
	{
		free(chat_info);
		return NULL;
	}
	size = strlen(c_server) + 1;
	chat_info->c_server = (char *)malloc(size * sizeof(char));
	memcpy(chat_info->c_server, c_server, size);
	// get server ID
	chat_info->c_server_id = (long long)json_integer_value(json_object_get(root, "nodeInstance"));
	// get port
	const char *c_port = json_string_value(json_object_get(root, "nodePort"));
	if (!c_port)
	{
		free(chat_info);
		return NULL;
	}
	size = strlen(c_port) + 1;
	chat_info->c_port = (char *)malloc(size * sizeof(char));
	memcpy(chat_info->c_port, c_port, size);
	// get room ID
	chat_info->c_room_id = (long long)json_integer_value(json_object_get(root, "roomId"));
	json_decref(root);
	return chat_info;
}