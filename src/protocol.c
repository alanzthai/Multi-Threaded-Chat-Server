#include "protocol.h"
#include <stdlib.h>
#include <unistd.h>
#include "debug.h"

int write_all(int fd, const void *buf, size_t count) {
    size_t bytes_written = 0;
    while (bytes_written < count) {
        ssize_t ret = write(fd, (const char *)buf + bytes_written, count - bytes_written);
        if (ret <= 0) {
            return ret; // Error or EOF
        }
        bytes_written += ret;
    }
    return bytes_written;
}

int read_all(int fd, void *buf, size_t count) {
    size_t bytes_read = 0;
    while (bytes_read < count) {
        ssize_t ret = read(fd, (char *)buf + bytes_read, count - bytes_read);
        if (ret <= 0) {
            return ret; // Error or EOF
        }
        bytes_read += ret;
    }
    return bytes_read;
}

int proto_send_packet(int fd, CHLA_PACKET_HEADER *hdr, void *payload) {
    // // Convert multi-byte fields in the header to network byte order
    // hdr->type = htons(hdr->type);
    // hdr->payload_length = htonl(hdr->payload_length);
    // hdr->msgid = htonl(hdr->msgid);
    // hdr->timestamp_sec = htonl(hdr->timestamp_sec);
    // hdr->timestamp_nsec = htonl(hdr->timestamp_nsec);

    // Write the header to the wire
    ssize_t bytes_written = write_all(fd, hdr, sizeof(CHLA_PACKET_HEADER));
    if (bytes_written != sizeof(CHLA_PACKET_HEADER)) {
        debug("SHORT COUNT ERROR RETURN -1");
        return -1; // Error writing header
    }

    // If payload is present, write it to the wire
    if (payload != NULL && ntohl(hdr->payload_length) > 0) {
        ssize_t payload_bytes_written = write_all(fd, payload, ntohl(hdr->payload_length));
        // Check short count
        if (payload_bytes_written != ntohl(hdr->payload_length)) {
            debug("SHORT COUNT ERROR RETURN -1");
            return -1; // Error writing payload
        }
    }

    debug("EXIT SEND");
    return 0;

    // --------------------------------------------------

    // // Convert multi-byte fields in the header to network byte order
    // hdr->type = htons(hdr->type);
    // hdr->payload_length = htonl(hdr->payload_length);
    // hdr->msgid = htonl(hdr->msgid);
    // hdr->timestamp_sec = htonl(hdr->timestamp_sec);
    // hdr->timestamp_nsec = htonl(hdr->timestamp_nsec);

    // // Write the header to the wire
    // ssize_t bytes_written = write(fd, hdr, sizeof(CHLA_PACKET_HEADER));
    // // Check short count
    // if (bytes_written < 0 || (size_t)bytes_written < sizeof(CHLA_PACKET_HEADER)) {
    //     debug("SHORT COUNT ERROR RETURN -1");
    //     return -1;
    // }

    // // If payload is present, write it to the wire
    // if (payload != NULL && ntohl(hdr->payload_length) > 0) {
    //     ssize_t payload_bytes_written = write(fd, payload, ntohl(hdr->payload_length));
    //     // Check short count
    //     if (payload_bytes_written < 0 || (size_t)payload_bytes_written < ntohl(hdr->payload_length)) {
    //         debug("SHORT COUNT ERROR RETURN -1");
    //         return -1;
    //     }
    // }

    // debug("EXIT SEND");
    // return 0;
}

int proto_recv_packet(int fd, CHLA_PACKET_HEADER *hdr, void **payload) {
    // debug("--------------------------------PARAMS: %lu", sizeof(CHLA_PACKET_HEADER));
    // Read the header from the wire
    ssize_t header_bytes_read = read_all(fd, hdr, sizeof(CHLA_PACKET_HEADER));
    // Check short count
    if (header_bytes_read != sizeof(CHLA_PACKET_HEADER)) {
        return -1;
    }

	// Convert multi-byte fields in the header to host byte order
	// hdr->type = ntohs(hdr->type);
	hdr->payload_length = ntohl(hdr->payload_length);
    // debug("----------------------------------PAYLOAD LENGTH: %d", hdr->payload_length);
	// hdr->msgid = ntohl(hdr->msgid);
	// hdr->timestamp_sec = ntohl(hdr->timestamp_sec);
	// hdr->timestamp_nsec = ntohl(hdr->timestamp_nsec);

    // Allocate memory for payload if necessary
    if (hdr->payload_length > 0) {
        *payload = malloc(hdr->payload_length);
        if (*payload == NULL) {
            return -1;
        }
        // Read the payload from the wire
        ssize_t payload_bytes_read = read_all(fd, *payload, hdr->payload_length);
        // Check short count
        if (payload_bytes_read != hdr->payload_length) {
            debug("SHORT COUNT ERROR RETURN -1");
            return -1;
        }
    }
    else {
        *payload = NULL;
    }

    // Convert back to network byte order
    hdr->payload_length = htonl(hdr->payload_length);

    return 0;
}