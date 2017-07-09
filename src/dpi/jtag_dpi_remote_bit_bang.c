//   Copyright (C) 2017 Texas Instruments Incorporated - http://www.ti.com/
//   Nishanth Menon

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <string.h>

/* *INDENT-OFF* */
#ifdef __cplusplus
extern "C" {
#endif
/* *INDENT-ON* */

/*XXX: Define TEST_FRAMEWORK as 0 */
#ifndef JTAG_BB_TEST_FRAMEWORK
#define JTAG_BB_TEST_FRAMEWORK 1
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

#define MODULE_NAME "jtag_dpi_remote_bb: "
#if JTAG_BB_TEST_FRAMEWORK
#define DEBUG_PRINT(...) printf(MODULE_NAME "DEBUG: "  __VA_ARGS__)
#else
#define DEBUG_PRINT(...)
#endif
#define INFO_PRINT(...) printf(MODULE_NAME "INFO: "  __VA_ARGS__)
#define ERROR_PRINT(...) printf(MODULE_NAME "ERROR: "  __VA_ARGS__)

/**
 * struct server_info - Maintains the server params
 * @jp_got_con:		Are we connected?
 * @jp_server_p:	The listening socket
 * @jp_client_p:	The socket for communicating with remote
 * @socket_port:	What port to hook server to?
 */
struct server_info {
	uint8_t jp_got_con;
	int jp_server_p;	/* The listening socket */
	int jp_client_p;	/* The socket for communicating with Remote */

	int socket_port;
};

/* Server information instance */
static struct server_info si;

/**
 * server_socket_open() - Helper function to open a server socket.
 *
 * Return: 0 if all goes good, else corresponding error
 */
static int server_socket_open()
{
	struct sockaddr_in addr;
	int ret;
	int yes = 1;

	si.jp_got_con = 0;

	addr.sin_family = AF_INET;
	addr.sin_port = htons(si.socket_port);
	addr.sin_addr.s_addr = INADDR_ANY;
	memset(addr.sin_zero, '\0', sizeof(addr.sin_zero));

	si.jp_server_p = socket(PF_INET, SOCK_STREAM, 0);
	if (si.jp_server_p < 0) {
		ERROR_PRINT("Unable to create comm socket: %s\n",
			    strerror(errno));
		return errno;
	}

	if (setsockopt(si.jp_server_p, SOL_SOCKET, SO_REUSEADDR,
		       &yes, sizeof(int)) == -1) {
		ERROR_PRINT("Unable to setsockopt on the socket: %s\n",
			    strerror(errno));
		return -1;
	}

	if (bind(si.jp_server_p, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
		ERROR_PRINT("Unable to bind the socket: %s\n", strerror(errno));
		return -1;
	}

	if (listen(si.jp_server_p, 1) == -1) {
		ERROR_PRINT("Unable to listen: %s\n", strerror(errno));
		return -1;
	}

	ret = fcntl(si.jp_server_p, F_GETFL);
	ret |= O_NONBLOCK;
	fcntl(si.jp_server_p, F_SETFL, ret);

	INFO_PRINT("Listening on port %d\n", si.socket_port);
	return 0;
}

/**
 * client_recv() - Actual processing of rx data from remote bitbang client
 * @jtag_tms:	comm data with verilog
 * @jtag_tck:	comm data with verilog
 * @jtag_trst:	comm data with verilog
 * @jtag_srst:	comm data with verilog
 * @jtag_tdi:	comm data with verilog
 * @jtag_blink:	comm data with verilog
 * @bl_data_avail:	comm data with verilog
 * @wr_data_avail:	comm data with verilog
 * @rst_data_avail:	comm data with verilog
 * @send_tdo:	comm data with verilog
 *
 * Return: 0 if all goes good, else corresponding error
 */
static int client_recv(unsigned char *const jtag_tms,
		       unsigned char *const jtag_tck,
		       unsigned char *const jtag_trst,
		       unsigned char *const jtag_srst,
		       unsigned char *const jtag_tdi,
		       unsigned char *const jtag_blink,
		       unsigned char *const bl_data_avail,
		       unsigned char *const wr_data_avail,
		       unsigned char *const rst_data_avail,
		       unsigned char *const send_tdo)
{
	uint8_t dat;
	int ret;

	ret = recv(si.jp_client_p, &dat, 1, 0);

	/* Check connection abort */
	if ((ret == -1 && errno != EWOULDBLOCK) || (ret == 0)) {
		ERROR_PRINT("JTAG Connection closed\n");
		close(si.jp_client_p);
		return server_socket_open();
	}
	/* no available data */
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
		DEBUG_PRINT("TX\n");
		*send_tdo = 1;
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
		close(si.jp_client_p);
		return server_socket_open();
#endif
	default:
		DEBUG_PRINT("Unknown request: '%c'\n", dat);
		/* Fall through */
	}
	return 0;
}

/**
 * client_check_con() - Checks to see if we got a connection
 *
 * Return: 0 if all goes good, else corresponding error
 */
static int client_check_con()
{
	int ret;

	if ((si.jp_client_p = accept(si.jp_server_p, NULL, NULL)) == -1) {
		if (errno == EAGAIN)
			return 1;

		DEBUG_PRINT("Unable to accept connection: %s\n",
			    strerror(errno));
		return 1;
	}
	/* Set the comm socket to non-blocking. */
	ret = fcntl(si.jp_client_p, F_GETFL);
	ret |= O_NONBLOCK;
	fcntl(si.jp_client_p, F_SETFL, ret);
	/*
	 * Close the server socket, so that the port can be taken again
	 * if the simulator is reset.
	 */
	close(si.jp_server_p);

	DEBUG_PRINT("JTAG communication connected!\n");
	si.jp_got_con = 1;
	return 0;
}

/**
 * jtag_server_init() - Called during enable to startup a server port
 * @port:	Network port number
 *
 * Return: 0 if all goes good, else corresponding error
 */
int jtag_server_init(const int port)
{
	si.socket_port = port;

	return server_socket_open();
}

/**
 * jtag_server_deinit() - Shutdown the network server
 *
 */
void jtag_server_deinit(void)
{
	close(si.jp_server_p);
	close(si.jp_client_p);
	si.jp_got_con = 0;
}

/**
 * jtag_server_tick() - Called for every clock cycle if server is enabled.
 * @jtag_tms:	comm data with verilog
 * @jtag_tck:	comm data with verilog
 * @jtag_trst:	comm data with verilog
 * @jtag_srst:	comm data with verilog
 * @jtag_tdi:	comm data with verilog
 * @jtag_blink:	comm data with verilog
 * @bl_data_avail:	comm data with verilog
 * @wr_data_avail:	comm data with verilog
 * @rst_data_avail:	comm data with verilog
 * @send_tdo:	comm data with verilog
 *
 * Return: 0 if all goes good, else corresponding error
 */
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
		     unsigned char *const send_tdo)
{

	*rst_data_avail = 0;
	*wr_data_avail = 0;
	*bl_data_avail = 0;
	*send_tdo = 0;
	if (!si.jp_got_con) {
		if (client_check_con()) {
			*jtag_client_on = si.jp_got_con;
			return 0;
		}
	}
	*jtag_client_on = si.jp_got_con;

	return client_recv(jtag_tms, jtag_tck, jtag_trst, jtag_srst,
			   jtag_tdi, jtag_blink, bl_data_avail,
			   wr_data_avail, rst_data_avail, send_tdo);
}

/**
 * jtag_server_send() - Called if data has to be transmitted to remote client
 * @jtag_tdo:	TDO data
 *
 * NOTE: This assumes that it was invoked as response to send_tdo being set
 * in tick function.
 *
 * Return: 0 if all goes good, else corresponding error
 */
int jtag_server_send(unsigned char const jtag_tdo)
{
	uint8_t dat;

	dat = '0' + jtag_tdo;
	DEBUG_PRINT("Read = '%c'\n", dat);
	send(si.jp_client_p, &dat, 1, 0);
	return 0;
}


/* *INDENT-OFF* */
#ifdef __cplusplus
}
#endif
/* *INDENT-ON* */

/* vim: set ai: ts=8 sw=8 noet: */
