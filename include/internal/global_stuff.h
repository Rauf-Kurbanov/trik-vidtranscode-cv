#ifndef GLOBAL_STUFF_H
#define GLOBAL_STUFF_H

#include <stdlib.h>
#include <stdio.h>

unsigned char* inputData;
myCascade cascadeObj;
myCascade *cascade = &cascadeObj;
MySize minSize = {20, 20};
MySize maxSize = {0, 0};

float scaleFactor = 1.2;
int minNeighbours = 1;
std::vector<MyRect> result;


#endif // GLOBAL_STUFF_H
