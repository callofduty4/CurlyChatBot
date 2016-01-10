#include <jansson.h>
#include "./login.h"

#define LOGIN_URL_CHARCOUNT 45
#define LOGIN_URL_CHAR_ADDITION 9

struct curl_slist *g_login_cookies = (struct curl_slist *)0;

/*
 * Logs into Wikia. Uses global variables g_username and g_password
 * Input:	Nothing
 * Output:	Nothing
 */
Login_result login()
{
	// setup the URL and data to send
	size_t username_size = strlen(g_username);
	size_t password_size = strlen(g_password);
	size_t actualsize = username_size + password_size;
	actualsize = actualsize + LOGIN_URL_CHARCOUNT;
	actualsize = actualsize*sizeof(char);
	char login_URL[] = "http://community.wikia.com/api.php";
	char *login_data_buf;
	login_data_buf = (char *)malloc(actualsize);
	snprintf(login_data_buf, actualsize, "action=login&lgname=%s&lgpassword=%s&format=json", g_username, g_password);
	login_data_buf[actualsize-1] = 0;
	// send the request and receive the returned data
	HTTP_response_t token_response = make_POST_request(login_URL, login_data_buf, NULL);
	if (!token_response.success)
	{
		fprintf(stderr, "Login failed\n");
		//destroy_HTTP_response(&token_response, 0);
		free(login_data_buf);
		return FATALERROR;
	}
	if (!token_response.cookies)
	{
		fprintf(stderr, "Login failed. No cookies received\n");
		destroy_HTTP_response(&token_response, 0);
		free(login_data_buf);
		return FATALERROR;
	}
	g_login_cookies = token_response.cookies;
	// make json type for root and error
	json_t *root;
	json_error_t error;
	// load string into json form
	root = json_loads(token_response.memory, 0, &error);
	// check if root was successfully made
	if (!root)
	{
		// not successfully made. Cleanup and exit
		fprintf(stderr, "Error parsing JSON: line %d: %s\n", error.line, error.text);
		destroy_HTTP_response(&token_response, 0);
		free(login_data_buf);
		return FATALERROR;
	}
	// check if root is an object
	if (!json_is_object(root))
	{
		// not an object. Cleanup and exit
		fprintf(stderr, "JSON root is not an object.\n");
		json_decref(root);
		destroy_HTTP_response(&token_response, 0);
		free(login_data_buf);
		return FATALERROR;
	}
	// get the login token
	json_t *login;
	login = json_object_get(root, "login");
	json_t *token;
	token = json_object_get(login, "token");
	const char* token_value = json_string_value(token);
	#ifdef DEBUG
	printf("Login token: %s\n", token_value);
	#endif
	// cleanup previous request
	destroy_HTTP_response(&token_response, KEEP_COOKIES);
	// create the new login POST data
	size_t token_value_len = (strlen(token_value) + LOGIN_URL_CHAR_ADDITION) * sizeof(char);
	actualsize = actualsize + token_value_len;
	login_data_buf = (char *)realloc(login_data_buf, actualsize);
	if (!login_data_buf)
	{
		fprintf(stderr, "Ran out of memory in login()\n");
		return FATALERROR;
	}
	snprintf(login_data_buf, actualsize, "action=login&lgname=%s&lgpassword=%s&lgtoken=%s&format=json", g_username, g_password, token_value);
	login_data_buf[actualsize-1] = 0;
	HTTP_response_t login_response = make_POST_request(login_URL, login_data_buf, g_login_cookies);
	if (!login_response.success)
	{
		fprintf(stderr, "Login failed\n");
		destroy_HTTP_response(&login_response, 0);
		free(login_data_buf);
		return FATALERROR;
	}
	if (!login_response.cookies)
	{
		fprintf(stderr, "Login failed. No cookies received\n");
		destroy_HTTP_response(&login_response, 0);
		free(login_data_buf);
		return FATALERROR;
	}
	curl_slist_free_all(g_login_cookies);
	g_login_cookies = login_response.cookies;
	// load string into json form 
	root = json_loads(login_response.memory, 0, &error);
	if (!root)
	{
		// not successfully made. Cleanup and exit
		fprintf(stderr, "Error parsing JSON: line %d: %s\n", error.line, error.text);
		destroy_HTTP_response(&login_response, 0);
		free(login_data_buf);
		return FATALERROR;
	}
	// check if root is an object
	if (!json_is_object(root))
	{
		// not an object. Cleanup and exit
		fprintf(stderr, "JSON root is not an object.\n");
		json_decref(root);
		destroy_HTTP_response(&login_response, 0);
		free(login_data_buf);
		return FATALERROR;
	}
	// get the login result
	login = json_object_get(root, "login");
	json_t *result;
	result = json_object_get(login, "result");
	const char* result_value = json_string_value(result);
	Login_result login_res = determine_login_result(result_value);
	// cleanup, remove password from memory 
	memset(g_password,0,strlen(g_password));
	free(login_data_buf);
	destroy_HTTP_response(&login_response, KEEP_COOKIES);
	json_decref(root);
	json_decref(token);
	return login_res;
}

Login_result determine_login_result(const char* string_result)
{
	if (strcmp(string_result, "Success") == 0)
	{
		printf("Logged into %s\n", g_username);
		return SUCCESS;
	}
	if (strcmp(string_result, "NoName") == 0)
	{
		printf("Username not properly set\n");
		return NONAME;
	}
	if (strcmp(string_result, "NotExists") == 0)
	{
		printf("That username does not exist\n");
		return NOTEXISTS;
	}
	if (strcmp(string_result, "EmptyPass") == 0)
	{
		printf("No password was provided\n");
		return EMPTYPASS;
	}
	if (strcmp(string_result, "WrongPass") == 0)
	{
		printf("Incorrect password for %s\n", g_username);
		return WRONGPASS;
	}
	if (strcmp(string_result, "WrongPluginPass") == 0)
	{
		printf("Incorrect passcode/passphrase for %s\n", g_username);
		return WRONGPLUGINPASS;
	}
	if (strcmp(string_result, "CreateBlocked") == 0)
	{
		printf("Account creation is blocked for this IP address\n");
		return CREATEBLOCKED;
	}
	if (strcmp(string_result, "Throttled") == 0)
	{
		printf("Account login rate limit reached. Please try again later\n");
		return THROTTLED;
	}
	if (strcmp(string_result, "Blocked") == 0)
	{
		printf("Account %s is blocked\n", g_username);
		return BLOCKED;
	}
	if (strcmp(string_result, "mustbeposted") == 0)
	{
		// this really shouldn't happen. Abort everything
		printf("Server received HTTP GET, requires POST. Exiting\n");
		return FATALERROR;
	}
	if (strcmp(string_result, "NeedToken") == 0)
	{
		// if this happened then something messed up with the cookies. Abort everything
		printf("Server did not receive required cookies. Exiting\n");
		return FATALERROR;
	}
}