#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <complex.h>
#include <string.h>
#include <pthread.h>

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

double PI;

typedef double complex cplx;

char* 	inputFile;
char* 	outputFile;
int 	P;
int 	N;
cplx*	input;
cplx* 	output;
cplx*	buf;
cplx*	out;
pthread_barrier_t barrier;


void getArgs(int argc, char * argv[]) {
	if (argc < 4) {
		printf("Not enough parameters.\n");
		exit(1);
	}

	inputFile = calloc(sizeof(char), (strlen(argv[1]) + 1));
	strcpy(inputFile, argv[1]);

	outputFile = calloc(sizeof(char), (strlen(argv[2]) + 1));
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

void _fft(cplx *buf, cplx *out, int step)
{
	if (step < N) {
		_fft(out, buf, step * 2);
		_fft(out + step, buf + step, step * 2);
 
		for (int i = 0; i < N; i += 2 * step) {
			cplx t = cexp(-I * PI * i / N) * out[i + step];
			buf[i / 2]     = out[i] + t;
			buf[(i + N)/2] = out[i] - t;
		}
	}
}


void fft(cplx *buf, cplx* out)
{
	for (int i = 0; i < N; i++) out[i] = buf[i];
 
	_fft(buf, out, 1);
}

// Parallelized FFT
void* FFT_Par(void* var) {
	int tid = *(int*)var;

	int start   = tid * ceil((double)N / P);
    int end     = MIN(N, (tid + 1) * ceil((double)N / P));
    int step;

    // Sequential
    if (P == 1) {
    	// Root - _fft: buf & out
    	// step = 1
    	_fft(buf, out, 1);
    }

    // 2 Threads => Recursive call from each thread
    // on layer 1 children. Need to compute root
    if (P == 2) {
    	// Layer1 - _fft: out & buf
    	// step = 2
    	_fft(out + tid, buf + tid, 2);

    	// Computing Root
    	step = 1;
    	for (int i = 0; i < N; i += 2 * step) {
			cplx t = cexp(-I * PI * i / N) * out[i + step];
			buf[i / 2]     = out[i] + t;
			buf[(i + N)/2] = out[i] - t;
		}
    }

    // 4 Threads => Recursive call from each thread
    // on layer 2 children. Need to compute root
    // and layer 1 children
    if (P == 4) {
    	// Layer 2 - _fft: buf & out
    	// step = 4
    	_fft(buf + tid, out + tid, 4);
    	// Wait until all L2 subtrees are computed
    	pthread_barrier_wait(&barrier);

    	step = 2;
    	// Computng first L1 child
    	for (int i = 0; i < N; i += 2 * step) {
			cplx t = cexp(-I * PI * i / N) * buf[i + step];
			out[i / 2]     = buf[i] + t;
			out[(i + N)/2] = buf[i] - t;
		}
		pthread_barrier_wait(&barrier);

		// Computing second L1 child -> buf+1, out+1
		for (int i = 0; i < N; i += 4) {
			cplx t = cexp(-I * PI * i / N) * buf[1 + i + step];
			out[1 + i / 2]     = buf[1 + i] + t;
			out[1 + (i + N)/2] = buf[1 + i] - t;
		}
		pthread_barrier_wait(&barrier);

		// Computing root
		step = 1;
		for (int i = 0; i < N; i += 2 * step) {
			cplx t = cexp(-I * PI * i / N) * out[i + step];
			buf[i / 2]     = out[i] + t;
			buf[(i + N)/2] = out[i] - t;
		}

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
	pthread_barrier_destroy(&barrier);
}

int main(int argc, char * argv[]) {
	PI = atan2(1, 1) * 4;

	getArgs(argc, argv);
	parseInput(inputFile);


	for (int i = 0; i < N; ++i) output[i] = input[i];
	// using buf and out for simplicity when calling _fft
	buf = output;
	out = input;

	pthread_t tid[P];	
	int thread_id[P];
	for(int i = 0; i < P; i++)
		thread_id[i] = i;


	// initialising barrier
	pthread_barrier_init(&barrier, NULL, P);

	for(int i = 0; i < P; i++) {
		pthread_create(&(tid[i]), NULL, FFT_Par, &(thread_id[i]));
	}
	for(int i = 0; i < P; i++) {
		pthread_join(tid[i], NULL);
	}
	writeOutput(outputFile);
	destroy();
}
