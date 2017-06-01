#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
pthread_mutex_t lock;
void *addKernel(void *args);
void *onesInitKernel(void *args);
void *scalarMulKernel(void *args);
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

typedef struct MatrixInitWrapper{
	Matrix mat;
	int threadID;
}MatrixInitWrapper;

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
	return initMatrix(width, height, elements);
}
Matrix identityMatrix(int m){
	float* elements = (float*) calloc(m*m, sizeof(float));
	Matrix mat = initMatrix(m,m, elements);
	if(m<=1000)
	for(int i =0; i<m;i++)
		mat.elements[i*m + i] = 1;
	return mat;
}
Matrix onesMatrix(int height, int width){
	int n = width*height;
	float *elements = (float*) malloc(sizeof(float)*n);
	Matrix mat = {.elements = elements, .height = height, .width = width};
	int cores = sysconf(_SC_NPROCESSORS_ONLN);
	if(n<1000)
	for(int i=0; i<n; i++){
		elements[i] = 1;
	}
	else{
		MatrixInitWrapper wrapper = {.mat = mat, .threadID = 0};
		int cores = sysconf(_SC_NPROCESSORS_ONLN);
		pthread_t tid[cores];
	if(pthread_mutex_init(&lock, NULL) !=0){
		printf("mutex init failure\n");
	}
		for(int i =0; i<cores; i++)
			pthread_create(&tid[i], NULL, onesInitKernel, &wrapper);
		for(int i =0; i<cores; i++)
			pthread_join(tid[i], NULL);
	}
	return initMatrix(width, height, elements);
}
Matrix sequenceMatrix(int height, int width){
	int n = width*height;
	float *elements = (float*) malloc(sizeof(float) * n);
	for(int i = 0; i<n; i++){
		elements[i] = i;
	}
	return initMatrix(width, height, elements);
}
Matrix matAdd(const Matrix a, const Matrix b){
	if(a.height != b.height || a.width != b.width){
		printf("Unable to add a: %ix%i with b: %ix%i\n", a.height, a.width, b.height, b.width);
	}
	float *elements = (float*) malloc(sizeof(float)*a.height*a.width);
	for(int i = 0; i<a.height; i++){
		for(int j = 0; j<a.width; j++){
			elements[i*a.width + j] = a.elements[i*a.width+ j] + b.elements[i*a.width+ j];
		}
	}
	return initMatrix(a.height, a.width, elements);
}


Matrix fasterMatAdd(const Matrix a, const Matrix b){
	int cores = sysconf(_SC_NPROCESSORS_ONLN);
	ThreadWrapper wrapper = {.a = a, .b =b, .c = c, .threadID = 0};
	Matrix c = zeroMatrix(a.width, a.height);
	pthread_t tid[cores];
	if(pthread_mutex_init(&lock, NULL) !=0){
		printf("mutex init failure\n");
	}
	for(int i =0; i<cores; i++)
		pthread_create(&tid[i], NULL, addKernel, &wrapper);
	for(int i =0; i<cores; i++)
		pthread_join(tid[i], NULL);
	return wrapper.c;
}
Matrix scalarMul(float a, Matrix mat){
	float* elements = (float*)malloc(sizeof(float)*mat.height*mat.width);
	Matrix c = {.elements = elements, .height  = mat.height, .width = mat.width};
	if(mat.height*mat.width > 100000)
	for(int i = 0; i<mat.height; i++)
		for(int j = 0; j<mat.width; i++)
			elements[i*a.width + j] = mat.elements[i*a.width + j]*a;
	else{
		Matrix scalarMat = {.elements = &a, .height = 1, .width= 1};
		Matrix c = {.elements = elements, .height = mat.height, .width = mat.width};
		ThreadWrapper wrapper = {.threadID = 0, .a = scalarMat, .b = mat, .c = c};
		int cores = sysconf(_SC_NPROCESSORS_ONLN);


	}
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
void *onesInitKernel(void *args){
	MatrixInitWrapper *wrapper = (MatrixInitWrapper*) args;
	int cores = sysconf(_SC_NPROCESSORS_ONLN);
	int n = (wrapper->mat.width)*(wrapper->mat.height);
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
		wrapper->mat.elements[i] = 1;
	return NULL;

}

