/**
 * CIC Filter implementation.
 *
 * Some words about "Register growth"
 *
 * Depending an the filter parameters the CIC filter has a unity gain. For
 * decimation filters the gain is Gmax = (RM)^N. For interpolation filters
 * the gain on the last stage is Gmax = (RM)^N / R.
 * This means that the input data must be smaller than the maximum cic_t
 * value divided by Gmax. If the following values are given for a decimation
 * filter:
 *
 * R = 5
 * N = 4
 * M = 1
 * cic_t = int32_t
 *
 * Gmax = 625
 * The maximum input value is 2^31 / 625 which is around 3.4e6.
 *
 * For larger rate change factors Gmax can be huge. For interpolation filters
 * the only way is to switch to a larger data type int64_t for example. For
 * decimation filters the integrator stage may be modified in the future.
 * Regarding to the original paper it should be possible to decimate the data
 * within each stage of the integrator.
 *
 * References:
 * [Hog81]  Eugene B. Hogenauer, An Economical Class of Digital Filters
 *          for Decimation and Interpolation, IEEE Transactions on Acoustics,
 *          Speech and Signal Processing, ASSP-29(2):155–162, 1981.
 *
 */

#ifndef CIC_FILTER_H
#define CIC_FILTER_H

#include "cic_config.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

struct cic_filter {
    size_t N;       //Stage length
    size_t M;       //Differential delay (either 1 or 2)
    size_t mpos;    //Differential delay position

    size_t R;       //Rate change factor
    size_t cnt;     //Rate change counter

    cic_t *delay;   //Delay buffer
    bool to_free;   //True if malloc was used

    bool normalize; //Normalize the output
    cic_t ddiv;     //Decimation gain
    cic_t idiv;     //Interpolation gain
};


/**
 * Initialize the CIC filter structure.
 *
 * Be sure the buffer is at least N + (N * M) elements long.
 *
 * @param p Pointer to the cic_filter struct
 * @param N Stage length.
 * @param M Differential delay (either 1 or 2)
 * @param R Rate change factor
 * @param delay Buffer for the delay line. Be sure the buffer is at
 *              least N + (N * M) elements long. If the pointer is NULL
 *              the cic_malloc function is used to allocate the delay buffer.
 * @param normalize Normalize the output. Note that this requires a division.
 * @return 0 on success, -1 on any error
 */
int cic_filter_init(struct cic_filter *p, size_t N, size_t M, size_t R,
        cic_t *delay, bool normalize);

/**
 * Release the allocated buffers.
 *
 * @param p Pointer to the cic_filter struct
 */
void cic_filter_free(struct cic_filter *p);

/**
 * Decimate the given buffer.
 *
 * @param p Pointer to the cic_filter struct.
 * @param buf Input and output buffer
 * @param len Input length in samples
 * @return Number of valid samples in buf.
 */
size_t cic_filter_decimate(struct cic_filter *p, cic_t *buf, size_t len);

/**
 * Interpolate the given buffer.
 *
 * Be sure the sample buffer can hold R * len samples. Otherwise zero is
 * returned and nothing happens.
 *
 * @param p Pointer to the cic_filter struct.
 * @param buf Input and output buffer.
 * @param buf_len The length of the buffer.
 * @param len Number of valid input samples in buf.
 * @return The number of valid output samples in buf.
 */
size_t cic_filter_interpolate(struct cic_filter *p, cic_t *buf, size_t buf_len,
        size_t len);

#endif
