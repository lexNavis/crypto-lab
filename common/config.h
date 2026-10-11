#pragma once
#include <stdbool.h>
#include <stddef.h>

/**
 * @struct Структура для описания операции над bignum_t
 */
typedef struct {
    char op[16];       /* "add", "sub", "mul", "divmod", "pow", "pow_mod", "gcd", "lcm", "cmp" */
    char a[256];       /* hex-строка операнда 1 */
    char b[256];       /* hex-строка операнда 2 */
    char mod[256];     /* hex-строка модуля (только для pow_mod) */
    bool neg_a;
    bool neg_b;
} bignum_operation_t;

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
    struct {
        bignum_operation_t *operations;
        size_t count;
    } bignum;
} config_t;
/**
 * @enum Список ошибок
 */
enum CONFIG_ERRORS {
    ERR_CONFIG_OPEN_FILE = 1,
    ERR_CONFIG_MALLOC_FAILED,
    ERR_CONFIG_PARSE_JSON,
    ERR_CONFIG_READ_FILE
};
/**
 * @brief Загружает конфиг в структуру config_t
 * @param path Путь к файлу
 * @param config Указатель на структуру config_t
 * @returns 0 в случае успеха, отрицательное значение в случае неудачи
 */
int load_config(const char *path, config_t *config);

/**
 * @brief Освобождает динамическую память внутри config_t
 * 
 * @param cfg Конфигурация
 */
void config_free(config_t *cfg);
