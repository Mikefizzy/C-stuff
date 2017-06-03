#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h> 
#include <string.h>
#include <ctype.h>
#include "matlib.h"
Matrix loadCsv(FILE* file, int bufferSize, int numBufferSize, int nElements);
Matrix loadCsv(FILE* file,int bufferSize,int numBufferSize, int nElements){
	if(bufferSize == 0)
		bufferSize = 32000;
	if(numBufferSize ==0)
		numBufferSize = 20;
	if(nElements ==0)
		nElements = 1024;
	char buffer[bufferSize], numBuffer[numBufferSize];
	int numIndex = 0, width = 0,digitIdx = 0 ,nread;
	bool hitNew = false;
	float* elements = (float*) malloc(sizeof(float)*nElements);

	while((nread = fread(buffer, 1, bufferSize, file))>0){
		for(int i =0; i<bufferSize; i++){
			if(isdigit(buffer[i]) || buffer[i] == '.'){
				numBuffer[digitIdx] = buffer[i];
				digitIdx++;
			}
			else if(buffer[i] == ',' || buffer[i] == '\n'){
				if(!hitNew)
				switch(buffer[i]){
					case '\n':
					hitNew = true;
					case ',':
					width++;
					break;

				}
				numBuffer[digitIdx] = '\0';
				if(numIndex>=nElements){
					nElements += (nElements/2);
					elements = (float*)realloc(elements, sizeof(float)*nElements);
				}

				elements[numIndex] = strtof(numBuffer,NULL);
				memset(numBuffer, 0, numBufferSize);
				digitIdx = 0;
				numIndex++;
		}else if(buffer[i]!=' '){
			break;
		}

		}
	}
	if(nElements>(numIndex)){
			elements = (float*)realloc(elements, (numIndex+1)*sizeof(float));
		}
	int height = (numIndex)/width;
	if((numIndex)%width>0)
		printf("Matrix loaded is not even, mod %i\n", numIndex%width);
	Matrix mat = {.elements = elements, .height = height, .width = width};
	return mat;
}
