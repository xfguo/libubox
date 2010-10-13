#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include "usock.h"

int usock(int type, const char *host, const char *service) {
	int sock = -1;

	if (service && !(type & USOCK_UNIX)) {
		struct addrinfo *result, *rp;

		struct addrinfo hints = {
			.ai_family = (type & USOCK_IPV6ONLY) ? AF_INET6 :
				(type & USOCK_IPV4ONLY) ? AF_INET : AF_UNSPEC,
			.ai_socktype = ((type & 0xff) == USOCK_TCP)
				? SOCK_STREAM : SOCK_DGRAM,
			.ai_flags = AI_ADDRCONFIG
				| ((type & USOCK_SERVER) ? AI_PASSIVE : 0)
				| ((type & USOCK_NUMERIC) ? AI_NUMERICHOST : 0),
		};

		if (getaddrinfo(host, service, &hints, &result)) {
			return -1;
		}

		for (rp = result; rp != NULL; rp = rp->ai_next) {
			if ((sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol))
			== -1) {
				continue;
			}

			if (!(type & USOCK_NOCLOEXEC)) {
				fcntl(sock, F_SETFD, fcntl(sock, F_GETFD) | FD_CLOEXEC);
			}

			if (type & USOCK_NONBLOCK) {
				fcntl(sock, F_SETFL, fcntl(sock, F_GETFL) | O_NONBLOCK);
			}

			if (type & USOCK_SERVER) {
				const int one = 1;
				setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

				if (!bind(sock, rp->ai_addr, rp->ai_addrlen)
				&& ((type & 0xff) != USOCK_TCP || !listen(sock, SOMAXCONN))) {
					break;
				}
			} else {
				if (!connect(sock, rp->ai_addr, rp->ai_addrlen)
				|| errno == EINPROGRESS) {
					break;
				}
			}

			close(sock);
			sock = -1;
		}
		freeaddrinfo(result);
	} else {
		struct sockaddr_un sun = {.sun_family = AF_UNIX};
		if (strlen(host) >= sizeof(sun.sun_path)) {
			errno = EINVAL;
			return -1;
		}
		strcpy(sun.sun_path, host);

		if ((sock = socket(AF_UNIX, ((type & 0xff) == USOCK_TCP)
				? SOCK_STREAM : SOCK_DGRAM, 0)) == -1) {
			return -1;
		}

		if (!(type & USOCK_NOCLOEXEC)) {
			fcntl(sock, F_SETFD, fcntl(sock, F_GETFD) | FD_CLOEXEC);
		}

		if (type & USOCK_NONBLOCK) {
			fcntl(sock, F_SETFL, fcntl(sock, F_GETFL) | O_NONBLOCK);
		}

		if (type & USOCK_SERVER) {
			if (bind(sock, (struct sockaddr*)&sun, sizeof(sun)) ||
			((type & 0xff) == USOCK_TCP && listen(sock, SOMAXCONN))) {
				close(sock);
				return -1;
			}
		} else {
			if (connect(sock, (struct sockaddr*)&sun, sizeof(sun))
			&& errno != EINPROGRESS) {
				close(sock);
				return -1;
			}
		}
	}
	return sock;
}
