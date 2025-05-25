#include "protocol.h"
#include "csapp.h"
#include "debug.h"

// readn without EINTR retry (modification of CSAPP wrapper rio_readn)
static ssize_t readn(int fd, void *usrbuf, size_t n) {
    size_t nleft = n;
    ssize_t nread;
    char *bufp = usrbuf;
    while (nleft > 0) {
        nread = read(fd, bufp, nleft);
        if (nread < 0)
            return -1; // no retry on EINTR
        else if (nread == 0)
            break; // EOF or real error
        nleft -= nread;
        bufp += nread;
    }
    return (n - nleft);
}

int proto_send_packet(int fd, MZW_PACKET *pkt, void *data) {
    if(!pkt){ // ensure incoming pkt is non-NULL
        return -1;
    }
    MZW_PACKET netpkt = {
        .type = pkt->type,
        .param1 = pkt->param1,
        .param2 = pkt->param2,
        .param3 = pkt->param3,
        .size = htons(pkt->size),
        .timestamp_sec = htonl(pkt->timestamp_sec),
        .timestamp_nsec = htonl(pkt->timestamp_nsec)
    };
    // send header
    ssize_t size = rio_writen(fd, &netpkt, sizeof netpkt);
    if (size != sizeof(netpkt)) return -1;
    // send payload if exists
    if (pkt->size > 0 && data) {
        size = rio_writen(fd, data, pkt->size);
        if (size != pkt->size) return -1;
    }
    return 0;
}

int proto_recv_packet(int fd, MZW_PACKET *pkt, void **datap) {
    if(!pkt || !datap){ // ensure incoming pkt and datap are non-NULL
        return -1;
    }
    MZW_PACKET netpkt;
    ssize_t size = readn(fd, &netpkt, sizeof(netpkt)); // read header
    if (size != sizeof(netpkt)) return -1; // real error or EOF
    // convert data into network-byte order
    pkt->type = netpkt.type;
    pkt->param1 = netpkt.param1;
    pkt->param2 = netpkt.param2;
    pkt->param3 = netpkt.param3;
    pkt->size = ntohs(netpkt.size);
    pkt->timestamp_sec = ntohl(netpkt.timestamp_sec);
    pkt->timestamp_nsec = ntohl(netpkt.timestamp_nsec);
    // read payload if exists
    if (pkt->size > 0) {
        void *buf = Malloc(pkt->size);
        if (!buf) return -1;
        size = readn(fd, buf, pkt->size);
        if (size != pkt->size) {
            free(buf);
            return -1;
        }
        *datap = buf; // pointer to data payload
    } else {
        *datap = NULL; // else NULL
    }
    return 0;
}