#include <criterion/criterion.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/time.h>

#include "protocol.h"
#include "excludes.h"

static void init() {
}

Test(protocol_suite, send_no_payload, .init = init, .timeout = 5) {
#ifdef NO_PROTOCOL
    cr_assert_fail("Protocol was not implemented");
#endif
    int fd;
    void *payload = NULL;
    MZW_PACKET pkt = {0};

    pkt.type = MZW_READY_PKT;
    pkt.param1 = 0101;
    pkt.param2 = 0202;
    pkt.param3 = 0303;
    pkt.size = 0;
    pkt.timestamp_sec = 0x11223344;
    pkt.timestamp_nsec = 0x55667788;

    fd = open("test_output/pkt_no_payload", O_CREAT|O_TRUNC|O_RDWR, 0644);
    cr_assert(fd > 0, "Failed to create output file");
    int ret = proto_send_packet(fd, &pkt, payload);
    cr_assert_eq(ret, 0, "Returned value %d was not 0", ret);
    close(fd);

    ret = system("cmp test_output/pkt_no_payload tests/rsrc/pkt_no_payload");
    cr_assert_eq(ret, 0, "Packet sent did not match expected");
}

Test(protocol_suite, send_with_payload, .init = init, .timeout = 5) {
#ifdef NO_PROTOCOL
    cr_assert_fail("Protocol was not implemented");
#endif

    int fd;
    void *payload = "0123456789012345";
    MZW_PACKET pkt = {0};

    pkt.type = MZW_CHAT_PKT;
    pkt.param1 = 0101;
    pkt.param2 = 0202;
    pkt.param3 = 0303;
    pkt.size = 0xf;
    pkt.timestamp_sec = 0x11223344;
    pkt.timestamp_nsec = 0x55667788;

    fd = open("test_output/pkt_with_payload", O_CREAT|O_TRUNC|O_RDWR, 0644);
    cr_assert(fd > 0, "Failed to create output file");
    int ret = proto_send_packet(fd, &pkt, payload);
    cr_assert_eq(ret, 0, "Returned value was %d not 0", ret);
    close(fd);

    ret = system("cmp test_output/pkt_with_payload tests/rsrc/pkt_with_payload");
    cr_assert_eq(ret, 0, "Packet sent did not match expected");
}

Test(protocol_suite, send_error, .init = init, .timeout = 5) {
#ifdef NO_PROTOCOL
    cr_assert_fail("Protocol was not implemented");
#endif
    int fd;
    void *payload = NULL;
    MZW_PACKET pkt = {0};

    pkt.type = MZW_READY_PKT;
    pkt.param1 = 0101;
    pkt.param2 = 0202;
    pkt.param3 = 0303;
    pkt.size = 0;
    pkt.timestamp_sec = 0x11223344;
    pkt.timestamp_nsec = 0x55667788;

    fd = open("test_output/pkt_error", O_CREAT|O_TRUNC|O_RDWR, 0644);
    cr_assert(fd > 0, "Failed to create output file");
    // Here is the error.
    close(fd);
    int ret = proto_send_packet(fd, &pkt, payload);
    cr_assert_neq(ret, 0, "Returned value was zero", ret);
}

Test(protocol_suite, recv_no_payload, .init = init, .timeout = 5) {
#ifdef NO_PROTOCOL
    cr_assert_fail("Protocol was not implemented");
#endif
    int fd;
    void *payload = NULL;
    MZW_PACKET pkt = {0};

    fd = open("tests/rsrc/pkt_no_payload", O_RDONLY, 0);
    cr_assert(fd > 0, "Failed to open test input file");
    int ret = proto_recv_packet(fd, &pkt, &payload);
    cr_assert_eq(ret, 0, "Returned value was not 0");
    close(fd);

    cr_assert_eq(pkt.type, MZW_READY_PKT, "Received packet type %d did not match expected %d",
		 pkt.type, MZW_READY_PKT);
    cr_assert_eq(pkt.param1, (int8_t)0101, "Received param1 field %d did not match expected %d",
		 pkt.param1, (int8_t)0101);
    cr_assert_eq(pkt.param2, (int8_t)0202, "Received param2 field %d did not match expected %d",
		 pkt.param2, (int8_t)0202);
    cr_assert_eq(pkt.param3, (int8_t)0303, "Received param3 field %d did not match expected %d",
		 pkt.param3, (int8_t)0303);
    cr_assert_eq(pkt.size, 0, "Received payload size was %u not zero", pkt.size);
    cr_assert_eq(pkt.timestamp_sec, 0x11223344,
		 "Received message timestamp_sec 0x%x did not match expected 0x%x",
		 pkt.timestamp_sec, 0x11223344);
    cr_assert_eq(pkt.timestamp_nsec, 0x55667788,
		 "Received message timestamp_nsec 0x%x did not match expected 0x%x",
		 pkt.timestamp_nsec, 0x55667788);
}

Test(protocol_suite, recv_with_payload, .init = init, .timeout = 5) {
#ifdef NO_PROTOCOL
    cr_assert_fail("Protocol was not implemented");
#endif
    int fd;
    void *payload = NULL;
    MZW_PACKET pkt = {0};

    fd = open("tests/rsrc/pkt_with_payload", O_RDONLY, 0);
    cr_assert(fd > 0, "Failed to open test input file");
    int ret = proto_recv_packet(fd, &pkt, &payload);
    cr_assert_eq(ret, 0, "Returned value was not 0");
    close(fd);

    cr_assert_eq(pkt.type, MZW_CHAT_PKT, "Received packet type %d did not match expected %d",
		 pkt.type, MZW_CHAT_PKT);
    cr_assert_eq(pkt.param1, (int8_t)0101, "Received param1 field %d did not match expected %d",
		 pkt.param1, (int8_t)0101);
    cr_assert_eq(pkt.param2, (int8_t)0202, "Received param2 field %d did not match expected %d",
		 pkt.param2, (int8_t)0202);
    cr_assert_eq(pkt.param3, (int8_t)0303, "Received param3 field %d did not match expected %d",
		 pkt.param3, (int8_t)0303);
    cr_assert_eq(pkt.size, 0xf, "Received payload size was %u not %u", pkt.size, 0xf);
    cr_assert_eq(pkt.timestamp_sec, 0x11223344,
		 "Received message timestamp_sec 0x%x did not match expected 0x%x",
		 pkt.timestamp_sec, 0x11223344);
    cr_assert_eq(pkt.timestamp_nsec, 0x55667788,
		 "Received message timestamp_nsec 0x%x did not match expected 0x%x",
		 pkt.timestamp_nsec, 0x55667788);
    int n = strncmp(payload, "0123456789012345", 0xf);
    cr_assert_eq(n, 0, "Received message payload did not match expected");
}

Test(protocol_suite, recv_empty, .init = init, .timeout = 5) {
#ifdef NO_PROTOCOL
    cr_assert_fail("Protocol was not implemented");
#endif
    int fd;
    void *payload = NULL;
    MZW_PACKET pkt = {0};

    fd = open("tests/rsrc/pkt_empty", O_RDONLY, 0);
    cr_assert(fd > 0, "Failed to open test input file");
    int ret = proto_recv_packet(fd, &pkt, &payload);
    cr_assert_neq(ret, 0, "Returned value was 0");
    close(fd);
}

Test(protocol_suite, recv_short_header, .init = init, .timeout = 5) {
#ifdef NO_PROTOCOL
    cr_assert_fail("Protocol was not implemented");
#endif
    int fd;
    void *payload = NULL;
    MZW_PACKET pkt = {0};

    fd = open("tests/rsrc/pkt_short_header", O_RDONLY, 0);
    cr_assert(fd > 0, "Failed to open test input file");
    int ret = proto_recv_packet(fd, &pkt, &payload);
    cr_assert_neq(ret, 0, "Returned value was 0");
    close(fd);
}

Test(protocol_suite, recv_short_payload, .init = init, .signal = SIGALRM) {
#ifdef NO_PROTOCOL
    cr_assert_fail("Protocol was not implemented");
#endif
    int fd;
    void *payload = NULL;
    MZW_PACKET pkt = {0};
    struct itimerval itv = {{0, 0}, {1, 0}};

    fd = open("tests/rsrc/pkt_short_payload", O_RDONLY, 0);
    cr_assert(fd > 0, "Failed to open test input file");
    // On a network connection, reading will block until the specified
    // amount of payload has been received.  So we have to set an alarm
    // to terminate the test.  Because we are reading from a file here,
    // the underlying read() should return 0, indicating EOF, which
    // proto_recv_packet() should detect and set errno != EINTR.
    // In that case, we have to generate the expected signal manually.
    setitimer(ITIMER_REAL, &itv, NULL);
    int ret = proto_recv_packet(fd, &pkt, &payload);
    cr_assert_neq(ret, 0, "Returned value was 0");
    if(errno != EINTR)
	kill(getpid(), SIGALRM);
    close(fd);
}

Test(protocol_suite, recv_error, .init = init, .timeout = 5) {
#ifdef NO_PROTOCOL
    cr_assert_fail("Protocol was not implemented");
#endif
    int fd;
    void *payload = NULL;
    MZW_PACKET pkt = {0};

    fd = open("tests/rsrc/pkt_empty", O_RDONLY, 0);
    cr_assert(fd > 0, "Failed to open test input file");
    close(fd);
    int ret = proto_recv_packet(fd, &pkt, &payload);
    cr_assert_neq(ret, 0, "Returned value was zero");
}
