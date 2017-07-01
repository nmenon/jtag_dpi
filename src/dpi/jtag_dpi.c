
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include  <string.h>


#ifdef __cplusplus
extern "C" {
#endif
uint8_t jp_waiting;
uint8_t count_comp;
uint8_t jp_got_con;

static int jp_server_p;		// The listening socket
static int jp_client_p;		// The socket for communicating with Remote

int socket_port;

static int server_socket_open()
{
	struct sockaddr_in addr;
	int ret;
	int yes = 1;

	count_comp = 0;
	jp_waiting = 0;
	jp_got_con = 0;

	addr.sin_family = AF_INET;
	addr.sin_port = htons(socket_port);
	addr.sin_addr.s_addr = INADDR_ANY;
	memset(addr.sin_zero, '\0', sizeof(addr.sin_zero));

	jp_server_p = socket(PF_INET, SOCK_STREAM, 0);
	if (jp_server_p < 0) {
		fprintf(stderr, "Unable to create comm socket: %s\n",
			strerror(errno));
		return errno;
	}

	if (setsockopt(jp_server_p, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))
	    == -1) {
		fprintf(stderr, "Unable to setsockopt on the socket: %s\n",
			strerror(errno));
		return -1;
	}

	if (bind(jp_server_p, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
		fprintf(stderr, "Unable to bind the socket: %s\n",
			strerror(errno));
		return -1;
	}

	if (listen(jp_server_p, 1) == -1) {
		fprintf(stderr, "Unable to listen: %s\n", strerror(errno));
		return -1;
	}

	ret = fcntl(jp_server_p, F_GETFL);
	ret |= O_NONBLOCK;
	fcntl(jp_server_p, F_SETFL, ret);

	fprintf(stderr, "Listening on port %d\n", socket_port);
	return 0;
}

static int client_recv(unsigned char *const jtag_tms,
		       unsigned char *const jtag_tck,
		       unsigned char *const jtag_trst,
		       unsigned char *const jtag_tdi,
		       unsigned char *const jtag_new_data_available,
		       const unsigned char jtag_tdo)
{
	uint8_t dat;
	int ret;

	ret = recv(jp_client_p, &dat, 1, 0);

	// check connection abort
	if ((ret == -1 && errno != EWOULDBLOCK) || (ret == 0)) {
		printf("JTAG Connection closed\n");

		close(jp_client_p);
		*jtag_new_data_available = 0;
		return server_socket_open();
	}
	// no available data
	if (ret == -1 && errno == EWOULDBLOCK) {

		*jtag_new_data_available = 0;
		return 0;
	}

	if (dat & 0x80) {
		switch (dat & 0x7f) {
		case 0:
			send(jp_client_p, &jtag_tdo, 1, 0);
			return 0;
		case 1:
			// jp wants a time-out
			if (count_comp) {
				dat = 0xFF;	// A value of 0xFF is expected, but not required
				send(jp_client_p, &dat, 1, 0);
			} else {
				jp_waiting = 1;
			}
			return 0;
		default:
			return 1;
		}
	}

	*jtag_tck = (dat & 0x1) >> 0;
	*jtag_trst = (dat & 0x2) >> 1;
	*jtag_tdi = (dat & 0x4) >> 2;
	*jtag_tms = (dat & 0x8) >> 3;

	dat |= 0x10;
	ret = send(jp_client_p, &dat, 1, 0);

	return 0;
}

#if 0
void jtag_timeout()
{
	uint8_t dat = 0xFF;
	if (jp_waiting) {
		send(jp_client_p, &dat, 1, 0);
		jp_waiting = 0;
	}

	count_comp = 1;
}
#endif

// Checks to see if we got a connection
static int client_check_con()
{
	int ret;

	if ((jp_client_p = accept(jp_server_p, NULL, NULL)) == -1) {
		if (errno == EAGAIN)
			return 1;

		fprintf(stderr, "Unable to accept connection: %s\n",
			strerror(errno));
		return 1;
	}
	// Set the comm socket to non-blocking.
	ret = fcntl(jp_client_p, F_GETFL);
	ret |= O_NONBLOCK;
	fcntl(jp_client_p, F_SETFL, ret);
	// Close the server socket, so that the port can be taken again
	// if the simulator is reset.
	close(jp_server_p);

	printf("JTAG communication connected!\n");
	jp_got_con = 1;
	return 0;
}

int jtag_server_init(const int port)
{
	socket_port = port;

	return server_socket_open();
}

int jtag_server_tick(unsigned char *const jtag_tms,
		     unsigned char *const jtag_tck,
		     unsigned char *const jtag_trst,
		     unsigned char *const jtag_tdi,
		     unsigned char *const jtag_new_data_available,
		     const unsigned char jtag_tdo)
{

	if (!jp_got_con) {
		if (client_check_con()) {
			*jtag_new_data_available = 0;
			return 0;
		}
	}

	return client_recv(jtag_tms, jtag_tck, jtag_trst, jtag_tdi,
			   jtag_new_data_available, jtag_tdo);
}
#ifdef __cplusplus
}
#endif

/* vim: set ai: ts=8 sw=8 noet: */
