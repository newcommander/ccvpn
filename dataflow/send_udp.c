#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define RECV_BUF_SIZE 500

int alloc_and_read_data(const char *filename, char **buf, ssize_t *len)
{
	struct stat st;
	size_t remain;
	int fd, ret;
	char *p;

	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "open file failed: %s, %s\n", filename, strerror(errno));
		return -1;
	}

	if (fstat(fd, &st) < 0) {
		fprintf(stderr, "stat file failed: %s, %s\n", filename, strerror(errno));
		close(fd);
		return -1;
	}

	*buf = (char*)calloc(st.st_size, 1);
	if (!(*buf)) {
		fprintf(stderr, "alloc buf failed: %s\n", strerror(errno));
		close(fd);
		return -1;
	}

	remain = st.st_size;
	p = *buf;
	while (remain) {
		ret = read(fd, p, remain);
		if (ret < 0) {
			fprintf(stderr, "read data failed: %s\n", strerror(errno));
			free(*buf);
			close(fd);
			return -1;
		}
		remain -= ret;
		p += ret;
	}
	*len = st.st_size;

	close(fd);

	return 0;
}

int main(int argc, char *argv[])
{
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	int sfd, s, i;
	ssize_t nwrite;
	char *send_buf = NULL;

    if (argc < 4) {
        fprintf(stderr, "Usage: %s host port datafile...\n", argv[0]);
        exit(EXIT_FAILURE);
    }

	/* Obtain address(es) matching host/port */

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
	hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */
	hints.ai_flags = 0;
	hints.ai_protocol = 0;          /* Any protocol */

	s = getaddrinfo(argv[1], argv[2], &hints, &result);
	if (s != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		exit(EXIT_FAILURE);
	}

	/* getaddrinfo() returns a list of address structures.
	 * Try each address until we successfully connect(2).
	 * If socket(2) (or connect(2)) fails, we (close the socket
	 * and) try the next address. */

	for (rp = result; rp != NULL; rp = rp->ai_next) {
		sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sfd == -1)
			continue;

		if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
			break;                  /* Success */

		close(sfd);
	}

	if (rp == NULL) {               /* No address succeeded */
		fprintf(stderr, "Could not connect\n");
		exit(EXIT_FAILURE);
	}

	freeaddrinfo(result);           /* No longer needed */

	for (i = 3; i < argc; i++) {
		if (alloc_and_read_data(argv[i], &send_buf, &nwrite) < 0) {
			fprintf(stderr, "read data failed.\n");
			exit(EXIT_FAILURE);
		}

		if (!send_buf) {
			fprintf(stderr, "no data buf alloced.\n");
			exit(EXIT_FAILURE);
		}

		if (write(sfd, send_buf, nwrite) != nwrite) {
			fprintf(stderr, "partial/failed write\n");
			free(send_buf);
			exit(EXIT_FAILURE);
		}

		char recv_buf[RECV_BUF_SIZE];
        memset(recv_buf, 0, RECV_BUF_SIZE);
		ssize_t nread;
		if (nwrite + 1 > RECV_BUF_SIZE) {
			fprintf(stderr,	"Ignoring long message in argument %d\n", i);
			continue;
		}

		nread = read(sfd, recv_buf, RECV_BUF_SIZE);
		if (nread == -1) {
			perror("read");
			free(send_buf);
			exit(EXIT_FAILURE);
		}
		fprintf(stdout, "Received %ld bytes: %s\n", (long) nread, recv_buf);

		free(send_buf);
	}

	exit(EXIT_SUCCESS);
}
