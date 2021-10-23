#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int main(int argc, const char** argv) {

	if (argc <= 1) {
		printf("Requires file name\n");
		return -1;
	}
	
	unsigned int offset = 7;
	if (argc > 2) {
		offset = atoi(argv[2]);
	}

	printf("Decode file \"%s\"\n", argv[1]);

	FILE *fid;
	fid = fopen(argv[1], "rb");
	if (!fid) {
		printf("Cannot open file");
		return -2;
	}

	const unsigned int buf_len = 16;
	unsigned char buf[buf_len];
	size_t result;
	int mov_cnt0 = 0;
	int mov_cnt1 = 0;

	unsigned int noise = 1;
	unsigned int buf_res[256];
	size_t buf_res_ind = 0;
	do {
		result = fread(buf, sizeof(unsigned char), buf_len, fid);

		unsigned int cnt = 0;
		for (unsigned int i = 0; i < result; ++i) {
			cnt += buf[i];
			//printf("%i ", (int)buf[i]);
		}
		if (cnt > buf_len - noise) {
			if (mov_cnt0) {
				unsigned int count = round(mov_cnt0 / (float)16);
				if (count == 1) {
					buf_res[buf_res_ind++] = 0;
				} else if (count == 2) {
					buf_res[buf_res_ind++] = 0;
					buf_res[buf_res_ind++] = 0;
				}
				//printf("0: %i - %i\n", mov_cnt0, count);
				mov_cnt0 = 0;
			}
			mov_cnt1++;
			//printf("1 ");
		} else if (cnt < noise) {
			if (mov_cnt1) {
				unsigned int count = round(mov_cnt1 / (float)16);
				if (count == 1) {
					buf_res[buf_res_ind++] = 1;
				} else if (count == 2) {
					buf_res[buf_res_ind++] = 1;
					buf_res[buf_res_ind++] = 1;
				}
				//printf("1: %i - %i\n", mov_cnt1, count);
				mov_cnt1 = 0;
			}
			mov_cnt0++;
			//printf("0 ");
		} else {
			//
		}

	} while (result == buf_len);
	
	fclose(fid);

	for (size_t i = 0; i < buf_res_ind; ++i) {
		printf("%i", buf_res[i]);
	}

	printf("\n");
	printf("\n");

	int ind = 0;

	// Manchester encoding, translate 10 to 0, and 01 to 1.
	// In other words, just display one of those bits (in this case the second one).
	for (size_t i = offset; i < buf_res_ind; i+=2) {
		printf("%i", buf_res[i]);
		ind = (ind + 1) % 4;
		if (!ind) {
			printf(" ");
		}
	}
	
	printf("\n");
	printf("\n");

	ind = 0;
	unsigned int value = 0;
	// pick 4-bit sequences and calculate value
	for (size_t i = offset; i < buf_res_ind; i+=2) {
		if (buf_res[i]) {
			value += pow(2,3-ind);
		}
		ind = (ind + 1) % 4;
		if (!ind) {
			printf("%4i ", value);
			value = 0;
		}
	}

}
