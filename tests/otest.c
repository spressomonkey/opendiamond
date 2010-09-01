/*
 *  The OpenDiamond Platform for Interactive Search
 *  Version 4
 *
 *  Copyright (c) 2002-2005 Intel Corporation
 *  All rights reserved.
 *
 *  This software is distributed under the terms of the Eclipse Public
 *  License, Version 1.0 which can be found in the file named LICENSE.
 *  ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS SOFTWARE CONSTITUTES
 *  RECIPIENT'S ACCEPTANCE OF THIS AGREEMENT
 */

/*
 * rprintmsg.c: remote version of "printmsg.c"
 */


#include <stdio.h>
#include <rpc/rpc.h>		/* always needed */
#include "od.h"			/* msg.h will be generated by rpcgen */


int
main(int argc, char **argv)
{
	CLIENT         *cl;
	int            *result;
	char           *server;
	char           *message;
	struct update_gid_args aa;
	groupid_t       gt;
	obj_id_t        oid;

	if (!g_thread_supported()) g_thread_init(NULL);

	if (argc != 3) {
		fprintf(stderr, "usage: %s host message\n", argv[0]);
		exit(1);
	}

	server = argv[1];
	message = argv[2];


	/*
	 * Create client "handle" used for calling MESSAGEPROG on
	 * the server designated on the command line.  We tell
	 * the RPC package to use the TCP protocol when
	 * contacting the server.
	 */

	cl = clnt_create(server, MESSAGEPROG, MESSAGEVERS, "tcp");
	if (cl == NULL) {
		/*
		 * XXX no server 
		 */
		clnt_pcreateerror(server);
		exit(1);
	}

	gt = 1;
	aa.gid = &gt;
	aa.oid = &oid;

	result = rpc_add_gid_1(&aa, cl);
	if (result == NULL) {
		clnt_perror(cl, server);
		exit(1);
	}

	printf("after add: gid %llx \n", gt);


	if (*result == 0) {
		fprintf(stderr, "%s: %s couldn't print your message\n",
			argv[0], server);
		exit(1);
	}
	printf("Message delivered to %s!\n", server);
	exit(0);
}
