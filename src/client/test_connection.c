#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

static char test_vectors[]= {
	'B',
	'b',
	'0',
	'1',
	'2',
	'3',
	'4',
	'5',
	'6',
	'r',
	's',
	't',
	'u',
	 0,
};
int main(int argc, char **argv)
{
	int sd;
	// Use proper arg parsing.. eventually
	int port = 9000;
	char *hostname = "localhost";
	int start;
	int end;
	int rval;
	struct hostent *hostaddr;
	struct sockaddr_in servaddr;
	uint8_t data_tx, data_rx;
	char tck, trstn, tdi, tms;
	int idx = 0;
	int i = 0;

	sd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sd == -1) {
		perror("Socket()\n");
		return (errno);
	}

	memset(&servaddr, 0, sizeof(servaddr));

	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);
	hostaddr = gethostbyname(hostname);

	memcpy(&servaddr.sin_addr, hostaddr->h_addr, hostaddr->h_length);

	rval = connect(sd, (struct sockaddr *)&servaddr, sizeof(servaddr));
	if (rval == -1) {
		printf("Host %s Port %d is closed\n", hostname, port);
		close(sd);
		return rval;
	} else {
		printf("Host %s Port %d is open\n", hostname, port);
	}

	for (i = 0; i < 10; i++ ) {
		idx = 0;
		while (test_vectors[idx]) {
			data_tx = test_vectors[idx];
			if (send(sd, &data_tx, sizeof(data_tx), 0) < 0) {
				printf("%d tx error\n", idx);
			} else {
				printf("%5d Tx: %02x[%c] OK\n", idx, data_tx, data_tx);
			}
			idx++;
		}
	}

	data_tx = 'R';
	if (send(sd, &data_tx, sizeof(data_tx), 0) < 0) {
		printf("%d tx error\n", idx);
	} else {
		printf("%5d Tx: %02x[%c] OK\n", idx, data_tx, data_tx);
	}
	if (recv(sd, &data_rx, sizeof(data_rx), 0) < 0) {
		printf("Rx error\n");
	} else {
		printf("Rx: %02x[%c] OK\n", data_rx, data_rx);
	}

	// Exit the sim..
	data_tx = 'Q';
	if (send(sd, &data_tx, sizeof(data_tx), 0) < 0) {
		printf("%d tx error\n", idx);
	} else {
		printf("%5d Tx: %02x[%c] OK\n", idx, data_tx, data_tx);
	}
	close(sd);

	return 0;
}
