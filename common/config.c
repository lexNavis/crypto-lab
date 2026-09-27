#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <cjson/cJSON.h>

int load_config(const char *path, config_t *config)
{
    FILE *f = fopen(path, "r");
    if (!f) {
        printf("Error with opening file!\n");
        return -ERR_OPEN_FILE;
    }
    fseek(f, 0, SEEK_END);
    /* Длина текста в файле */
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* buf = malloc(len + 1);
    if (!buf) {
        fclose(f);
        return -ERR_MALLOC_FAILED;
    }
    size_t n = fread(buf, 1, len, f);
    if (n != (size_t)len) {
        free(buf);
        fclose(f);
        return -ERR_READ_FILE;
    }
    buf[len] = '\0';
    fclose(f);

    /* Разбор конфига */
    cJSON *root = cJSON_Parse(buf);
    free(buf);
    if (!root) {
        printf("Couldn't parse json file!\n");
        return -ERR_PARSE_JSON;
    }
    cJSON *run_arg = cJSON_GetObjectItem(root, "run");
    if (cJSON_IsString(run_arg)) {
        snprintf(config->run, sizeof(config->run), "%s", run_arg->valuestring);
    }
    /* RSA параметры */
    cJSON *rsa = cJSON_GetObjectItem(root, "rsa");
    if (rsa) {
        cJSON *bits = cJSON_GetObjectItem(rsa, "bits");
        if (cJSON_IsNumber(bits)) {
            config->rsa.bits = bits->valueint;
        }
        cJSON *from_file = cJSON_GetObjectItem(rsa, "from_file");
        if (cJSON_IsBool(from_file)) {
            config->rsa.from_file = cJSON_IsTrue(from_file);
        }
        cJSON *rsa_message = cJSON_GetObjectItem(rsa, "message");
        if (cJSON_IsString(rsa_message)) {
            snprintf(config->rsa.message, sizeof(config->rsa.message), "%s", rsa_message->valuestring);
        }
    }
    /* Параметры Кузнечика */
    cJSON *kuz = cJSON_GetObjectItem(root, "kuznyechik");
    if (kuz) {
        cJSON *use_random_key = cJSON_GetObjectItem(kuz, "use_random_key");
        if (cJSON_IsBool(use_random_key)) {
            config->kuznyechik.use_random_key = cJSON_IsTrue(use_random_key);
        }
        cJSON *kuz_block = cJSON_GetObjectItem(kuz, "block");
        if (cJSON_IsString(kuz_block)) {
            snprintf(config->kuznyechik.block, sizeof(config->kuznyechik.block), "%s", kuz_block->valuestring);
        }
    }
    /* Параметры Стрибога */
    cJSON *streebog = cJSON_GetObjectItem(root, "streebog");
    if (streebog) {
        cJSON *streebog_message = cJSON_GetObjectItem(streebog, "message");
        if (cJSON_IsString(streebog_message)) {
            snprintf(config->streebog.message, sizeof(config->streebog.message), "%s", streebog_message->valuestring);
        }
    }
    cJSON_Delete(root);
    return 0;
}
