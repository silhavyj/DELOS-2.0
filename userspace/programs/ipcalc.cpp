#include <system.h>
#include <string.h>
#include <memory.h>

void print_ip_bin(uint32_t ip, const char *format) {
    char buff[64];
    int p = 0;
    for (int i = 0; i < 32; i++) {
        if (i % 8 == 0 && i != 0) {
            buff[p++] = '.';
        }
        buff[p] = ((ip >> i) & 1) ? '1' : '0';
        p++;
    }
    buff[p] = '\0';
    reverse(buff);
    printf(format, buff);
}

void print_ip(uint32_t ip, const char *format) {
    uint8_t octet1 = (ip & 0xFF);
    uint8_t octet2 = (ip & 0xFF00) >> 8;
    uint8_t octet3 = (ip & 0xFF0000) >> 16;
    uint8_t octet4 = (ip & 0xFF000000) >> 24;
    printf(format, octet4, octet3, octet2, octet1);
}

int main() {
    const char *ENTER_IP = "enter an ip: ";
    const char *INVALID_OCTET_SIZE = "Invalid octet size!\n\r";
    const char *INVALID_IP_FORMAT = "Invalid format of the ip address!\n\r";
    const char *INVALID_MASK_SIZE = "Invalid ip mask size!\n\r";

    int mask = 24;
    char buffer[32];
    char octet_buff[32];

    printf(ENTER_IP);
    read_line(buffer);

    int len = strlen(buffer);
    int last_dot_pos = 0;
    uint16_t octet;
    int docts = 0;

    for (int i = 0; i < len; i++) {
        if (buffer[i] == '/') {
            mask = atoi(&buffer[i+1]);
            if (mask == 0)
                mask = 24;
            if (mask < 0 || mask > 32) {
                printf(INVALID_MASK_SIZE);
                return 3;
            }
            buffer[i] = '\0';
            len = strlen(buffer);
            break;
        }
    }

    uint32_t mask_val = 0;
    for (int i = 0; i < mask; i++)
        mask_val |= (1 << (32 - i - 1));
    uint32_t wildcard = ~mask_val;

    uint32_t ip = 0;
    for (int i = 0; i < len; i++) {
        if (buffer[i] == '.') {
            docts++;
            memcpy(octet_buff, &buffer[last_dot_pos], i - last_dot_pos);
            octet_buff[i - last_dot_pos] = '\0';
            octet = (uint16_t)atoi(octet_buff);
            if (octet > 255) {
                printf(INVALID_OCTET_SIZE);
                return 1;
            }
            // printf(OCTET_BUFF, octet_buff);
            // printf(DEC, octet);
            last_dot_pos = i + 1;
            ip |= (octet << (32 - (docts * 8)));
        }
    }
    if (docts != 3) {
        printf(INVALID_IP_FORMAT);
        return 2;
    }
    memcpy(octet_buff, &buffer[last_dot_pos], len - last_dot_pos);
    octet_buff[len - last_dot_pos] = '\0';
    octet = (uint16_t)atoi(octet_buff);
    if (octet > 255) {
        printf(INVALID_OCTET_SIZE);
        return 1;
    }
    // printf(OCTET_BUFF, octet_buff);
    // printf(DEC, octet);
    docts++;
    ip |= (octet << (32 - (docts * 8)));

    const char *IP_OCTETS_FORMAT = "%d.%d.%d.%d\n\r";

    const char *IP_FORMAT = "Address: %s\n\r";
    print_ip_bin(ip, IP_FORMAT);
    print_ip(ip, IP_OCTETS_FORMAT);

    const char *WILDCARD_FORMAT = "Wildcard: %s\n\r";
    print_ip_bin(wildcard, WILDCARD_FORMAT);
    print_ip(wildcard, IP_OCTETS_FORMAT);

    const char *MASK_STR = "Netmask: %s\n\r";
    print_ip_bin(mask_val, MASK_STR);
    print_ip(mask_val, IP_OCTETS_FORMAT);

    const char *SEPARATOR = ">=\n\r";
    printf(SEPARATOR);

    uint32_t network = ip & mask_val;
    const char *NETWORK_STR = "Network: %s\n\r";
    print_ip_bin(network, NETWORK_STR);
    print_ip(network, IP_OCTETS_FORMAT);

    uint32_t host_min = network + 1;
    const char *HOST_MIN_STR = "HostMin: %s\n\r";
    print_ip_bin(host_min, HOST_MIN_STR);
    print_ip(host_min, IP_OCTETS_FORMAT);

    uint32_t broadcast = network | wildcard;

    uint32_t host_max = broadcast - 1;
    const char *HOST_MAX_STR = "HostMax %s\n\r";
    print_ip_bin(host_max, HOST_MAX_STR);
    print_ip(host_max, IP_OCTETS_FORMAT);

    const char *BROADCAST_STR = "Broadcast: %s\n\r";
    print_ip_bin(broadcast, BROADCAST_STR);
    print_ip(broadcast, IP_OCTETS_FORMAT);

    uint32_t hosts = host_max - host_min + 1;
    const char *HOSTS_STR = "Hosts/Net: %d\n\r";
    printf(HOSTS_STR, hosts);

    return 0;
}