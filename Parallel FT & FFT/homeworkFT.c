#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <complex.h>
#include <string.h>

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

typedef double complex cplx;

char* 	inputFile;
char* 	outputFile;
int 	P;
int 	N;
cplx*	input;
cplx* 	output;

void getArgs(int argc, char * argv[]) {
	if (argc < 4) {
		printf("Not enough parameters.\n");
		exit(1);
	}

	inputFile = malloc(sizeof(char) * (strlen(argv[1]) + 1));
	strcpy(inputFile, argv[1]);

	outputFile = malloc(sizeof(char) * (strlen(argv[2]) + 1));
	strcpy(outputFile, argv[2]);

	P = atoi(argv[3]);
}

void parseInput(char* filename) {
	FILE* fp;
	fp = fopen(filename, "r");

	if (fp == NULL) {
		printf("Invalid input filename.\n");
		exit(1);
	}

	fscanf(fp, "%d\n", &N);

	input = malloc(sizeof(cplx) * N);
	output = malloc(sizeof(cplx) * N);

	for (int i = 0; i < N; ++i) {
		fscanf(fp, "%lf\n", &input[i]);
	}

	fclose(fp);
}

void DFT_Seq() {
	for (int k = 0; k < N; k++) {
		cplx sum = 0.0;
		for (int t = 0; t < N; t++) {
			double angle = 2 * M_PI * t * k / N;
			sum += input[t] * cexp(-angle * I);
		}
		output[k] = sum;
	}
}

/*
Correct output - TRUE
1 thread  - 1.3s
2 threads -	0.72s
3 threads - 0.63s
4 threads - 0.55s
*/
void* DFT_Par(void* var) {
	int tid = *(int*)var;

	int start   = tid * ceil((double)N / P);
    int end     = MIN(N, (tid + 1) * ceil((double)N / P));

    for (int k = start; k < end; k++) {
		cplx sum = 0.0;
		for (int t = 0; t < N; t++) {
			double angle = 2 * M_PI * t * k / N;
			sum += input[t] * cexp(-angle * I);
		}
		output[k] = sum;
	}
}

void writeOutput(char *filename) {
	FILE* fp;
	fp = fopen(filename, "w");

	if (fp == NULL) {
		printf("Cannot create/open output file.\n");
		exit(1);
	}

	fprintf(fp, "%d\n", N);

	for (int i = 0; i < N; ++i) {
		fprintf(fp, "%lf %lf\n", creal(output[i]), cimag(output[i]));
	}

	fclose(fp);
}

void destroy() {
	free(inputFile);
	free(outputFile);
	free(input);
	free(output);
}

int main(int argc, char * argv[]) {
	getArgs(argc, argv);
	parseInput(inputFile);

	pthread_t tid[P];
	int thread_id[P];
	for(int i = 0; i < P; i++)
		thread_id[i] = i;

	for(int i = 0; i < P; i++) {
		pthread_create(&(tid[i]), NULL, DFT_Par, &(thread_id[i]));
	}

	for(int i = 0; i < P; i++) {
		pthread_join(tid[i], NULL);
	}

	writeOutput(outputFile);
	destroy();
}
