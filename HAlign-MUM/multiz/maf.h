#ifndef MAF_H
#define MAF_H

/*
 *   maf.h version 12
 */
 // mini-maf.h - Multiple alignment format, header file stolen from Jim Kent and
 // abused
#include <stdio.h>

#define MAX_INT ((int)(~(1<<(sizeof(int)*8-1))))
#define MIN_INT ((int)(1<<(sizeof(int)*8-1)))

struct mul_two {
    size_t start;
    size_t end;
    struct mul_two* next;
};

struct mafFile
    /* A file full of multiple alignments. */
{
    struct mafFile* next;
    int version;		// Required
    char* scoring;		// Name of scoring scheme.
    struct mafAli* alignments;	// Possibly empty list of alignments.
    char* fileName;
    int line_nbr;
    int verbose;
    FILE* fp; 			// Open file if any. NULL except while parsing.
};

void mafFileFree(struct mafFile** pObj);
/* Free up a maf file including closing file handle if necessary. */

struct mafAli
    /* A multiple alignment. */
{
    struct mafAli* next;
    double score;
    struct mafComp* components;	/* List of components of alignment */
    int textSize;	 /* Size of text in each component. */
    //int chain_len;
};

void mafAliFree(struct mafAli** pObj);
/* Free up a maf alignment. */

struct mafComp
    /* A component of a multiple alignment. */
{
    struct mafComp* next;
    char* name;        /* comman name of sequence source. */
    char* src;	 /* Name of sequence source.  */
    char* text;        /* The sequence including dashes. */
    char* contig;
    int* mafPosMap;
    int srcSize;       /* Size of sequence source.  */
    int start;	 /* Start within sequence. Zero based. If strand is - is relative to src end. */
    int size;	         /* Size in sequence (does not include dashes).  */
    short nameID;
    char strand;       /* Strand of sequence.  Either + or -*/
    char paralog;
};

void mafCompFree(struct mafComp** pObj);
/* Free up a maf component. */
struct mafAli* copyMafAliNode(const struct mafAli* node);
struct mafComp* copyMafCompList(const struct mafComp* comp);
struct mafFile* mafOpen(char* fileName, int verbose);
/* Open up a .maf file for reading.  Read header and
 * verify. Prepare for subsequent calls to mafNext().
 * Prints error message and aborts if there's a problem. */

struct mafAli* mafNext(struct mafFile* mafFile);
struct mafAli* my_mafNext(struct mafFile* mafFile);
struct mafAli* my_mafNext_noY(struct mafFile* mf);
/* Return next alignment in file or NULL if at end.
 * This will close the open file handle at end as well. */

struct mafFile* mafReadAll(char* fileName, int verbose);
struct mafFile* my_mafReadAll(char* fileName, int verbose);
/* Read in full maf file */
void printStart(struct mafAli* head);

struct mafAli* merge(struct mafAli* left, struct mafAli* right);
struct mafAli* mergeSort(struct mafAli* head);

struct mafFile* mafReadAll_no_sort(char* fileName, int verbose);
void my_mafRead_sort_writeAll(char* fileName, char* ans_name, int verbose);
void my_mafRead_sort_writeAll_p(struct mafFile* mf, FILE* fpw1);
void my_mafReadAll_p(struct mafFile* mf, struct mafAli** maf_arrayi);

void mafWriteStart(FILE* f, char* scoring);
/* Write maf header and scoring scheme name (may be null) */

void mafWriteEnd(FILE* f);

void mafWrite(FILE* f, struct mafAli* maf);

/* Write next alignment to file. */

void my_mafWrite(FILE* f, struct mafAli* a, int thresh1, int thresh2, int thresh3, int thresh4);//1-len,2-num,3-Continuity,4-score.
/* Write next alignment to file. */

void my_mafWrite_only(FILE* f, struct mafAli* a, int thresh1, int thresh2, int thresh3, int thresh4);

struct mafAli* mafColDashRm(struct mafAli* a);
/* remove columns which contain all dashes. */

struct mafAli* mafRowDashRm(struct mafAli* a);
/* remove components which contain all dashes. */

struct mafComp* mafNewComp(char* src, int start, int size, char strand, int srcSize, int len, char paralog, char* name, char* contig);
/* construct a new component. */

struct mafAli* make_part_ali(struct mafAli* template1, int cbeg, int cend);

struct mafComp* mafCpyComp(struct mafComp*);
struct mafAli* mafNewAli(double score, int textSize);
struct mafAli* duplicate_ali(struct mafAli* template1);

#endif
