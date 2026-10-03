#pragma once
#include <stdbool.h>

/**
 * @struct Структура для описания конфигурации
 */
typedef struct {
    char run[16];           /* Режим работы */
    struct {
        unsigned int bits;  /* Размер rsa - ключа */
        bool from_file;     /* Использовать ли значения из файла */        
        char message[256];  /* Сообщение для зашифровки/расшифровки */
    } rsa;
    struct {
        bool use_random_key; /* Использовать ли случаный ключ */
        char block[64];      /* Сообщение для зашифровки/расшифровки */
    } kuznyechik;
    struct {
        char message[512];   /* Сообщение для хэширования */
    } streebog;
} config_t;
/**
 * @enum Список ошибок
 */
enum CONFIG_ERRORS {
    ERR_OPEN_FILE = 1,
    ERR_MALLOC_FAILED,
    ERR_PARSE_JSON,
    ERR_READ_FILE
};
/**
 * @brief Загружает конфиг в структуру config_t
 * @param path Путь к файлу
 * @param config Указатель на структуру config_t
 * @returns 0 в случае успеха, отрицательное значение в случае неудачи
 */
int load_config(const char *path, config_t *config);
