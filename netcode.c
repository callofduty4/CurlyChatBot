#include "./netcode.h"

/*
 * Makes a GET request.
 * Input:	URL - the URL to GET
 * Output:	An HTTP_response_t struct containing the downloaded page source, the length
 *			of the response, and the cookies
 */
HTTP_response_t make_GET_request(const char *URL, struct curl_slist *cookies)
{
	CURL *curl_handle;
	CURLcode res;
	HTTP_response_t chunk;
	chunk.memory = malloc(1);
	chunk.size = 0;
	chunk.success = 0;
	chunk.cookies = NULL;
	curl_global_init(CURL_GLOBAL_ALL);
	// init the curl session
	curl_handle = curl_easy_init();
	// specify URL to GET
	curl_easy_setopt(curl_handle, CURLOPT_URL, URL);
	// start cookie engine
	curl_easy_setopt(curl_handle, CURLOPT_COOKIEFILE, "");
	// send all the data to write_memory_callback
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_memory_callback);
	// give the chunk struct to write_memory_callback
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
	// give a useragent
	curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "CurlyChatBot/0.0.1");
	// now we do the GET
	while(cookies)
	{
		res = curl_easy_setopt(curl_handle, CURLOPT_COOKIELIST, cookies->data);
		cookies = cookies->next;
	}
	res = curl_easy_perform(curl_handle);
	// check errores
	if (res != CURLE_OK)
	{
		fprintf(stderr, "curl_easy_perform() failed. %s\n", curl_easy_strerror(res));
		curl_easy_cleanup(curl_handle);
	    free(chunk.memory);
	    chunk.memory = NULL;
		return chunk;
	}
	else
	{
		printf("%lu bytes retrieved\n", (long)chunk.size);
		chunk.cookies = get_cookies(curl_handle);
	}
	// cleanup all
	curl_easy_cleanup(curl_handle);
	chunk.success = 1;
	return chunk;
}

/*
 * Makes a POST request
 * Input:	URL - the URL to post to
 *			data_to_post - the string of data to POST to the server
 * Output:	An HTTP_response_t struct containing the downloaded page source, the length
 *			of the response, and the cookies
 */
HTTP_response_t make_POST_request(const char *URL, const char *data_to_post, struct curl_slist *cookies)
{
	#ifdef DEBUG
	printf("Posting %s\n", data_to_post);
	#endif
	CURL *curl_handle;
	CURLcode res;
	// create a chunk. We won't actually use it, but we will redirect output to it
	// in this way we avoid cluttering up stdout.
	HTTP_response_t chunk;
	chunk.memory = malloc(1);
	chunk.size = 0;
	chunk.cookies = NULL;
	chunk.success = 0;
	curl_handle = curl_easy_init();
	if (!curl_handle) {
		// an error occurred in initialization.
		free(chunk.memory);
		chunk.memory = NULL;
		return chunk;
	}
	// setup the URL to POST to
	curl_easy_setopt(curl_handle, CURLOPT_URL, URL);
	// start cookie engine
	curl_easy_setopt(curl_handle, CURLOPT_COOKIEFILE, "");
	// set the data to POST
	curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, data_to_post);
	// set the length of the data to POST
	curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE, (long)strlen(data_to_post));
	// send all the data to write_memory_callback
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_memory_callback);
	// give the chunk struct to write_memory_callback
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
	// give a useragent
	curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "CurlyChatBot/0.0.1");
	// add cookies
	while(cookies)
	{
		res = curl_easy_setopt(curl_handle, CURLOPT_COOKIELIST, cookies->data);
		cookies = cookies->next;
	}
	res = curl_easy_perform(curl_handle);
	if (res != CURLE_OK)
	{
		fprintf(stderr, "curl_easy_perform() failed in make_POST_request(). %s\n", curl_easy_strerror(res));
		curl_easy_cleanup(curl_handle);
		free(chunk.memory);
		chunk.memory = NULL;
		return chunk;
	}
	chunk.cookies = get_cookies(curl_handle);
	curl_easy_cleanup(curl_handle);
	chunk.success = 1;
	return chunk;
}

/*
 * Cleans up an HTTP response
 * Input:	reponse, a pointer to a HTTP_response_t
 * Output:	nothing
 */
void destroy_HTTP_response(HTTP_response_t *response, Delete_options options)
{
	if (response->memory)
	{
		free(response->memory);
	}
	if (response->cookies && !(options == KEEP_COOKIES))
	{
		curl_slist_free_all(response->cookies);
	}
}

/* 
 * Used so we can redirect output from cURL from stdout into a chunk of memory
 * Passed as a callback function with curl_setopt and WRITEFUNCTION
 */
static size_t write_memory_callback(void *contents, size_t size, size_t nmemb, void *userp) 
{
	size_t actualsize = size * nmemb;
	HTTP_response_t *mem = (HTTP_response_t *)userp;
	// correctly allocate the size needed to store all the data
	mem->memory = realloc(mem->memory, mem->size + actualsize + 1);
	if (mem->memory == NULL)
	{
		// we are out of memory.
		printf("realloc returned NULL\n");
		return 0;
	}
	// copy everything into the userp memory
	memcpy(&(mem->memory[mem->size]), contents, actualsize);
	mem->size += actualsize;
	mem->memory[mem->size] = 0;
	// return the size of the data for bookkeeping
	return actualsize;
}

static struct curl_slist *get_cookies(CURL *curl_handle)
{
	CURLcode res;
	struct curl_slist *cookies;
	res = curl_easy_getinfo(curl_handle, CURLINFO_COOKIELIST, &cookies);
	if (res != CURLE_OK)
	{
		fprintf(stderr, "curl_easy_getinfo() failed: %s\n", curl_easy_strerror(res));
		return NULL;
	}
	else
	{
		return cookies;
	}
}