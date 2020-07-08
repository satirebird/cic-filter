/*
 * CIC implementation in C.
 */

#include "cic_filter.h"

#include <errno.h>
#include <strings.h>

/**
 * Integrate the given buffer.
 *
 * @param p Pointer to the cic_filter struct
 * @param buf Input / Output buffer
 * @param len Number of samples in buffer
 */
static void cic_integrate(struct cic_filter *p, cic_t *buf, size_t len) {

    cic_t sum = 0;
    cic_t *delay = p->delay;

    size_t i, s;
    for (i = 0; i < len; i++) {
        sum = buf[i];
        for (s = 0; s < p->N; s++) {
            sum += delay[s];
            delay[s] = sum;
        }
        buf[i] = sum;
    }
}

/**
 * Comb filter execution.
 *
 * @param p Pointer to the cic_filter struct
 * @param buf Input / Output buffer
 * @param len Number of samples in buffer
 */
static void cic_comb(struct cic_filter *p, cic_t *buf, size_t len) {

    cic_t sum = 0;
    const size_t m = p->M;
    const size_t mx = m - 1; // We use mx as xor value to calculate the other position, M is either 1 or 2
    const size_t slen = p->N * m;
    size_t mpos = p->mpos;
    cic_t *delay = &p->delay[p->N];

    size_t i, s;
    for (i = 0; i < len; i++) {
        // Get the position of the last differential delay
        sum = buf[i];
        for (s = 0; s < slen; s += m) {
            cic_t dv = delay[s + mpos];
            delay[s + mpos] = sum;
            sum -= dv;
        }
        buf[i] = sum;
        mpos ^= mx; // Switch the position if m == 2, otherwise mx is 0
    }
    p->mpos = mpos;
}

/**
 * Perform a sample rate conversion.
 *
 * @param p Pointer to the cic_filter struct
 * @param buf Input / Output buffer
 * @param len Number of valid input samples in buffer
 * @return Number of valid output samples in the buffer
 */
static size_t cic_rate_down(struct cic_filter *p, cic_t *buf, size_t len) {

    const size_t cnt = p->cnt;
    const size_t r = p->R;

    size_t i;
    size_t j = 0;
    for (i = cnt; i < len; i += r) {
        buf[j++] = buf[i];
    }

    p->cnt = i - len;

    return j;
}

/**
 * Perform a sample rate conversion.
 *
 * Be sure that the buffer can hold up to len * R samples.
 *
 * @param p Pointer to the cic_filter struct
 * @param buf Input / Output buffer
 * @param buf_len The length of the buffer.
 * @param len Number of valid input samples in buffer
 * @return Number of valid output samples in the buffer
 */
static size_t cic_rate_up(struct cic_filter *p, cic_t *buf, size_t buf_len, size_t len) {

    const size_t r = p->R;
    const size_t rlen = len * r;

    if (buf_len < rlen)
        return 0;

    size_t i;
    size_t j = rlen - r;
    for (i = len; i > 0; i--) {
        buf[j] = buf[i - 1];
        size_t ins;
        for (ins = 1; ins < r; ins++)
            buf[j + ins] = 0;
        j -= r;
    }

    return rlen;
}

/**
 * Integer pow() implementation.
 *
 * @param base The base
 * @param exp The exponent
 * @return The result
 */
static cic_t cic_pow(size_t base, size_t exp)
{
    cic_t result = 1;
    for (;;){
        if (exp & 1)
            result *= base;
        exp >>= 1;
        if (!exp)
            break;
        base *= base;
    }

    return result;
}

/**
 * Normalize the samples in the given buffer by a given divisor
 *
 * @param buf Input / Output buffer
 * @param len Number of valid input samples in buffer
 * @param div The didisor
 */
static void cic_normalize(cic_t *buf, size_t len, cic_t div){
    size_t i;
    for (i = 0; i < len; i++)
        buf[i] /= div;
}

int cic_filter_init(struct cic_filter *p, size_t N, size_t M, size_t R,
        cic_t *delay, bool normalize) {

    const size_t delay_len = N + (N * M);

    bzero(p, sizeof(struct cic_filter));

#if CIC_MALLOC
    if (delay == NULL) {
        delay = cic_malloc(delay_len * sizeof(cic_t));
        p->to_free = true;
    }
#endif

    if (delay == NULL) {
        errno = ENOMEM;
        return -1;
    }

    bzero(delay, delay_len * sizeof(cic_t));

    p->N = N;
    p->M = M;
    p->R = R;
    p->delay = delay;
    p->normalize = normalize;

    if (normalize){
        p->ddiv = cic_pow((p->R * p->M), p->N);
        p->idiv = cic_pow((p->R * p->M), p->N) / p->R;
    }

    return 0;
}

void cic_filter_free(struct cic_filter *p) {
#if CIC_MALLOC
    if (p->to_free) {
        cic_free(p->delay);
    }
#endif
}

size_t cic_filter_decimate(struct cic_filter *p, cic_t *buf, size_t len) {
    size_t rlen;

    cic_integrate(p, buf, len);
    rlen = cic_rate_down(p, buf, len);
    cic_comb(p, buf, rlen);

    if (p->normalize)
        cic_normalize(buf, rlen, p->ddiv);

    return rlen;
}

size_t cic_filter_interpolate(struct cic_filter *p, cic_t *buf, size_t buf_len,
        size_t len) {

    //Check for space requirements
    if ((len * p->R) > buf_len)
        return 0;

    size_t rlen;
    cic_comb(p, buf, len);
    rlen = cic_rate_up(p, buf, buf_len, len);
    cic_integrate(p, buf, rlen);

    if (p->normalize)
        cic_normalize(buf, rlen, p->idiv);

    return rlen;
}

