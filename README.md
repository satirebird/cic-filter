# Configurable CIC-Filter for interpolation and decimation

* C implementation of the CIC filter for decimation and interpolation.
* Configurable data type
* With or without memory allocation
* Single buffer operation
* CUnit tests

## Usage

Copy the `cic_config.template` to you project with the name `cic_config.h`. 

```c
#include cic_filter.h

#define N 4
#define M 1
#define R 5

#define BLOCK_LEN 128

cic_t buffer[BLOCK_LEN * R]

struct cic_filter filter;

if (cic_filter_init(&filter, N, M, R, NULL, true) < 0)
	return -1;

size_t rlen;	

// Calculate the interpolation. The result is located in buffer with rlen elements.
rlen = cic_filter_interpolate(&filter, buffer, BLOCK_LEN * R, BLOCK_LEN);

// Calculate the decimation. The result is located in buffer with rlen elements.
rlen = cic_filter_decimate(&filter, buffer, rlen);
```

## Unit tests

CUnit must be installed on you system. For Debian / Ubuntu `apt install libcunit1` can be used. 

The unit test can be executed with

```
make test
./cic_test
```

## Literature

* [Wikipedia article](https://en.wikipedia.org/wiki/Cascaded_integrator%E2%80%93comb_filter)
* [The original paper from E. Hogenauer](http://read.pudn.com/downloads163/ebook/744947/123.pdf)
