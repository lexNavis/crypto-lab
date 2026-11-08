
#include "../bignum/bignum.h"

int form_mersen_number(bignum_t *val, uint32_t exp);

int mersen_check_standard(const bignum_t *val, bool *is_prime);

int mersen_check_eratosthenes(const bignum_t *val, bool *is_prime);

int mersen_check_atkin(const bignum_t *val, bool *is_prime);

int mersen_check_lucas_lehmer(const bignum_t *val, uint32_t exp, bool *is_prime);

