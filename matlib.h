#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h> 
pthread_mutex_t lock;
void *addKernel(void *args);
void *scalarMatrixInitKernel(void *args);
void *matmulKernel(void *args);
typedef struct Matrix{
	float* elements;
	int height, width;
}Matrix;
typedef struct ThreadWrapper{
	Matrix a;
	Matrix b;
	Matrix c;
	int threadID;
}ThreadWrapper;

void initThreads(int threads, void* kernel, void*args){
	pthread_t tid[threads];
	if(pthread_mutex_init(&lock, NULL) !=0){
		printf("mutex init failure\n");
	}
	for(int i =0; i<threads; i++)
		pthread_create(&tid[i], NULL, kernel, args);
	for(int i =0; i<threads; i++)
		pthread_join(tid[i], NULL);
}

float* matrixPointer(Matrix mat, int x, int y){
	return &mat.elements[y*mat.width + x];
}
float matrixElement(Matrix mat, int x, int y){
	return mat.elements[y*mat.width + x];
}
Matrix initMatrix(int height, int width, float* elements){
	Matrix mat = {.height = height, .width = width, .elements = elements};
	return mat;
}
void freeMatrix(Matrix mat){
	free(mat.elements);
}
Matrix zeroMatrix(int height, int width){
	float *elements = (float*) calloc(width*height, sizeof(float));
	return initMatrix(height, width, elements);
}
Matrix emptyMatrix(int height, int width){
	float *elements = (float*) malloc(width*height* sizeof(float));
	return initMatrix(height, width, elements );
}
Matrix identityMatrix(int m){
	float* elements = (float*) calloc(m*m, sizeof(float));
	Matrix mat = initMatrix(m,m, elements);
	for(int i =0; i<m;i++)
		mat.elements[i*m + i] = 1;
	return mat;
}
Matrix scalarMatrix(float scalar,int height, int width){
	int n = width*height;
	float *elements = (float*) malloc(sizeof(float)*n);
	Matrix c = {.elements = elements, .height = height, .width = width};
	if(n<1000000)
	for(int i=0; i<n; i++){
		elements[i] = scalar;
	}
	else{
		Matrix a = {.elements = &scalar};
		ThreadWrapper wrapper = {.a = a, .c = c, .threadID = 0};
		int cores = sysconf(_SC_NPROCESSORS_ONLN);
		initThreads(cores, scalarMatrixInitKernel, &wrapper);
	}
	return c;
}
Matrix onesMatrix(int height, int width){
	return scalarMatrix(1.0, height, width);
}
Matrix sequenceMatrix(int height, int width){
	int n = width*height;
	float *elements = (float*) malloc(sizeof(float) * n);
	for(int i = 0; i<n; i++){
		elements[i] = i;
	}
	return initMatrix(width, height, elements);
}
Matrix matAdd(const Matrix a, const Matrix b, bool multithread){
	if(a.height != b.height || a.width != b.width){
		printf("Unable to add a: %ix%i with b: %ix%i\n", a.height, a.width, b.height, b.width);
	}
	float *elements = (float*) malloc(sizeof(float)*a.height*a.width);
	Matrix c = {.height = a.height, .width = a.width, .elements = elements};
	if(!multithread)
	for(int i = 0; i<a.height; i++)
		for(int j = 0; j<a.width; j++)
			elements[i*a.width + j] = a.elements[i*a.width+ j] + b.elements[i*a.width+ j];
	else{
		int cores = sysconf(_SC_NPROCESSORS_ONLN);
		ThreadWrapper wrapper = {.a = a, .b =b, .c = c, .threadID = 0};
		initThreads(cores, addKernel, &wrapper);
	}

	return c;
}

Matrix matMul(Matrix a, Matrix b, bool multithread){
	if(a.width!=b.height){
		printf("Unable to multiply a: %ix%i with %ix%i\n", a.height, a.width, b.height, b.width);
	}
	float* elements = (float*) malloc(sizeof(float)*a.height*b.width);
	Matrix c = {.elements = elements, .height = a.height, .width = b.width};
	if(!multithread)
	for(int i =0; i<a.height; i++){
		for(int j = 0; j<b.width; j++)
			for(int k = 0; k<a.width; k++)
				elements[i*b.width + j] += a.elements[i*a.width + k]*b.elements[k*b.width + j];
	}
	else{
		int cores = sysconf(_SC_NPROCESSORS_ONLN);
		ThreadWrapper wrapper = {.a = a, .b = b, .c = c, .threadID = 0};
		initThreads(cores, matmulKernel, &wrapper);

	}
	return c;
}
void *addKernel(void *args){
	ThreadWrapper *wrapper = (ThreadWrapper*) args;

	int cores = sysconf(_SC_NPROCESSORS_ONLN);

	int width = wrapper->a.width;
	int height= wrapper->a.height;
	
	int xStride = (int) (width/cores);
	int yStride = (int) (height/cores);

	pthread_mutex_lock(&lock);
	const int id = wrapper->threadID;
	++wrapper->threadID;
	pthread_mutex_unlock(&lock);
	int xTerminator = xStride*(id+1);
	int yTerminator = yStride*(id+1);
	if((cores-1) == id){
		xTerminator += width%cores;
		yTerminator += height%cores;
	}

	for(int i = id*yStride; i< yTerminator; i++){
		for(int j = id*xStride; j<xTerminator; j++)
			wrapper->c.elements[i*width + j] = wrapper->a.elements[i*width + j] + wrapper->b.elements[i*width + j];
		}
    return NULL;
}
void *matmulKernel(void *args){
	ThreadWrapper *wrapper = (ThreadWrapper*) args;
	int cores = sysconf(_SC_NPROCESSORS_ONLN);

	int xStride = (int) wrapper->c.width/cores;
	int yStride = (int) wrapper->c.height/cores;
	pthread_mutex_lock(&lock);
	const int id = wrapper->threadID;
	++wrapper->threadID;
	pthread_mutex_unlock(&lock);
	int xTerminator = xStride*(id + 1);
	int yTerminator = yStride*(id + 1);
	if((cores-1) == id){
		xTerminator += wrapper->c.width%cores;
		yTerminator += wrapper->c.height%cores;
	}

	for(int i = yStride*id; i<yTerminator; i++)
		for(int j = xStride*id;j<xTerminator; j++)
			for(int k =0; k<wrapper->a.width;k++)
				wrapper->c.elements[i*wrapper->b.width + j] += wrapper->a.elements[i*wrapper->a.width + k]
																*wrapper->b.elements[k*wrapper->b.width + j];
	return NULL;	
}
void *scalarMatrixInitKernel(void *args){
	ThreadWrapper *wrapper = (ThreadWrapper*) args;
	int cores = sysconf(_SC_NPROCESSORS_ONLN);
	int n = (wrapper->c.width)*(wrapper->c.height);
	int stride = (int) (n/cores);
	int remainder = n%cores;

	pthread_mutex_lock(&lock);
	const int id = wrapper->threadID;
	++wrapper->threadID;
	pthread_mutex_unlock(&lock);

	int terminator = stride*(id+1);
	if(id == (cores-1))
		terminator += n%cores;

	for(int i = id*stride; i<terminator; i++)
		wrapper->c.elements[i] = wrapper->a.elements[0];
	return NULL;

}

