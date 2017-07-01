#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

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

	while (1) {
		// TCK, TRST, TDI and TMS should twiggle in sequence?
		data_tx = 1 << idx % 4;
		if (send(sd, &data_tx, sizeof(data_tx), 0) < 0) {
			printf("%d tx error\n", idx);
		} else {
			printf("%5d Tx: %02x OK\n", idx, data_tx);
		}
		idx++;
		if (recv(sd, &data_rx, sizeof(data_rx), 0) < 0) {
			printf("Rx error\n");
		} else {
			printf("Rx: %02x OK\n", data_rx);
		}
		if (idx > 100)
			break;
	}

	// Exit the sim..
	data_tx = 0x83;
	if (send(sd, &data_tx, sizeof(data_tx), 0) < 0) {
		printf("%d tx error\n", idx);
	} else {
		printf("%5d Tx: %02x OK\n", idx, data_tx);
	}
	close(sd);

	return 0;
}
