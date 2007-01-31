
#include <netdb.h>
#include <sys/socket.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include "lib_auth.h"

void usage() {
	usage("astest <port_number>");
	exit(-1);
}

int main(int argc, char **argv)
{
	int             sockfd, new_sock;
	struct protoent *pent;
	struct sockaddr_in sa, ca;
	int             err;
	int				len;
	int csize, wsize;
	uint32_t data;
	char buf[BUFSIZ];
	auth_handle_t handle;
	
	if (argc < 2) {
		usage();
	}

	pent = getprotobyname("tcp");
	if (pent == NULL) {
		printf("failed to find tcp");
		return(ENOENT);
	}

	if ((sockfd = socket(PF_INET, SOCK_STREAM, pent->p_proto)) < 0) {
		printf("failed to create socket");
		return(ENOENT);
	}

	sa.sin_family = AF_INET;
	sa.sin_port = htons((unsigned short) atoi(argv[1]));
	sa.sin_addr.s_addr = htonl(INADDR_ANY);

	err = bind(sockfd, (struct sockaddr *) &sa, sizeof(sa));
	if (err) {
		perror("bind failed ");
		return (ENOENT);
	}

	err = listen(sockfd, SOMAXCONN);
	if (err) {
		printf("listen failed \n");
		return (ENOENT);
	}


	csize = sizeof(ca);
	new_sock = accept(sockfd, (struct sockaddr *) &ca, &csize);

	if (new_sock < 0) {
		printf("accept failed \n");
		exit(ENOENT);
	}
	
	/* authenticate connection */
	handle = auth_conn_server(new_sock);
	if (handle == NULL) {
		close(new_sock);
		return(ENOENT);
	}

	/* send an encrypted cookie */
	data = 1;
	len = auth_msg_encrypt(handle, (char *) &data, sizeof(data),
							buf, BUFSIZ);
	if (len < 0) {
		printf("Error while encrypting message\n");
		exit(1);
    }	
	printf("Encrypted cookie is %d bytes\n", len);
	
	wsize = write(new_sock, &buf[0], len);
	if (wsize < 0) {
		printf("Failed write on cntrl connection\n");
		close(new_sock);
		return(1);
	}
	
	printf("Sent %d bytes\n", wsize);

	return (0);
}
