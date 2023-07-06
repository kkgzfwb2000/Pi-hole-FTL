/* Pi-hole: A black hole for Internet advertisements
*  (c) 2021 Pi-hole, LLC (https://pi-hole.net)
*  Network-wide ad blocking via your own hardware.
*
*  FTL Engine
*  API Implementation
*
*  This file is copyright under the latest version of the EUPL.
*  Please see LICENSE file for your rights under this license. */

#include "../FTL.h"
#include "../webserver/http-common.h"
#include "../webserver/json_macros.h"
#include "api.h"
#include "../shmem.h"
#include "../datastructure.h"
// overTime data
#include "../overTime.h"
// config struct
#include "../config/config.h"
// read_setupVarsconf()
#include "../setupVars.h"
// get_aliasclient_list()
#include "../database/aliasclients.h"

int api_history(struct ftl_conn *api)
{
	unsigned int from = 0, until = OVERTIME_SLOTS;
	const time_t now = time(NULL);
	bool found = false;

	lock_shm();
	time_t mintime = overTime[0].timestamp;

	// Start with the first non-empty overTime slot
	for(unsigned int slot = 0; slot < OVERTIME_SLOTS; slot++)
	{
		if((overTime[slot].total > 0 || overTime[slot].blocked > 0) &&
		   overTime[slot].timestamp >= mintime)
		{
			from = slot;
			found = true;
			break;
		}
	}

	// End with last non-empty overTime slot or the last slot that is not
	// older than the maximum history to be sent
	for(unsigned int slot = 0; slot < OVERTIME_SLOTS; slot++)
	{
		if(overTime[slot].timestamp >= now ||
		   overTime[slot].timestamp - now > (time_t)config.webserver.api.maxHistory.v.ui)
		{
			until = slot;
			break;
		}
	}

	// If there is no data to be sent, we send back an empty array
	// and thereby return early
	if(!found)
	{
		cJSON *json = JSON_NEW_ARRAY();
		cJSON *item = JSON_NEW_OBJECT();
		JSON_ADD_ITEM_TO_ARRAY(json, item);
		JSON_SEND_OBJECT_UNLOCK(json);
	}

	// Minimum structure is
	// {"history":[]}
	cJSON *history = JSON_NEW_ARRAY();
	for(unsigned int slot = from; slot < until; slot++)
	{
		cJSON *item = JSON_NEW_OBJECT();
		JSON_ADD_NUMBER_TO_OBJECT(item, "timestamp", overTime[slot].timestamp);
		JSON_ADD_NUMBER_TO_OBJECT(item, "total", overTime[slot].total);
		JSON_ADD_NUMBER_TO_OBJECT(item, "cached", overTime[slot].cached);
		JSON_ADD_NUMBER_TO_OBJECT(item, "blocked", overTime[slot].blocked);
		JSON_ADD_ITEM_TO_ARRAY(history, item);
	}

	cJSON *json = JSON_NEW_OBJECT();
	JSON_ADD_ITEM_TO_OBJECT(json, "history", history);
	JSON_SEND_OBJECT_UNLOCK(json);
}

int api_history_clients(struct ftl_conn *api)
{
	int sendit = false;
	unsigned int from = 0, until = OVERTIME_SLOTS;
	const time_t now = time(NULL);

	lock_shm();

	// Find minimum ID to send
	for(unsigned int slot = 0; slot < OVERTIME_SLOTS; slot++)
	{
		if((overTime[slot].total > 0 || overTime[slot].blocked > 0) &&
		   overTime[slot].timestamp >= overTime[0].timestamp)
		{
			sendit = true;
			from = slot;
			break;
		}
	}

	// Exit before processing any data if requested via config setting
	if(config.misc.privacylevel.v.privacy_level >= PRIVACY_HIDE_DOMAINS_CLIENTS || !sendit)
	{
		// Minimum structure is
		// {"history":[], "clients":[]}
		cJSON *json = JSON_NEW_OBJECT();
		cJSON *history = JSON_NEW_ARRAY();
		JSON_ADD_ITEM_TO_OBJECT(json, "history", history);
		cJSON *clients = JSON_NEW_ARRAY();
		JSON_ADD_ITEM_TO_OBJECT(json, "clients", clients);
		JSON_SEND_OBJECT_UNLOCK(json);
	}

	// End with last non-empty overTime slot or the last slot that is not
	// older than the maximum history to be sent
	for(unsigned int slot = 0; slot < OVERTIME_SLOTS; slot++)
	{
		if(overTime[slot].timestamp >= now ||
		   overTime[slot].timestamp - now > (time_t)config.webserver.api.maxHistory.v.ui)
		{
			until = slot;
			break;
		}
	}

	// Get clients which the user doesn't want to see
	// if skipclient[i] == true then this client should be hidden from
	// returned data. We initialize it with false
	bool *skipclient = calloc(counters->clients, sizeof(bool));

	unsigned int excludeClients = cJSON_GetArraySize(config.webserver.api.excludeClients.v.json);
	if(excludeClients > 0)
	{
		for(int clientID = 0; clientID < counters->clients; clientID++)
		{
			// Get client pointer
			const clientsData* client = getClient(clientID, true);
			if(client == NULL)
				continue;
			// Check if this client should be skipped
			for(unsigned int i = 0; i < excludeClients; i++)
			{
				cJSON *item = cJSON_GetArrayItem(config.webserver.api.excludeClients.v.json, i);
				if(strcmp(getstr(client->ippos), item->valuestring) == 0 ||
				   strcmp(getstr(client->namepos), item->valuestring) == 0)
					skipclient[clientID] = true;
			}
		}
	}

	// Also skip alias-clients
	for(int clientID = 0; clientID < counters->clients; clientID++)
	{
		// Get client pointer
		const clientsData* client = getClient(clientID, true);
		if(client == NULL)
			continue;
		if(!client->flags.aliasclient && client->aliasclient_id > -1)
			skipclient[clientID] = true;
	}

	cJSON *history = JSON_NEW_ARRAY();
	// Main return loop
	for(unsigned int slot = from; slot < until; slot++)
	{
		cJSON *item = JSON_NEW_OBJECT();
		JSON_ADD_NUMBER_TO_OBJECT(item, "timestamp", overTime[slot].timestamp);

		// Loop over clients to generate output to be sent to the client
		cJSON *data = JSON_NEW_ARRAY();
		for(int clientID = 0; clientID < counters->clients; clientID++)
		{
			if(skipclient[clientID])
				continue;

			// Get client pointer
			const clientsData* client = getClient(clientID, true);

			// Skip invalid clients and also those managed by alias clients
			if(client == NULL || client->aliasclient_id >= 0)
				continue;

			const int thisclient = client->overTime[slot];

			JSON_ADD_NUMBER_TO_ARRAY(data, thisclient);
		}
		JSON_ADD_ITEM_TO_OBJECT(item, "data", data);
		JSON_ADD_ITEM_TO_ARRAY(history, item);
	}
	cJSON *json = JSON_NEW_OBJECT();
	JSON_ADD_ITEM_TO_OBJECT(json, "history", history);

	cJSON *clients = JSON_NEW_ARRAY();
	// Loop over clients to generate output to be sent to the client
	for(int clientID = 0; clientID < counters->clients; clientID++)
	{
		if(skipclient[clientID])
			continue;

		// Get client pointer
		const clientsData* client = getClient(clientID, true);
		if(client == NULL)
			continue;

		const char *client_ip = getstr(client->ippos);
		const char *client_name = client->namepos != 0 ? getstr(client->namepos) : NULL;

		cJSON *item = JSON_NEW_OBJECT();
		JSON_REF_STR_IN_OBJECT(item, "name", client_name);
		JSON_REF_STR_IN_OBJECT(item, "ip", client_ip);
		JSON_ADD_ITEM_TO_ARRAY(clients, item);
	}
	JSON_ADD_ITEM_TO_OBJECT(json, "clients", clients);

	// Free memory
	free(skipclient);

	JSON_SEND_OBJECT_UNLOCK(json);
}
