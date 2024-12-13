// SPDX-License-Identifier: BSD-3-Clause

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/sendfile.h>
#include <sys/eventfd.h>
#include <libaio.h>
#include <errno.h>

#include "aws.h"
#include "utils/util.h"
#include "utils/debug.h"
#include "utils/sock_util.h"
#include "utils/w_epoll.h"

/* server socket file descriptor */
static int listenfd;

/* epoll file descriptor */
static int epollfd;

static io_context_t ctx;

static int aws_on_path_cb(http_parser *p, const char *buf, size_t len)
{
	struct connection *conn = (struct connection *)p->data;
	memcpy(conn->request_path, buf, len);
	conn->request_path[len] = '\0';
    char buff[BUFSIZ];
    buff[0] = '.';
    buff[1] = '\0';
    strcat(buff, conn->request_path);
    strcpy(conn->request_path, buff);
	conn->have_path = 1;
	return 0;
}

static void connection_prepare_send_reply_header(struct connection *conn)
{
	/* TODO: Prepare the connection buffer to send the reply header. */
    int fd = open(conn->request_path, O_RDONLY);
    struct stat file_stat;
    fstat(fd, &file_stat);
    conn->file_size = file_stat.st_size;
    close(fd);
    char buff[BUFSIZ];
    // dlog(LOG_INFO, "File size: %ld\n", conn->file_size);
    snprintf(buff, BUFSIZ, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: %zu\r\n\r\n", conn->file_size);
    strcpy(conn->send_buffer, buff);
    conn->send_len = strlen(conn->send_buffer);
    int ret = 0;
    while(ret < conn->send_len) {
        int s;
        s = send(conn->sockfd, conn->send_buffer + ret, conn->send_len - ret, 0);
        if (s == -1) {
            break;
        }
        ret += s;
    }
}

static void connection_prepare_send_404(struct connection *conn)
{
	/* TODO: Prepare the connection buffer to send the 404 header. */
    snprintf(conn->send_buffer, BUFSIZ, "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\nContent-Length: 0\r\n\r\n");
    conn->send_len = strlen(conn->send_buffer);
    int ret = 0;
    while(ret < conn->send_len) {
        int s;
        s = send(conn->sockfd, conn->send_buffer + ret, conn->send_len - ret, 0);
        if (s == -1) {
            perror("send");
            exit(1);
        }
        ret += s;
    }
}

static enum resource_type connection_get_resource_type(struct connection *conn)
{
	/* TODO: Get resource type depending on request path/filename. Filename should
	 * point to the static or dynamic folder.
	 */
	return RESOURCE_TYPE_NONE;
}


struct connection *connection_create(int sockfd)
{
	/* TODO: Initialize connection structure on given socket. */
    struct connection *conn = malloc(sizeof(*conn));
    conn->sockfd = sockfd;
    conn->state = STATE_NO_STATE;
    conn->have_path = 0;
    conn->res_type = RESOURCE_TYPE_NONE;
    conn->file_size = 0;
    conn->async_read_len = 0;
    conn->file_pos = 0;
    conn->send_pos = 0;
    conn->send_len = 0;
    conn->recv_len = 0;
    memset(conn->filename, 0, BUFSIZ);
    memset(conn->recv_buffer, 0, BUFSIZ);
    memset(conn->send_buffer, 0, BUFSIZ);
    return conn;
}

void connection_start_async_io(struct connection *conn)
{
	/* TODO: Start asynchronous operation (read from file).
	 * Use io_submit(2) & friends for reading data asynchronously.
	 */
    conn->ctx = ctx;
    conn->file_pos = 0;
    conn->piocb[0] = &conn->iocb;
    int fd = open(conn->request_path, O_RDONLY);
    conn->fd = fd;
    struct stat file_stat;
    fstat(fd, &file_stat);
    conn->file_size = file_stat.st_size;
    dlog(LOG_INFO, "File size: %ld\n", file_stat.st_size);
    if ( file_stat.st_size < BUFSIZ ) {
        conn->async_read_len = file_stat.st_size;
    } else {
        if ( file_stat.st_size % BUFSIZ == 0 )
            conn->async_read_len = BUFSIZ;
        else
            conn->async_read_len = file_stat.st_size % BUFSIZ;
    }
    io_prep_pread(&conn->iocb, fd, conn->send_buffer, BUFSIZ, conn->file_pos);
    dlog(LOG_INFO, "Prepared read\n");
    int ret = io_submit(conn->ctx, 1, conn->piocb);
    // dlog(LOG_INFO, "Ret code io submit: %d\n", ret);
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLOUT;
    ev.data.ptr = conn;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, conn->sockfd, &ev);
}

void connection_remove(struct connection *conn)
{
	/* TODO: Remove connection handler. */
    close(conn->sockfd);
    free(conn);
}

void handle_new_connection(void)
{
	/* TODO: Handle a new connection request on the server socket. */
    int sockfd;
    socklen_t addrlen = sizeof(struct sockaddr_in);
	struct sockaddr_in addr;
	struct connection *conn;
	/* TODO: Accept new connection. */
	sockfd = accept(listenfd, (SSA *) &addr, &addrlen);
	/* TODO: Set socket to be non-blocking. */
    fcntl(sockfd, F_SETFL, O_NONBLOCK);
	/* TODO: Instantiate new connection handler. */
    conn = connection_create(sockfd);
	/* TODO: Add socket to epoll. */
    w_epoll_add_ptr_in(epollfd, sockfd, conn);
    // dlog(LOG_INFO, "Accepted connection from %s:%d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
	/* TODO: Initialize HTTP_REQUEST parser. */
    http_parser_init(&conn->request_parser, HTTP_REQUEST);
    conn->request_parser.data = conn;
}


int connection_send_data(struct connection *conn)
{
	/* May be used as a helper function. */
	/* TODO: Send as much data as possible from the connection send buffer.
	 * Returns the number of bytes sent or -1 if an error occurred
	 */
    // dlog(LOG_INFO, "Sending data. Data type: %d. File name: %s\n", conn->res_type, conn->request_path);
    if (conn->res_type == RESOURCE_TYPE_STATIC) {
        connection_prepare_send_reply_header(conn);
        conn->state = connection_send_static(conn);
        connection_remove(conn);
    } else if (conn->res_type == RESOURCE_TYPE_DYNAMIC) {
        conn->state = connection_send_dynamic(conn);
    } else {
        connection_prepare_send_404(conn);
        connection_remove(conn);
    }
	return -1;
}

void receive_data(struct connection *conn)
{
	/* TODO: Receive message on socket.
	 * Store message in recv_buffer in struct connection.
	 */
    char buf[BUFSIZ];
    conn->recv_buffer[0] = '\0';

    int ret = 1;
    int total_len = 0;
    while ( ret > 0 ) {
        ret = recv(conn->sockfd, buf, BUFSIZ, 0);
        if (ret == -1) {
            dlog(LOG_INFO, "All data READ!\n");
            break;
        }
        if ( total_len + ret > BUFSIZ ) {
            dlog(LOG_INFO, "Buffer overflow\n");
            break;
        }
        total_len += ret;
        strncat(conn->recv_buffer, buf, ret);
    }
    conn->recv_len = total_len;
    conn->recv_buffer[conn->recv_len] = '\0';
    // dlog(LOG_INFO, "Receiving data with len: %d\n", (int)conn->recv_len);
    // dlog(LOG_INFO, "buffer info: \n%s\n", conn->recv_buffer);
}

int connection_open_file(struct connection *conn)
{
	/* TODO: Open file and update connection fields. */

	return -1;
}

void connection_complete_async_io(struct connection *conn)
{
	/* TODO: Complete asynchronous operation; operation returns successfully.
	 * Prepare socket for sending.
	 */
    if ( conn->file_pos != conn->file_size ) {
        // We need to continue reading
        // If we didnt fully transmit the file
        memset(conn->send_buffer, 0, BUFSIZ);
        memset(&conn->iocb, 0, sizeof(conn->iocb));
        conn->piocb[0] = &conn->iocb;
        // dlog(LOG_INFO, " File pointer: %zu\n", lseek(conn->fd, 0, SEEK_CUR));
        io_prep_pread(&conn->iocb, conn->fd, conn->send_buffer, BUFSIZ, conn->file_pos);
        io_submit(conn->ctx, 1, conn->piocb);
    } else {
        w_epoll_remove_ptr(epollfd, conn->sockfd, conn);
        close(conn->fd);
        connection_remove(conn);
    }

}

int parse_header(struct connection *conn)
{
	/* TODO: Parse the HTTP header and extract the file path. */
	/* Use mostly null settings except for on_path callback. */
	http_parser_settings settings_on_path = {
		.on_message_begin = 0,
		.on_header_field = 0,
		.on_header_value = 0,
		.on_path = aws_on_path_cb,
		.on_url = 0,
		.on_fragment = 0,
		.on_query_string = 0,
		.on_body = 0,
		.on_headers_complete = 0,
		.on_message_complete = 0
	};
    http_parser_execute(&conn->request_parser, &settings_on_path, conn->recv_buffer, conn->recv_len);
	return 0;
}

enum connection_state connection_send_static(struct connection *conn)
{
	/* TODO: Send static data using sendfile(2). */
    // dlog(LOG_INFO, "Sending static data\n");
    int fd = open(conn->request_path, O_RDONLY);
    struct stat file_stat;
    fstat(fd, &file_stat);
    off_t offset = 0;
    // dlog(LOG_INFO, "File size: %ld\n", file_stat.st_size);
    while (offset < file_stat.st_size) {
        int ret = sendfile(conn->sockfd, fd, &offset, file_stat.st_size);
        // dlog(LOG_INFO, "Sent %d bytes\n", ret);
    }
    close(fd);
	return STATE_NO_STATE;
}



int connection_send_dynamic(struct connection *conn)
{
    connection_prepare_send_reply_header(conn);
    connection_start_async_io(conn);
	return 0;
}


void handle_input(struct connection *conn)
{
	/* TODO: Handle input information: may be a new message or notification of
	 * completion of an asynchronous I/O operation.
	 */
    // dlog(LOG_INFO, "Handling shit\n");

    receive_data(conn);
    parse_header(conn);
    // dlog(LOG_INFO, "Path info: %s\n", conn->request_path);


    if ( strstr(conn->request_path, AWS_REL_STATIC_FOLDER) != NULL ) {
        conn->res_type = RESOURCE_TYPE_STATIC;
    } else if ( strstr(conn->request_path, AWS_REL_DYNAMIC_FOLDER) != NULL ) {
        conn->res_type = RESOURCE_TYPE_DYNAMIC;
    } else {
        conn->res_type = RESOURCE_TYPE_NONE;
    }

    // Check if file exists
    int fd = open(conn->request_path, O_RDONLY);
    // dlog(LOG_INFO, "File descriptor: %d\n", fd);
    if ( fd == -1 )
        conn->res_type = RESOURCE_TYPE_NONE;
    close(fd);
    connection_send_data(conn);
}

void handle_output(struct connection *conn)
{
	/* TODO: Handle output information: may be a new valid requests or notification of
	 * completion of an asynchronous I/O operation or invalid requests.
	 */
    // dlog(LOG_INFO, "Handling output\n");
    struct io_event event[1];
	int ret = io_getevents(ctx, 1, 1, &event[0], NULL);

    // dlog(LOG_INFO, "File position: %zu\n", conn->file_pos);
    if ( conn->send_len == 0 ) {
        connection_remove(conn);
        return;
    }
    ret = 0;
    while(ret < conn->async_read_len) {
        int s;
        s = send(conn->sockfd, conn->send_buffer + ret, conn->async_read_len - ret, 0);
        if (s == -1) {
            perror("send");
            exit(1);
        }
        ret += s;
    }
    conn->file_pos += conn->async_read_len;
    connection_complete_async_io(conn);
}

void handle_client(uint32_t event, struct connection *conn)
{
	/* TODO: Handle new client. There can be input and output connections.
	 * Take care of what happened at the end of a connection.
	 */
}

int main(void)
{
	int rc;
	/* TODO: Initialize asynchronous operations. */
    io_setup(10, &ctx);
	/* TODO: Initialize multiplexing. */
    epollfd = w_epoll_create();
	/* TODO: Create server socket. */
    listenfd = tcp_create_listener(AWS_LISTEN_PORT, DEFAULT_LISTEN_BACKLOG);

	/* TODO: Add server socket to epoll object*/
    w_epoll_add_fd_in(epollfd, listenfd);

	/* Uncomment the following line for debugging. */
	// dlog(LOG_INFO, "Server waiting for connections on port %d\n", AWS_LISTEN_PORT);

	/* server main loop */
	while (1) {
		struct epoll_event rev;
        w_epoll_wait_infinite(epollfd, &rev);
        if (rev.data.fd == listenfd) {
            if (rev.events & EPOLLIN) {
                    handle_new_connection();
            } else {
                if (EPOLLOUT & rev.events) {
                    handle_output((struct connection *)rev.data.ptr);
                } else {
                    dlog(LOG_INFO, "New message\n");
                    handle_input((struct connection *)rev.data.ptr);
                }
            }
        } else {
            if (rev.events & EPOLLIN) {
                dlog(LOG_INFO, "New message\n");
                handle_input((struct connection *)rev.data.ptr);
            } else {
                if (EPOLLOUT & rev.events) {
                    handle_output((struct connection *)rev.data.ptr);
                }
            }
        }

	}

	return 0;
}
