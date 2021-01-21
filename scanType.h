/***************************
 * Ronnie Keating
 * CS445
 * Assignment 2
 * scanType.h
 ***************************/

#ifndef SCANTYPE_H
#define SCANTYPE_H
struct TokenData {
    int tokenclass;     // token class
    int linenum;        // what line did this token occur on?
    char *tokenstr;     // char string that I saw
    int idIndex;        // index for id
    int numValue;    // the value of the number as a DOUBLE!!
};

#endif

