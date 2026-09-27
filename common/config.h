#pragma once
#include <stdbool.h>

/**
 * @struct Структура для описания конфигурации
 */
typedef struct {
    char run[16];
    struct {
        unsigned int bits;
        bool from_file;
        char message[256];
    } rsa;
    struct {
        bool use_random_key;
        char block[64];  // hex-строка
    } kuznyechik;
    struct {
        char message[512];  // hex-строка
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
 * @param [path] - путь к файлу
 * @param [config] - указатель на структуру config_t
 * @returns 0 в случае успеха, отрицательное значение в случае неудачи
 */
int load_config(const char *path, config_t *config);
