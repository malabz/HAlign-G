/* multiz.c   version 11.2
 *
 * Change argument row2 to all, make it default not to output
 * single-row blocks. -- 10/06/05
 * Add option not to output single-row blocks. -- 09/28/05
 *
 * most recent modification: previously not to break some large
 * blocks when aligned to very small blocks, the unused small blocks
 * overlap with large blocks when they are both output to the same
 * file. Fixed by enforcing aligning for any size. Version output
 * are made consistent. -- 09/26/05
 *
*  when input block only contain few bases
*  length, it was ignored and regarded as unused. If both input
*  files contain the short blocks at the same positions, overlapping
*  happens. Fixed by enforcing aligning for any size. -- 09/15/05
*
*  Modified on Dec. 22, if [out1][out2] are specified, unused
*  blocks are output to out1 and out2, if not specified,
*  unused blocks are output to stdout.
*
*  Modified on Aug. 31 to fix the bug where blocks might
*  be lost when column width is below a certain value.
*
*  Modified on Aug. 31 to allow for arguments of R and M.
*
*  Variant to multiz program. It aligns two files of
*  alignment blocks where top row is always the reference,
*  assuming blocks are increasing ordered based on the
*  start position on the refernece seqence. Single-coverage
*  on reference is required at this stage.
*
*  Three arguments are required: char* arg1, char* arg2,
*  int v. arg1 and arg2 are two files need to be aligned
*  together. The blocks are always topped by a reference,
*  for being used to determine the approximate alignment,
*  but the referece is fixed in that block or not, depending
*  on the argument of reference value of v:
*     0:  neither is fixed
*     1:  the first block is fixed
*/

#include <stdio.h>

#include <stdlib.h>
#include "maf.h"
#include "util.h"
#include "multi_util.h"
#include "mz_preyama.h"
#include "mz_scores.h"

extern int row2;
extern int radius;
extern int MIN_OUTPUT_WID;
extern int LRG_BREAK_WID;
extern int SML_BREAK_WID;

#define VERSION 11.2

struct mafAli* two_list_to_one(struct mafAli** cp_list1, struct mafAli** cp_list2, FILE* fpw1, FILE* fpw2);
struct mafAli* mul_two_list_to_one(int ii, struct mafAli** result, struct mafAli** cp_list1, struct mafAli** cp_list2, FILE* fpw1, FILE* fpw2);
void mul_two_file_to_one(char* path, int ii, int jj, int jj1);
void mul_mafReadAll(const char* path, const char* _maf, int file_i, struct mafAli** cp_list_i);
void mul_mafRead_sort_writeAll(const char* path, const char* _maf, int file_i);
int multiz(struct mafAli** wk_list1, struct mafAli** wk_list2, FILE* fpw1, FILE* fpw2, int v, struct mafAli** t);

int main_(int argc, char** argv);

int multiz_main(int filenum, char* path);

int multiz_main_sv(int filenum, char* path);

int multiz_main2(int result_maf, char* file_name1, char* file_name2, char* ans_name, int thresh1, int thresh2, int thresh3);

void multiz_main3(char* file_name1, char* file_name2, char* ans_name, int thresh1, int thresh2, int thresh3);