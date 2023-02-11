/* Pi-hole: A black hole for Internet advertisements
*  (c) 2021 Pi-hole, LLC (https://pi-hole.net)
*  Network-wide ad blocking via your own hardware.
*
*  FTL Engine
*  FTL struct metrics definition
*
*  This file is copyright under the latest version of the EUPL.
*  Please see LICENSE file for your rights under this license. */
#ifndef METRICS_H
#define METRICS_H

struct metrics
{
	struct dns
	{
		struct cache
		{
			// <cache-size> is obvious
			int size;
			// It means the resolver handled <cache-inserted> names lookups that
			// needed to be sent to upstream servers and that <cache-live-freed>
			// was thrown out of the cache before reaching the end of its
			// time-to-live, to make room for a newer name.
			// For <cache-live-freed>, smaller is better. New queries are always
			// cached. If the cache is full with entries which haven't reached
			// the end of their time-to-live, then the entry which hasn't been
			// looked up for the longest time is evicted.
			int live_freed;
			int inserted;
			// <expired> cache entries (to be removed when space is needed)
			int expired;
			// <immortal> cache records never expire (e.g. from /etc/hosts)
			int immortal;
			// <content> are cache entries with positive remaining TTL
			struct content
			{
				int a;
				int aaaa;
				int cname;
				int srv;
				int ds;
				int dnskey;
				int other;
			} content;
		} cache;
		int local_answered;
		int forwarded_queries;
		int stale_answered;
		int unanswered_queries;
		int auth_answered;
	} dns;
	struct dhcp {
		int ack;
		int decline;
		int discover;
		int inform;
		int nak;
		int offer;
		int release;
		int request;
		int noanswer;
		struct leases
		{
			int allocated_4;
			int pruned_4;
			int allocated_6;
			int pruned_6;
		} leases;
		int bootp;
		int pxe;
	} dhcp;
};

void get_dnsmasq_metrics(struct metrics *ci);

#endif // METRICS_H
