#ifndef PUBSUB_H
#define PUBSUB_H

#include "FreeRTOS.h"
#include "queue.h"

#define MAX_TOPIC_LENGTH      32
#define MAX_PAYLOAD_LENGTH    64
#define MAX_TOPICS            8
#define MAX_SUBSCRIBERS       5
#define SUBSCRIBER_QUEUE_LEN  8

typedef enum {
    PS_OK             =  0,
    PS_ERR_FULL       = -1,
    PS_ERR_NOT_FOUND  = -2,
    PS_ERR_INVALID    = -3,
    PS_ERR_DUPLICATE  = -4,
} ps_status_t;

typedef struct {
    char topic[MAX_TOPIC_LENGTH];
    char payload[MAX_PAYLOAD_LENGTH];
} Message;

typedef struct {
    QueueHandle_t queue;
    char          name[32];
} Subscriber;

typedef struct {
    char       topic_name[MAX_TOPIC_LENGTH];
    Subscriber *subscribers[MAX_SUBSCRIBERS];
    int        subscriber_count;
    uint32_t   publish_count;
    uint32_t   drop_count;
} Topic;

void        pubsub_init(void);
ps_status_t subscribe(const char *topic, Subscriber *subscriber);
ps_status_t publish(const char *topic, const char *payload);
void        pubsub_print_stats(void);

/* Topic 名稱 */
#define TOPIC_TEMPERATURE  "temperature"
#define TOPIC_HUMIDITY     "humidity"
#define TOPIC_TILT         "tilt"
#define TOPIC_ALARM        "alarm"

#endif
