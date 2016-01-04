#include "./common.h"
typedef struct 
{
	char *memory;
	size_t size;
	struct curl_slist *cookies;
	int success;
}HTTP_response_t;

typedef enum
{
	KEEP_COOKIES
}Delete_options;

HTTP_response_t make_GET_request(const char *URL, struct curl_slist *cookies);
HTTP_response_t make_POST_request(const char *URL, const char *data_to_post, struct curl_slist *cookies);
void destroy_HTTP_response(HTTP_response_t *response, Delete_options options);
static size_t write_memory_callback(void *contents, size_t size, size_t nmemb, void *userp);
static struct curl_slist *get_cookies(CURL *curl_handle);