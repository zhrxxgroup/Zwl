#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_KEY 256
#define MAX_VALUE 1024

typedef struct {
    char key[MAX_KEY];
    char value[MAX_VALUE];
} ZwlEntry;

typedef struct {
    char target[MAX_VALUE];
    char header[MAX_VALUE];
    ZwlEntry *payload;
    size_t payload_count;
} ZwlMessage;

typedef enum {
    HANDSHAKE = 1,
    CLOSE,
    INVALID_HEADER = -1
} ZwlHeader;


ZwlHeader parse_zwl_header(ZwlMessage *message) {
    if (!message || !message->header) return -1;

    if (strcmp(message->header, "CLOSE") == 0) {
        return CLOSE;
    } else if (strcmp(message->header, "HANDSHAKE") == 0) {
        return HANDSHAKE;
    }
    return -1;
}

char *trim_whitespace(char *str) {
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return str;
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    *(end + 1) = '\0';
    return str;
}

int zwl_parse_entry(const char *line, ZwlEntry *entry) {
    const char *delimiter = strchr(line, ':');
    if (!delimiter) {
        strncpy(entry->key, ".", MAX_KEY - 1);
        strncpy(entry->value, line, MAX_VALUE - 1);
    } else {
        size_t key_len = delimiter - line;
        size_t value_len = strlen(delimiter + 1);

        if (key_len >= MAX_KEY || value_len >= MAX_VALUE) return 0;

        strncpy(entry->key, line, key_len);
        entry->key[key_len] = '\0';
        strncpy(entry->value, delimiter + 1, MAX_VALUE - 1);
        entry->value[MAX_VALUE - 1] = '\0';
    }

    strncpy(entry->key, trim_whitespace(entry->key), MAX_KEY - 1);
    entry->key[MAX_KEY - 1] = '\0';
    strncpy(entry->value, trim_whitespace(entry->value), MAX_VALUE - 1);
    entry->value[MAX_VALUE - 1] = '\0';
    return 1;
}

ZwlMessage create_zwl_message(const char *target, const char *header, ZwlEntry *payload, size_t payload_count) {
    ZwlMessage message = {0};

    
    strncpy(message.target, target, MAX_VALUE - 1);
    message.target[MAX_VALUE - 1] = '\0'; 

    strncpy(message.header, header, MAX_VALUE - 1);
    message.header[MAX_VALUE - 1] = '\0'; 

    
    message.payload_count = payload_count;
    if (payload_count > 0) {
        message.payload = malloc(sizeof(ZwlEntry) * payload_count);
        if (!message.payload) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        memset(message.payload, 0, sizeof(ZwlEntry) * payload_count);

        for (size_t i = 0; i < payload_count; i++) {
            message.payload[i] = payload[i]; 
        }
    }

    return message;
}

char *ZwlMessage_to_String(ZwlMessage *message) {
    if (!message) return NULL;

    size_t total_size = strlen(message->target) + strlen(message->header) + 50; // For "TARGET:", "HEADER:", "\r\n"
    for (size_t i = 0; i < message->payload_count; i++) {
        total_size += strlen(message->payload[i].key) + strlen(message->payload[i].value) + 4; // For "key:value\r\n"
    }

    char *result = malloc(total_size + 1);
    if (!result) return NULL;

    snprintf(result, total_size, "TARGET: %s\r\nHEADER: %s\r\nPAYLOAD:\r\n", message->target, message->header);

    for (size_t i = 0; i < message->payload_count; i++) {
        strcat(result, message->payload[i].key);
        strcat(result, ": ");
        strcat(result, message->payload[i].value);
        strcat(result, "\r\n");
    }

    return result;
}


ZwlMessage parse_zwl_message_body(const char *message) {
    ZwlMessage zwl = {0};
    zwl.payload = malloc(sizeof(ZwlEntry) * 32);
    if (!zwl.payload) exit(EXIT_FAILURE);

    size_t payload_capacity = 32;

    char *msg_copy = strdup(message);
    if (!msg_copy) exit(EXIT_FAILURE);

    char *line = strtok(msg_copy, "\r\n");
    int parsing_payload = 0;

    while (line) {
        line = trim_whitespace(line);

        if (strncmp(line, "TARGET:", 7) == 0) {
            strncpy(zwl.target, trim_whitespace(line + 7), MAX_VALUE - 1);
            zwl.target[MAX_VALUE - 1] = '\0';
        } else if (strncmp(line, "HEADER:", 7) == 0) {
            strncpy(zwl.header, trim_whitespace(line + 7), MAX_VALUE - 1);
            zwl.header[MAX_VALUE - 1] = '\0';
        } else if (strncmp(line, "PAYLOAD:", 8) == 0) {
            parsing_payload = 1;
        } else if (parsing_payload && line[0] != '\0') {
            ZwlEntry entry = {0};
            if (zwl_parse_entry(line, &entry)) {
                if (zwl.payload_count >= payload_capacity) {
                    payload_capacity *= 2;
                    zwl.payload = realloc(zwl.payload, sizeof(ZwlEntry) * payload_capacity);
                    if (!zwl.payload) exit(EXIT_FAILURE);
                }
                zwl.payload[zwl.payload_count++] = entry;
            }
        }

        line = strtok(NULL, "\r\n");
    }

    free(msg_copy);
    return zwl;
}

void free_zwl_message(ZwlMessage *message) {
    if (message && message->payload) {
        free(message->payload);
        message->payload = NULL;
    }
}
