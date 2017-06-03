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
					case ',':
					width++;
					break;
					case '\n':
					width++;
					hitNew = true;
					break;

				}
				numBuffer[digitIdx] = '\0';
				if(numIndex==nElements){
					nElements += nElements/2;
					elements = realloc(elements, sizeof(float)*nElements);
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
	if(nElements>(numIndex+1)){
			elements = realloc(elements, (numIndex+1));
		}
	int height = (numIndex+1)/width;
	Matrix mat = {.elements = elements, .height = height, .width = width};
	return mat;
}