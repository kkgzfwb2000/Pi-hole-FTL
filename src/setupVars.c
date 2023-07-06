/* Pi-hole: A black hole for Internet advertisements
*  (c) 2017 Pi-hole, LLC (https://pi-hole.net)
*  Network-wide ad blocking via your own hardware.
*
*  FTL Engine
*  Configuration interpreting routines
*
*  This file is copyright under the latest version of the EUPL.
*  Please see LICENSE file for your rights under this license. */

#include "FTL.h"
#include "log.h"
#include "config/config.h"
#include "setupVars.h"
#include "datastructure.h"

unsigned int setupVarsElements = 0;
char ** setupVarsArray = NULL;

static void get_conf_string_from_setupVars(const char *key, struct conf_item *conf_item)
{
	const char *setupVarsValue = read_setupVarsconf(key);
	if(setupVarsValue == NULL)
	{
		// Do not change default value, this value is not set in setupVars.conf
		log_debug(DEBUG_CONFIG, "setupVars.conf:%s -> Not set", key);

		// Free memory, harmless to call if read_setupVarsconf() didn't return a result
		clearSetupVarsArray();
		return;
	}

	// Free previously allocated memory (if applicable)
	if(conf_item->t == CONF_STRING_ALLOCATED)
		free(conf_item->v.s);
	conf_item->v.s = strdup(setupVarsValue);
	conf_item->t = CONF_STRING_ALLOCATED;

	// Free memory, harmless to call if read_setupVarsconf() didn't return a result
	clearSetupVarsArray();

	// Parameter present in setupVars.conf
	log_debug(DEBUG_CONFIG, "setupVars.conf:%s -> Setting %s to %s", key, conf_item->k, conf_item->v.s);
}

static void get_conf_bool_from_setupVars(const char *key, struct conf_item *conf_item)
{
	const char *boolean = read_setupVarsconf(key);

	if(boolean == NULL)
	{
		// Do not change default value, this value is not set in setupVars.conf
		log_debug(DEBUG_CONFIG, "setupVars.conf:%s -> Not set", key);

		// Free memory, harmless to call if read_setupVarsconf() didn't return a result
		clearSetupVarsArray();
		return;
	}
	else
	{
		// Parameter present in setupVars.conf
		conf_item->v.b = getSetupVarsBool(boolean);
	}

	// Free memory, harmless to call if read_setupVarsconf() didn't return a result
	clearSetupVarsArray();

	// Parameter present in setupVars.conf
	log_debug(DEBUG_CONFIG, "setupVars.conf:%s -> Setting %s to %s",
	          key, conf_item->k, conf_item->v.b ? "true" : "false");
}

static void get_conf_string_array_from_setupVars(const char *key, struct conf_item *conf_item)
{
	// Get clients which the user doesn't want to see
	const char *array = read_setupVarsconf(key);

	if(array != NULL)
	{
		getSetupVarsArray(array);
		for (unsigned int i = 0; i < setupVarsElements; ++i)
		{
			// Add string to our JSON array
			cJSON *item = cJSON_CreateString(setupVarsArray[i]);
			cJSON_AddItemToArray(conf_item->v.json, item);

			log_debug(DEBUG_CONFIG, "setupVars.conf:%s -> Setting %s[%u] = %s\n",
			          key, conf_item->k, i, item->valuestring);
		}
	}

	// Free memory, harmless to call if read_setupVarsconf() didn't return a result
	clearSetupVarsArray();
}

static void get_conf_upstream_servers_from_setupVars(struct conf_item *conf_item)
{
	// Try to import up to 50 servers...
	#define MAX_SERVERS 50
	for(unsigned int j = 0; j < MAX_SERVERS; j++)
	{
		// Get clients which the user doesn't want to see
		char server_key[sizeof("PIHOLE_DNS_XX") + 1];
		sprintf(server_key, "PIHOLE_DNS_%u", j);
		// Get value from setupVars.conf (if present)
		const char *value = read_setupVarsconf(server_key);

		if(value != NULL)
		{
			log_debug(DEBUG_CONFIG, "%s = %s\n", server_key, value);
			// Add string to our JSON array
			cJSON *item = cJSON_CreateString(value);
			cJSON_AddItemToArray(conf_item->v.json, item);

			log_debug(DEBUG_CONFIG, "setupVars.conf:PIHOLE_DNS_%u -> Setting %s[%u] = %s\n",
			          j, conf_item->k, j, item->valuestring);
		}

		// Free memory, harmless to call if read_setupVarsconf() didn't return a result
		clearSetupVarsArray();
	}
}

static void get_conf_temp_limit_from_setupVars(void)
{
	// Try to obtain temperature hot value
	const char *temp_limit = read_setupVarsconf("TEMPERATURE_LIMIT");

	if(temp_limit == NULL)
	{
		// Do not change default value, this value is not set in setupVars.conf
		log_debug(DEBUG_CONFIG, "setupVars.conf:TEMPERATURE_LIMIT -> Not set");

		// Free memory, harmless to call if read_setupVarsconf() didn't return a result
		clearSetupVarsArray();
		return;
	}

	double lim;
	bool set = false;
	if(sscanf(temp_limit, "%lf", &lim) == 1)
	{
		// Parameter present in setupVars.conf
		config.webserver.api.temp.limit.v.d = lim;
		set = true;
	}

	// Free memory, harmless to call if read_setupVarsconf() didn't return a result
	clearSetupVarsArray();

	if(set)
	{
		// Parameter present in setupVars.conf
		log_debug(DEBUG_CONFIG, "setupVars.conf:TEMPERATURE_LIMIT -> Setting %s to %f",
		          config.webserver.api.temp.limit.k, config.webserver.api.temp.limit.v.d);
	}
	else
	{
		// Parameter not present in setupVars.conf
		log_debug(DEBUG_CONFIG, "setupVars.conf:TEMPERATURE_LIMIT -> Not set (found invalid value)");
	}
}

static void get_conf_weblayout_from_setupVars(void)
{
	// Try to obtain web layout
	const char *web_layout = read_setupVarsconf("WEBUIBOXEDLAYOUT");

	if(web_layout == NULL)
	{
		// Do not change default value, this value is not set in setupVars.conf
		log_debug(DEBUG_CONFIG, "setupVars.conf:WEBUIBOXEDLAYOUT -> Not set");

		// Free memory, harmless to call if read_setupVarsconf() didn't return a result
		clearSetupVarsArray();
		return;
	}

	// If the property is set to false and different than "boxed", the property
	// is disabled. This is consistent with the code in AdminLTE when writing
	// this code
	if(web_layout != NULL && strcasecmp(web_layout, "boxed") != 0)
		config.webserver.interface.boxed.v.b = false;

	// Free memory, harmless to call if read_setupVarsconf() didn't return a result
	clearSetupVarsArray();

	// Parameter present in setupVars.conf
	log_debug(DEBUG_CONFIG, "setupVars.conf:WEBUIBOXEDLAYOUT -> Setting %s to %s",
	         config.webserver.interface.boxed.k,config.webserver.interface.boxed.v.b ? "true" : "false");
}

static void get_conf_webtheme_from_setupVars(void)
{
	// Try to obtain listening mode
	const char *webTheme = read_setupVarsconf("WEBTHEME");

	if(webTheme == NULL)
	{
		// Do not change default value, this value is not set in setupVars.conf
		log_debug(DEBUG_CONFIG, "setupVars.conf:WEBTHEME -> Not set");

		// Free memory, harmless to call if read_setupVarsconf() didn't return a result
		clearSetupVarsArray();
		return;
	}

	bool set = false;
	int web_theme_enum = get_web_theme_val(webTheme);
	if(web_theme_enum != -1)
	{
		set = true;
		config.webserver.interface.theme.v.web_theme = web_theme_enum;
	}

	// Free memory, harmless to call if read_setupVarsconf() didn't return a result
	clearSetupVarsArray();

	if(set)
	{
		// Parameter present in setupVars.conf
		log_debug(DEBUG_CONFIG, "setupVars.conf:WEBTHEME -> Setting %s to %s",
		          config.webserver.interface.theme.k,
		          get_web_theme_str(config.webserver.interface.theme.v.web_theme));
	}
	else
	{
		// Parameter not present in setupVars.conf
		log_debug(DEBUG_CONFIG, "setupVars.conf:WEBTHEME -> Not set (found invalid value)");
	}
}

static void get_conf_temp_unit_from_setupVars(void)
{
	// Try to obtain listening mode
	const char *temp_unit = read_setupVarsconf("TEMPERATURE_UNIT");

	if(temp_unit == NULL)
	{
		// Do not change default value, this value is not set in setupVars.conf
		log_debug(DEBUG_CONFIG, "setupVars.conf:TEMPERATURE_UNIT -> Not set");

		// Free memory, harmless to call if read_setupVarsconf() didn't return a result
		clearSetupVarsArray();
		return;
	}

	bool set = false;
	int temp_unit_enum = get_temp_unit_val(temp_unit);
	if(temp_unit_enum != -1)
	{
		set = true;
		config.webserver.api.temp.unit.v.temp_unit = temp_unit_enum;
	}

	// Free memory, harmless to call if read_setupVarsconf() didn't return a result
	clearSetupVarsArray();

	if(set)
	{
		// Parameter present in setupVars.conf
		log_debug(DEBUG_CONFIG, "setupVars.conf:TEMPERATURE_UNIT -> Setting %s to %s",
		          config.webserver.interface.theme.k,
		          get_temp_unit_str(config.webserver.api.temp.unit.v.temp_unit));
	}
	else
	{
		// Parameter not present in setupVars.conf
		log_debug(DEBUG_CONFIG, "setupVars.conf:TEMPERATURE_UNIT -> Not set (found invalid value)");
	}
}

static void get_conf_listeningMode_from_setupVars(void)
{
	// Try to obtain listening mode
	const char *listeningMode = read_setupVarsconf("DNSMASQ_LISTENING");

	if(listeningMode == NULL)
	{
		// Do not change default value, this value is not set in setupVars.conf
		log_debug(DEBUG_CONFIG, "setupVars.conf:DNSMASQ_LISTENING -> Not set");

		// Free memory, harmless to call if read_setupVarsconf() didn't return a result
		clearSetupVarsArray();
		return;
	}

	bool set = false;
	int listeningMode_enum = get_listeningMode_val(listeningMode);
	if(listeningMode_enum != -1)
	{
		set = true;
		config.dns.listeningMode.v.listeningMode = listeningMode_enum;
	}

	// Free memory, harmless to call if read_setupVarsconf() didn't return a result
	clearSetupVarsArray();

	if(set)
	{
		// Parameter present in setupVars.conf
		log_debug(DEBUG_CONFIG, "setupVars.conf:DNSMASQ_LISTENING -> Setting %s to %s",
		          config.dns.listeningMode.k, get_listeningMode_str(config.dns.listeningMode.v.listeningMode));
	}
	else
	{
		// Parameter not present in setupVars.conf
		log_debug(DEBUG_CONFIG, "setupVars.conf:DNSMASQ_LISTENING -> Not set (found invalid value)");
	}
}

void importsetupVarsConf(void)
{
	// Try to obtain password hash from setupVars.conf
	get_conf_string_from_setupVars("WEBPASSWORD", &config.webserver.api.pwhash);

	// Try to obtain blocking active boolean
	get_conf_bool_from_setupVars("BLOCKING_ENABLED", &config.dns.blocking.active);

	// Get clients which the user doesn't want to see
	get_conf_string_array_from_setupVars("API_EXCLUDE_CLIENTS", &config.webserver.api.excludeClients);

	// Get domains which the user doesn't want to see
	get_conf_string_array_from_setupVars("API_EXCLUDE_DOMAINS", &config.webserver.api.excludeDomains);

	// Try to obtain temperature hot value
	get_conf_temp_limit_from_setupVars();

	// Try to obtain password hash from setupVars.conf
	get_conf_temp_unit_from_setupVars();

	// Try to obtain web layout
	get_conf_weblayout_from_setupVars();

	// Try to obtain web theme
	get_conf_webtheme_from_setupVars();

	// Try to obtain list of upstream servers
	get_conf_upstream_servers_from_setupVars(&config.dns.upstreams);

	// Try to get Pi-hole domain
	get_conf_string_from_setupVars("PIHOLE_DOMAIN", &config.dhcp.domain);

	// Try to get bool properties (the first two are intentionally set from the same key)
	get_conf_bool_from_setupVars("DNS_FQDN_REQUIRED", &config.dns.domainNeeded);
	get_conf_bool_from_setupVars("DNS_FQDN_REQUIRED", &config.dns.expandHosts);
	get_conf_bool_from_setupVars("DNS_bogusPriv", &config.dns.bogusPriv);
	get_conf_bool_from_setupVars("DNSSEC", &config.dns.dnssec);
	get_conf_string_from_setupVars("PIHOLE_INTERFACE", &config.dns.interface);
	get_conf_string_from_setupVars("HOSTRECORD", &config.dns.hostRecord);

	// Try to obtain listening mode
	get_conf_listeningMode_from_setupVars();

	// Try to obtain REV_SERVER settings
	get_conf_bool_from_setupVars("REV_SERVER", &config.dns.revServer.active);
	get_conf_string_from_setupVars("REV_SERVER_CIDR", &config.dns.revServer.cidr);
	get_conf_string_from_setupVars("REV_SERVER_TARGET", &config.dns.revServer.target);
	get_conf_string_from_setupVars("REV_SERVER_DOMAIN", &config.dns.revServer.domain);

	// Try to obtain DHCP settings
	get_conf_bool_from_setupVars("DHCP_ACTIVE", &config.dhcp.active);
	get_conf_string_from_setupVars("DHCP_START", &config.dhcp.start);
	get_conf_string_from_setupVars("DHCP_END", &config.dhcp.end);
	get_conf_string_from_setupVars("DHCP_ROUTER", &config.dhcp.router);
	get_conf_string_from_setupVars("DHCP_LEASETIME", &config.dhcp.leaseTime);

	// If the DHCP lease time is set to "24", it is interpreted as "24h".
	// This is some relic from the past that may still be present in some
	// setups
	if(strcmp(config.dhcp.leaseTime.v.s, "24") == 0)
	{
		if(config.dhcp.leaseTime.t == CONF_STRING_ALLOCATED)
			free(config.dhcp.leaseTime.v.s);
		config.dhcp.leaseTime.v.s = strdup("24h");
		config.dhcp.leaseTime.t = CONF_STRING_ALLOCATED;
	}

	get_conf_bool_from_setupVars("DHCP_IPv6", &config.dhcp.ipv6);
	get_conf_bool_from_setupVars("DHCP_RAPID_COMMIT", &config.dhcp.rapidCommit);

	get_conf_bool_from_setupVars("queryLogging", &config.dns.queryLogging);
}

char* __attribute__((pure)) find_equals(char *s)
{
	const char *chars = "=";
	// Make s point to the first char after the "=" sign
	while (*s && (!chars || !strchr(chars, *s)))
		s++;

	// additionally, we have to check if there is a whitespace and end here
	// (there may be a bash comment afterwards)
	char *p = s;
	while(*p && *p != ' ')
		p++;
	*p = '\0';

	return s;
}

void trim_whitespace(char *string)
{
	// isspace(char*) man page:
	// checks for white-space  characters. In the "C" and "POSIX"
	// locales, these are: space, form-feed ('\f'), newline ('\n'),
	// carriage return ('\r'), horizontal tab ('\t'), and vertical tab
	// ('\v').
	char *original = string, *modified = string;
	// Trim any whitespace characters (see above) at the beginning by increasing the pointer address
	while (isspace((unsigned char)*original))
		original++;
	// Copy the content of original into modified as long as there is something in original
	while ((*modified = *original++) != '\0')
		modified++;
	// Trim any whitespace characters (see above) at the end of the string by overwriting it
	// with the zero character (marking the end of a C string)
	while (modified > string && isspace((unsigned char)*--modified))
		*modified = '\0';
}

// This will hold the read string
// in memory and will serve the space
// we will point to in the rest of the
// process (e.g. setupVarsArray will
// actually point to memory addresses
// which we allocate for this buffer.
char * linebuffer = NULL;
size_t linebuffersize = 0;

char *read_setupVarsconf(const char *key)
{
	FILE *setupVarsfp;
	if((setupVarsfp = fopen(config.files.setupVars.v.s, "r")) == NULL)
	{
		log_debug(DEBUG_CONFIG, "Reading setupVars.conf failed: %s", strerror(errno));
		return NULL;
	}

	// Allocate keystr
	char * keystr = calloc(strlen(key)+2, sizeof(char));
	if(keystr == NULL)
	{
		log_warn("read_setupVarsconf(%s) failed: Could not allocate memory for keystr", key);
		fclose(setupVarsfp);
		return NULL;
	}
	sprintf(keystr, "%s=", key);

	errno = 0;
	while(getline(&linebuffer, &linebuffersize, setupVarsfp) != -1)
	{
		// Memory allocation issue
		if(linebuffersize == 0 || linebuffer == NULL)
			continue;

		// Strip (possible) newline
		linebuffer[strcspn(linebuffer, "\n")] = '\0';

		// Skip comment lines
		if(linebuffer[0] == '#' || linebuffer[0] == ';')
			continue;

		// Skip lines with other keys
		if((strstr(linebuffer, keystr)) == NULL)
			continue;

		// otherwise: key found
		fclose(setupVarsfp);
		free(keystr);
		return (find_equals(linebuffer) + 1);
	}

	if(errno == ENOMEM)
		log_warn("read_setupVarsconf(%s) failed: could not allocate memory for getline", key);

	// Key not found -> return NULL
	fclose(setupVarsfp);

	// Freeing keystr, not setting to NULL, since not used outside of this routine
	free(keystr);

	return NULL;
}

// split string in form:
//   abc,def,ghi
// into char ** array:
// setupVarsArray[0] = abc
// setupVarsArray[1] = def
// setupVarsArray[2] = ghi
// setupVarsArray[3] = NULL
void getSetupVarsArray(const char * input)
{
	char * p = strtok((char*)input, ",");

	/* split string and append tokens to 'res' */

	while (p) {
		setupVarsArray = realloc(setupVarsArray, sizeof(char*) * ++setupVarsElements);
		if(setupVarsArray == NULL) return;
		setupVarsArray[setupVarsElements-1] = p;
		p = strtok(NULL, ",");
	}

	/* realloc one extra element for the last NULL */
	setupVarsArray = realloc(setupVarsArray, sizeof(char*) * (setupVarsElements+1));
	if(setupVarsArray == NULL) return;
	setupVarsArray[setupVarsElements] = NULL;
}

void clearSetupVarsArray(void)
{
	setupVarsElements = 0;
	// setting unused pointers to NULL
	// protecting against dangling pointer bugs
	// free only if not already NULL
	if(setupVarsArray != NULL)
	{
		free(setupVarsArray);
		setupVarsArray = NULL;
	}
	if(linebuffer != NULL)
	{
		free(linebuffer);
		linebuffersize = 0;
		linebuffer = NULL;
	}
}

bool __attribute__((pure)) getSetupVarsBool(const char * input)
{
	return (strcmp(input, "true")) == 0;
}
