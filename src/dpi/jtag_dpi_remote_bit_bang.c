////   Copyright (C) 2017 Texas Instruments Incorporated - http://www.ti.com/ ////
////   Nishanth Menon                                             ////

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

/*XXX: Define TEST_FRAMEWORK as 0 */
#ifndef TEST_FRAMEWORK
#define TEST_FRAMEWORK 1
#endif

/*
 * Protocol: http://repo.or.cz/openocd.git/blob/HEAD:/doc/manual/jtag/drivers/remote_bitbang.txt
 * implementation: https://github.com/myelin/teensy-openocd-remote-bitbang/blob/master/remote_bitbang_serial_debug.py
 */
#define RB_BLINK_ON	'B'
#define RB_BLINK_OFF	'b'
#define RB_READ_REQ	'R'
#define RB_QUIT_REQ	'Q'
/*	Write requests:		TCK	TMS	TDI */
#define RB_WRITE_0  '0'		/*      0       0       0   */
#define RB_WRITE_1  '1'		/*      0       0       1   */
#define RB_WRITE_2  '2'		/*      0       1       0   */
#define RB_WRITE_3  '3'		/*      0       1       1   */
#define RB_WRITE_4  '4'		/*      1       0       0   */
#define RB_WRITE_5  '5'		/*      1       0       1   */
#define RB_WRITE_6  '6'		/*      1       1       0   */
#define RB_WRITE_7  '7'		/*      1       1       1   */
/*	Reset requests:		TRST	SRST	    */
#define RB_RST_R    'r'		/*      0       0           */
#define RB_RST_S    's'		/*      0       1           */
#define RB_RST_T    't'		/*      1       0           */
#define RB_RST_U    'u'		/*      1       1           */

#if TEST_FRAMEWORK
#define DEBUG_PRINT printf
#else
#define DEBUG_PRINT(ARGS...)
#endif

	uint8_t jp_waiting;
	uint8_t count_comp;
	uint8_t jp_got_con;

	static int jp_server_p;	// The listening socket
	static int jp_client_p;	// The socket for communicating with Remote

	int socket_port;

	static int server_socket_open() {
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
			DEBUG_PRINT("Unable to create comm socket: %s\n",
				    strerror(errno));
			return errno;
		}

		if (setsockopt
		    (jp_server_p, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))
		    == -1) {
			DEBUG_PRINT("Unable to setsockopt on the socket: %s\n",
				    strerror(errno));
			return -1;
		}

		if (bind(jp_server_p, (struct sockaddr *)&addr, sizeof(addr)) ==
		    -1) {
			DEBUG_PRINT("Unable to bind the socket: %s\n",
				    strerror(errno));
			return -1;
		}

		if (listen(jp_server_p, 1) == -1) {
			DEBUG_PRINT("Unable to listen: %s\n", strerror(errno));
			return -1;
		}

		ret = fcntl(jp_server_p, F_GETFL);
		ret |= O_NONBLOCK;
		fcntl(jp_server_p, F_SETFL, ret);

		DEBUG_PRINT("Listening on port %d\n", socket_port);
		return 0;
	}

	static int client_recv(unsigned char *const jtag_tms,
			       unsigned char *const jtag_tck,
			       unsigned char *const jtag_trst,
			       unsigned char *const jtag_srst,
			       unsigned char *const jtag_tdi,
			       unsigned char *const jtag_blink,
			       unsigned char *const bl_data_avail,
			       unsigned char *const wr_data_avail,
			       unsigned char *const rst_data_avail,
			       const unsigned char jtag_tdo) {
		uint8_t dat;
		int ret;

		ret = recv(jp_client_p, &dat, 1, 0);

		// check connection abort
		if ((ret == -1 && errno != EWOULDBLOCK) || (ret == 0)) {
			DEBUG_PRINT("JTAG Connection closed\n");
			close(jp_client_p);
			return server_socket_open();
		}
		// no available data
		if (ret == -1 && errno == EWOULDBLOCK) {
			return 0;
		}

		DEBUG_PRINT("Data: %c\n", dat);
		switch (dat) {
		case RB_BLINK_ON:
		case RB_BLINK_OFF:
			*jtag_blink = (dat == RB_BLINK_ON) ? 1 : 0;
			*bl_data_avail = 1;
			break;

		case RB_READ_REQ:
			dat = '0' + jtag_tdo;
			send(jp_client_p, &dat, 1, 0);
			break;

		case RB_WRITE_0:
		case RB_WRITE_1:
		case RB_WRITE_2:
		case RB_WRITE_3:
		case RB_WRITE_4:
		case RB_WRITE_5:
		case RB_WRITE_6:
		case RB_WRITE_7:
			DEBUG_PRINT("Write %c\n", dat);
			dat -= RB_WRITE_0;
			*jtag_tdi = (dat & 0x1) >> 0;
			*jtag_tms = (dat & 0x2) >> 1;
			*jtag_tck = (dat & 0x4) >> 2;
			*wr_data_avail = 1;
			break;

		case RB_RST_R:
		case RB_RST_S:
		case RB_RST_T:
		case RB_RST_U:
			DEBUG_PRINT("RST %c\n", dat);
			dat -= RB_RST_R;
			*jtag_srst = (dat & 0x1) >> 0;
			*jtag_trst = (dat & 0x2) >> 1;
			*rst_data_avail = 1;
			break;

		case RB_QUIT_REQ:
#if TEST_FRAMEWORK
			/* Shut down sim */
			return 1;
#else
			close(jp_client_p);
			return server_socket_open();
#endif
		default:
			DEBUG_PRINT("Unknown request: '%c'\n", dat);
			/* Fall through */
		}

		return 0;
	}

// Checks to see if we got a connection
	static int client_check_con() {
		int ret;

		if ((jp_client_p = accept(jp_server_p, NULL, NULL)) == -1) {
			if (errno == EAGAIN)
				return 1;

			DEBUG_PRINT("Unable to accept connection: %s\n",
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

		DEBUG_PRINT("JTAG communication connected!\n");
		jp_got_con = 1;
		return 0;
	}

	int jtag_server_init(const int port) {
		socket_port = port;

		return server_socket_open();
	}

	int jtag_server_tick(unsigned char *const jtag_tms,
			     unsigned char *const jtag_tck,
			     unsigned char *const jtag_trst,
			     unsigned char *const jtag_srst,
			     unsigned char *const jtag_tdi,
			     unsigned char *const jtag_blink,
			     unsigned char *const bl_data_avail,
			     unsigned char *const wr_data_avail,
			     unsigned char *const rst_data_avail,
			     unsigned char *const jtag_client_on,
			     const unsigned char jtag_tdo) {

		*rst_data_avail = 0;
		*wr_data_avail = 0;
		*bl_data_avail = 0;
		if (!jp_got_con) {
			if (client_check_con()) {
				*jtag_client_on = jp_got_con;
				return 0;
			}
		}
		*jtag_client_on = jp_got_con;

		return client_recv(jtag_tms, jtag_tck, jtag_trst, jtag_srst,
				   jtag_tdi, jtag_blink, bl_data_avail,
				   wr_data_avail, rst_data_avail, jtag_tdo);
	}
#ifdef __cplusplus
}
#endif

/* vim: set ai: ts=8 sw=8 noet: */
