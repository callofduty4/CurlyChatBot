#include <jansson.h>
#include "./netcode.h"
#include "./login.h"

#define LOGIN_URL_CHARCOUNT 45

/*
 * Logs into Wikia. Uses global variables g_username and g_password
 * Input:	Nothing
 * Output:	Nothing
 */
int login()
{
	size_t username_size = strlen(g_username);
	size_t password_size = strlen(g_password);
	size_t actualsize = username_size + password_size;
	actualsize = actualsize + LOGIN_URL_CHARCOUNT;
	char login_URL[] = "http://community.wikia.com/api.php";
	char login_data_buf[actualsize];
	snprintf(login_data_buf, sizeof(login_data_buf), "action=login&lgname=%s&lgpassword=%s&format=json", g_username, g_password);
	login_data_buf[actualsize-1] = 0;
	HTTP_response_t login_response = make_POST_request(login_URL, login_data_buf);
	if (!login_response.memory)
	{
		fprintf(stderr, "Login failed\n");
		return 1;
	}
	/*
	struct curl_slist *cookies = login_response.cookies;
	while (cookies)
	{
		printf("%s\n", cookies->data);
		cookies = cookies->next;
	}
	return 0;
	*/
	// make json type for root and error
	json_t *root;
	json_error_t error;
	// load string into json form
	root = json_loads(login_response.memory, 0, &error);
	// check if root was successfully made
	if (!root)
	{
		// not successfully made. Cleanup and exit
		fprintf(stderr, "Error parsing JSON: line %d: %s\n", error.line, error.text);
		destroy_HTTP_response(&login_response);
		return 1;
	}
	// check if root is an object
	if (!json_is_object(root))
	{
		// not an object. Cleanup and exit
		fprintf(stderr, "JSON root is not an object.\n");
		json_decref(root);
		destroy_HTTP_response(&login_response);
		return 1;
	}
	// get the login token
	json_t *login;
	login = json_object_get(root, "login");
	json_t *token;
	token = json_object_get(login, "token");
	// token has been found. Cleanup response
	destroy_HTTP_response(&login_response);
	#ifdef DEBUG
	printf("Login token: %s\n", json_string_value(token));
	#endif
	// TODO: perform second part of login
}