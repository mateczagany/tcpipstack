#pragma once

#include <stdint.h>
#include <netinet/in.h>
#include <string.h>

#include "skbuff.h"
#include "sock.h"
#include "eth.h"
#include "ipv4.h"
#include "utils.h"


#define TCP_HEADER_SIZE 20
#define TCP_OPTS_MAXLEN 32
#define TCP_DEFAULT_MSS 536
#define TCP_START_WINDOW_SIZE 29200

#define TCP_OPTIONS_END 0
#define TCP_OPTIONS_NOOP 1
#define TCP_OPTIONS_MSS 2
#define TCP_OPTIONS_WSCALE 3
#define TCP_OPTIONS_SACK_PERMITTED 4
#define TCP_OPTIONS_SACK 5
#define TCP_OPTIONS_TIMESTAMP 8

#define TCP_MAX_SOCKETS 32

// Timers
#define TCP_T_SLOW_INTERVAL 500 * 1000  // slow timer should run every 500ms
#define TCP_T_FAST_INTERVAL 100 * 1000  // fast timer should run every 100ms, TODO: delayed ack timer
#define TCP_T_COUNT 4  // four timers

#define TCP_T_RETRANSMISSION 0
#define TCP_T_PERSIST 1
#define TCP_T_KEEPALIVE 2  // keep-alive OR connection-establishment timer
#define TCP_T_2MSL 3

#define TCP_TV_MSL 120 // maximum segment life in seconds, RFC 793 states 2 minutes
#define TCP_TV_RETRANSMISSION_MIN 1
#define TCP_TV_RETRANSMISSION_MAX 64
#define TCP_TV_PERSIST_MIN 5
#define TCP_TV_PERSIST_MAX 64
#define TCP_TV_KEEP_INIT 75
#define TCP_TV_KEEP_IDLE 7200

#define TCP_MAX_RETRANSMISSIONS 12
#define TCP_RTT_DEFAULT 1  // default RTT



enum tcp_state {
	TCPS_CLOSED,
	TCPS_LISTEN,
	TCPS_SYN_SENT,
	TCPS_SYN_RCVD,
	TCPS_ESTABLISHED,
	TCPS_CLOSE_WAIT,
	TCPS_FIN_WAIT1,
	TCPS_CLOSING,
	TCPS_LAST_ACK,
	TCPS_FIN_WAIT2,
	TCPS_TIME_WAIT
};

struct tcp_options {
	uint16_t mss;
	uint8_t window_scale;
	uint8_t sack_permitted;
	uint32_t timestamp;
	uint32_t echo;
} __attribute__((packed)) tcp_options;

struct tcp_segment {
	uint16_t source_port;
	uint16_t dest_port;
	uint32_t seq;
	uint32_t ack_seq;
	uint8_t ns : 1, _reserved : 3, data_offset : 4;
	uint8_t fin : 1, syn : 1, rst : 1, psh : 1, ack : 1, urg : 1, ece : 1, cwr : 1;
	uint16_t window_size;
	uint16_t checksum;
	uint16_t urg_pointer;
	uint8_t data[];
}  __attribute__((packed));

struct tcp_socket {
	struct tcp_socket *next, *prev;
	struct sock sock;

	// TCP Control Block
	enum tcp_state state;
	uint16_t mss;
	uint32_t srtt;  // smoothed RTT

	uint16_t timers[TCP_T_COUNT];

	uint32_t snd_una;  // oldest unacknowledged sequence number
	uint32_t snd_nxt;  // next sequence number to be sent
	uint16_t snd_wnd;  // send window
	uint32_t snd_up;  // send urgent pointer
	uint32_t snd_wl1;  // segment sequence number used for last window update
	uint32_t snd_wl2;  // segment acknowledgment number used for last window update
	uint32_t iss;  // initial sent sequence number

	uint32_t rcv_nxt;  // next sequence number expected on an incoming segments, and is the left or lower edge of the receive window
	uint16_t rcv_wnd;  // receive window
	uint32_t rcv_up;  // receive urgent pointer
	uint32_t irs;  // initial received sequence number
};

struct tcp_socket *tcp_sockets_head;


static inline struct tcp_segment *tcp_segment_from_skb(struct sk_buff *buff) {
	return (struct tcp_segment *)(buff->data + ETHERNET_HEADER_SIZE + IP_HEADER_SIZE);
}

static inline void tcp_segment_ntoh(struct tcp_segment *tcp_segment) {
	tcp_segment->source_port = ntohs(tcp_segment->source_port);
	tcp_segment->dest_port = ntohs(tcp_segment->dest_port);
	tcp_segment->seq = ntohl(tcp_segment->seq);
	tcp_segment->ack_seq = ntohl(tcp_segment->ack_seq);
	tcp_segment->window_size = ntohs(tcp_segment->window_size);
	tcp_segment->checksum = ntohs(tcp_segment->checksum);
	tcp_segment->urg_pointer = ntohs(tcp_segment->urg_pointer);
}

uint16_t tcp_checksum(struct tcp_segment *tcp_segment, uint16_t tcp_segment_len, uint32_t source_ip, uint32_t dest_ip);
void tcp_in(struct eth_frame *frame);
struct sk_buff *tcp_create_buffer(uint16_t payload_size);


void tcp_socket_wait_2msl(struct tcp_socket *tcp_socket);
void tcp_out_data(struct tcp_socket *tcp_sock, uint8_t *data, uint16_t data_len);
void tcp_out_ack(struct tcp_socket *tcp_sock);
void tcp_out_syn(struct tcp_socket *tcp_sock);
void tcp_out_fin(struct tcp_socket *tcp_sock);
void tcp_out_synack(struct tcp_socket *tcp_sock);
void tcp_out_rst(struct tcp_socket *tcp_sock);
void tcp_out_rstack(struct tcp_socket *tcp_sock);
void *tcp_timer_slow();
void *tcp_timer_fast();


void tcp_socket_free(struct tcp_socket *tcp_socket);
struct tcp_socket* tcp_socket_new(uint32_t source_ip, uint32_t dest_ip, uint16_t source_port, uint16_t dest_port);
struct tcp_socket* tcp_socket_get(uint32_t source_ip, uint32_t dest_ip, uint16_t source_port, uint16_t dest_port);