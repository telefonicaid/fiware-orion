//
// Created by oriana on 7/3/25.
//
#include <librdkafka/rdkafka.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static volatile sig_atomic_t run = 1;

static void stop(int sig) {
    run = 0;
    fclose(stdin);
}

int main(int argc, char **argv) {
    rd_kafka_t *rk;
    rd_kafka_conf_t *conf;
    char errstr[512];
    const char *brokers = "localhost:9092";
    const char *topic_str = "hola";

    /* Create configuration object */
    conf = rd_kafka_conf_new();

    /* Set bootstrap brokers */
    if (rd_kafka_conf_set(conf, "bootstrap.servers", brokers,
                          errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
        fprintf(stderr, "%s\n", errstr);
        return 1;
    }

    /* Create producer instance */
    rk = rd_kafka_new(RD_KAFKA_PRODUCER, conf, errstr, sizeof(errstr));
    if (!rk) {
        fprintf(stderr, "Failed to create producer: %s\n", errstr);
        return 1;
    }

    /* Signal handler for clean shutdown */
    signal(SIGINT, stop);

    printf("Type messages to produce to topic %s (Ctrl-C to exit)\n", topic_str);

    while (run) {
        char buf[256];
        char key[32];
        rd_kafka_resp_err_t err;
        time_t now;
        char timestamp[20];

        if (!fgets(buf, sizeof(buf), stdin))
            break;

        size_t len = strlen(buf);
        if (len > 0 && buf[len-1] == '\n')
            buf[--len] = '\0';

        if (len == 0)
            continue;

        /* Create a message key */
        snprintf(key, sizeof(key), "key-%03d", ::rand() % 100);
        /* Create timestamp for header */
        time(&now);
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));

        /* Produce message with key and headers */
        err = rd_kafka_producev(
            rk,
            RD_KAFKA_V_TOPIC(topic_str),
            RD_KAFKA_V_KEY(key, strlen(key)),
            RD_KAFKA_V_VALUE(buf, len),
            /* Add custom headers */
            RD_KAFKA_V_HEADER("message-id", "12345", 5),  // Binary header (value, size)
            RD_KAFKA_V_HEADER("timestamp", timestamp, strlen(timestamp)),
            RD_KAFKA_V_HEADER("content-type", "text/plain", -1),  // -1 = auto strlen
            RD_KAFKA_V_HEADER("custom-header", "header-value", -1),
            RD_KAFKA_V_END);

        if (err) {
            fprintf(stderr, "Failed to produce message: %s\n",
                    rd_kafka_err2str(err));
            continue;
        }

        printf("Produced message (key: %s, timestamp: %s): %s\n",
               key, timestamp, buf);

        rd_kafka_poll(rk, 0);
    }

    printf("Flushing final messages...\n");
    rd_kafka_flush(rk, 10 * 1000);

    rd_kafka_destroy(rk);

    return 0;
}