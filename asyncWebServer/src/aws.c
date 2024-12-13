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

static int listenfd;
static int epollfd;
static io_context_t ctx;

static int aws_on_path_cb(http_parser *p, const char *buf, size_t len)
{
	struct connection *conn = (struct connection *)p->data;
	char buff[BUFSIZ];

	memcpy(conn->request_path, buf, len);
	conn->request_path[len] = '\0';
	buff[0] = '.';
	buff[1] = '\0';
	strcat(buff, conn->request_path);
	strcpy(conn->request_path, buff);
	conn->have_path = 1;
	return 0;
}

static void connection_prepare_send_reply_header(struct connection *conn)
{
	int fd = open(conn->request_path, O_RDONLY);
	struct stat file_stat;
	char buff[BUFSIZ];
	int ret = 0;

	fstat(fd, &file_stat);
	conn->file_size = file_stat.st_size;
	close(fd);
	snprintf(buff, BUFSIZ, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: %zu\r\n\r\n", conn->file_size);
	strcpy(conn->send_buffer, buff);
	conn->send_len = strlen(conn->send_buffer);
	while (ret < conn->send_len) {
		int s = send(conn->sockfd, conn->send_buffer + ret, conn->send_len - ret, 0);

		if (s == -1)
			break;
		ret += s;
	}
}

static void connection_prepare_send_404(struct connection *conn)
{
	int ret = 0;

	snprintf(conn->send_buffer, BUFSIZ, "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\nContent-Length: 0\r\n\r\n");
	conn->send_len = strlen(conn->send_buffer);
	while (ret < conn->send_len) {
		int s = send(conn->sockfd, conn->send_buffer + ret, conn->send_len - ret, 0);

		if (s == -1)
			break;
		ret += s;
	}
}

struct connection *connection_create(int sockfd)
{
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
	int fd = open(conn->request_path, O_RDONLY);
	struct stat file_stat;

	conn->ctx = ctx;
	conn->file_pos = 0;
	conn->piocb[0] = &conn->iocb;
	conn->fd = fd;
	fstat(fd, &file_stat);
	conn->file_size = file_stat.st_size;
	if (file_stat.st_size < BUFSIZ) {
		conn->async_read_len = file_stat.st_size;
	} else {
		if (file_stat.st_size % BUFSIZ == 0)
			conn->async_read_len = BUFSIZ;
		else
			conn->async_read_len = file_stat.st_size % BUFSIZ;
	}
	io_prep_pread(&conn->iocb, fd, conn->send_buffer, BUFSIZ, conn->file_pos);
	io_submit(conn->ctx, 1, conn->piocb);
	struct epoll_event ev;

	ev.events = EPOLLIN | EPOLLOUT;
	ev.data.ptr = conn;
	epoll_ctl(epollfd, EPOLL_CTL_MOD, conn->sockfd, &ev);
}

void connection_remove(struct connection *conn)
{
	close(conn->sockfd);
	free(conn);
}

void handle_new_connection(void)
{
	int sockfd;
	socklen_t addrlen = sizeof(struct sockaddr_in);
	struct sockaddr_in addr;
	struct connection *conn;

	sockfd = accept(listenfd, (SSA *) &addr, &addrlen);
	fcntl(sockfd, F_SETFL, O_NONBLOCK);
	conn = connection_create(sockfd);
	w_epoll_add_ptr_in(epollfd, sockfd, conn);
	http_parser_init(&conn->request_parser, HTTP_REQUEST);
	conn->request_parser.data = conn;
}

void connection_send_data(struct connection *conn)
{
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
}

void receive_data(struct connection *conn)
{
	char buf[BUFSIZ];
	int ret = 1;
	int total_len = 0;

	conn->recv_buffer[0] = '\0';
	while (ret > 0) {
		ret = recv(conn->sockfd, buf, BUFSIZ, 0);
		if (ret == -1)
			break;
		if (total_len + ret > BUFSIZ)
			break;
		total_len += ret;
		strncat(conn->recv_buffer, buf, ret);
	}
	conn->recv_len = total_len;
	conn->recv_buffer[conn->recv_len] = '\0';
}

void connection_complete_async_io(struct connection *conn)
{
	if (conn->file_pos != conn->file_size) {
		// We need to continue reading
		// If we didnt fully transmit the file
		io_prep_pread(&conn->iocb, conn->fd, conn->send_buffer, BUFSIZ, conn->file_pos);
		io_submit(conn->ctx, 1, conn->piocb);
	} else {
		w_epoll_remove_ptr(epollfd, conn->sockfd, conn);
		close(conn->fd);
		connection_remove(conn);
	}
}

void parse_header(struct connection *conn)
{
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
}

enum connection_state connection_send_static(struct connection *conn)
{
	int fd = open(conn->request_path, O_RDONLY);
	struct stat file_stat;
	off_t offset = 0;

	fstat(fd, &file_stat);
	while (offset < file_stat.st_size)
		sendfile(conn->sockfd, fd, &offset, file_stat.st_size);
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
	receive_data(conn);
	parse_header(conn);
	if (strstr(conn->request_path, AWS_REL_STATIC_FOLDER) != NULL)
		conn->res_type = RESOURCE_TYPE_STATIC;
	else if (strstr(conn->request_path, AWS_REL_DYNAMIC_FOLDER) != NULL)
		conn->res_type = RESOURCE_TYPE_DYNAMIC;
	else
		conn->res_type = RESOURCE_TYPE_NONE;
	int fd = open(conn->request_path, O_RDONLY);

	if (fd == -1)
		conn->res_type = RESOURCE_TYPE_NONE;
	close(fd);
	connection_send_data(conn);
}

void handle_output(struct connection *conn)
{
	struct io_event event[1];
	int ret = io_getevents(ctx, 1, 1, &event[0], NULL);

	if (conn->send_len == 0) {
		connection_remove(conn);
		return;
	}
	ret = 0;
	while (ret < conn->async_read_len) {
		int s = send(conn->sockfd, conn->send_buffer + ret, conn->async_read_len - ret, 0);

		if (s == -1)
			break;
		ret += s;
	}
	conn->file_pos += conn->async_read_len;
	connection_complete_async_io(conn);
}

int main(void)
{
	io_setup(10, &ctx);
	epollfd = w_epoll_create();
	listenfd = tcp_create_listener(AWS_LISTEN_PORT, DEFAULT_LISTEN_BACKLOG);
	w_epoll_add_fd_in(epollfd, listenfd);
	while (1) {
		struct epoll_event rev;

		w_epoll_wait_infinite(epollfd, &rev);
		if (rev.data.fd == listenfd) {
			if (rev.events & EPOLLIN)
				handle_new_connection();
			else if (EPOLLOUT & rev.events)
				handle_output((struct connection *)rev.data.ptr);
			else
				handle_input((struct connection *)rev.data.ptr);
		} else {
			if (rev.events & EPOLLIN)
				handle_input((struct connection *)rev.data.ptr);
			else
				handle_output((struct connection *)rev.data.ptr);
		}
	}
	return 0;
}
