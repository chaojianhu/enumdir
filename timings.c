#include <stdio.h>

void print_index(int line, int page, int ncache_lines, int npages, int npages_per_line) {
	int row, col, sum;
	for(row = 0; row < npages; ++row) {
			col = (line + (row + page) / npages_per_line) % ncache_lines;
			sum = col + row * ncache_lines;
			printf("%04x-%04x ", col, sum);
			if(row == 127) {
				printf("\n");
			} else if((row + 1) % 8 == 0) {
				printf("\n        ");
			}
	}
}

int main(int argc, char * argv[]) {
	int i, j;
	for(i = 0; i < 64; i++) {
		for(j = 0; j < 8; j++) {
			printf("%04x :  ", i * 8 + j);
			print_index(i, j, 64, 128, 8);			
		}
	}
	return 0;
}

