#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <error.h>
#include <errno.h>
typedef struct Vector {

	int capacity;
	intptr_t *tab;
	int rgFirstElt;
	int nbElt;

} vector_;
#define MAX_Elts 	50
vector_ * createVector(int capacity);
int addElt(void *pe, vector_ *v);
void *removeFirst(vector_ *v);
void *elementAt(int index, vector_ *v);


vector_ * createVector(int capacity){
	vector_ *v=NULL;
	v=malloc(sizeof(vector_));
	v->capacity=capacity;
	v->nbElt=0;
	v->rgFirstElt=0;
	v->tab = calloc(v->capacity,sizeof(void *));

	return v;
}

int addElt(void *pe, vector_ *v){

	if (v->nbElt >= v->capacity) {

		//pintf("Error : addElt in vector\n");
		return -1;
	}
	v->tab[(v->rgFirstElt+v->nbElt)%v->capacity] = (intptr_t)pe;
	v->nbElt++;

	return 1;

}

void *removeFirst(vector_ *v){

	void *pe;
	if (v->nbElt == 0) {
		 error_at_line(EXIT_FAILURE,errno,__FILE__,__LINE__,"removeFirst");

	}
	pe = (void *)(v->tab[v->rgFirstElt]);
	v->tab[v->rgFirstElt] = (intptr_t)NULL;
	v->rgFirstElt=(v->rgFirstElt+1)%v->capacity;
	v->nbElt--;

	return pe;

}

void *elementAt(int index, vector_ *v){

	if (index >= v->nbElt) {

		error_at_line(EXIT_FAILURE,errno,__FILE__,__LINE__,"elementAt");

	}
	return  (void *)(v->tab[(v->rgFirstElt+index)%v->capacity]);

}

int numberOfElt(vector_ *v){
  return v->nbElt;
}

