#include <jansson.h>
#include "./netcode.h"

extern char* g_username;
extern char* g_password;

typedef enum
{
	SUCCESS,
	NONAME,
	ILLEGAL,
	NOTEXISTS,
	EMPTYPASS,
	WRONGPASS,
	WRONGPLUGINPASS,
	CREATEBLOCKED,
	THROTTLED,
	BLOCKED,
	MUSTBEPOSTED,
	NEEDTOKEN,
	FATALERROR
}Login_result;

Login_result login();
Login_result determine_login_result(const char* string_result);