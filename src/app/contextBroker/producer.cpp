#include <librdkafka/rdkafka.h>
#include <stdio.h>
#include <cstring>
#include <string>  // Para std::string

int main() {
    // 1. Crear configuración básica
    rd_kafka_conf_t *config = rd_kafka_conf_new();

    // 2. Configurar brokers (usando std::string para mejor manejo)
    std::string brokers_str = "localhost:9092";
    rd_kafka_conf_set(config, "bootstrap.servers", brokers_str.c_str(), NULL, 0);

    // 3. Crear el productor
    char errstr[512];
    rd_kafka_t *producer = rd_kafka_new(RD_KAFKA_PRODUCER, config, errstr, sizeof(errstr));

    if(!producer) {
        fprintf(stderr, "Error creando productor: %s\n", errstr);
        return 1;
    }

    // 4. Preparar el mensaje (usando buffer no constante)
    std::string topic_str = "mi_topic";
    std::string message_str = "Hola Mundo desde C!";

    // Crear copia modificable del mensaje
    char* message = new char[message_str.size() + 1];
    std::strcpy(message, message_str.c_str());

    // 5. Enviar el mensaje
    rd_kafka_produce(
        rd_kafka_topic_new(producer, topic_str.c_str(), NULL), // Topic
        RD_KAFKA_PARTITION_UA, // Partición automática
        RD_KAFKA_MSG_F_COPY,   // Copiar el payload
        message,               // Mensaje (no constante)
        message_str.size(),    // Tamaño
        NULL, 0,               // Key/valor opcional
        NULL                   // Opaque pointer
    );

    // 6. Esperar que se envíe
    rd_kafka_flush(producer, 10*1000); // Espera 10 segundos

    // 7. Limpiar
    delete[] message; // Liberar memoria del mensaje
    rd_kafka_destroy(producer);

    printf("Mensaje enviado con éxito!\n");
    return 0;
}