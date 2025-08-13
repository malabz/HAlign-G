#include "multiz.h"


int multiz(struct mafAli** wk_list1, struct mafAli** wk_list2, FILE* fpw1, FILE* fpw2, int v, struct mafAli** t) {
    struct mafAli* a1, * a2, * new_ali;
    int beg1, end1, beg2, end2, beg, end, col_beg, col_end;
    int test = 0;

    a1 = retrieve_first(wk_list1);
    a2 = retrieve_first(wk_list2);

    while (1) {
        while (a1 != NULL && (a2 == NULL || a1->components->start + a1->components->size - 1 < a2->components->start)) {
            if (a1->components->size >= MIN_OUTPUT_WID && fpw1 != NULL && (row2 == 0 || a1->components->next != NULL))
            {
                (*t)->next = a1; (*t) = a1;
            }
                //mafWrite(fpw1, a1);//ztzt
            else
                mafAliFree(&a1);
            a1 = retrieve_first(wk_list1);
        }
        while (a2 != NULL && (a1 == NULL || a2->components->start + a2->components->size - 1 < a1->components->start)) {
            if (a2->components->size >= MIN_OUTPUT_WID && fpw2 != NULL && (row2 == 0 || a2->components->next != NULL))
            {
                (*t)->next = a2; (*t) = a2;
            }
            else
                //mafWrite(fpw2, a2);//ztzt
                mafAliFree(&a2);
            a2 = retrieve_first(wk_list2);
        }
        if (a1 == NULL && a2 == NULL)
            break;
        if (a1 == NULL || a2 == NULL)
            continue;
        if (a1->components->start + a1->components->size - 1 < a2->components->start)
            continue;
        if (a2->components->start + a2->components->size - 1 < a1->components->start)
            continue;

        if (a1->components->start == 11305)
            test++;


        beg1 = a1->components->start;                           // at this point, a1 a2 overlap or
        end1 = a1->components->start + a1->components->size - 1;// cover, print uncovered front part
        beg2 = a2->components->start;                           // pre_yama middle/covered part,
        end2 = a2->components->start + a2->components->size - 1;// then add another ali to process end part

        if (beg1 < beg2&& beg2 - beg1 >= MIN_OUTPUT_WID && fpw1 != NULL) {
            col_beg = mafPos2Col(a1->components, beg1, a1->textSize);
            for (; col_beg > 0 && a1->components->text[col_beg - 1] == '-'; col_beg--)
                ;
            col_end = mafPos2Col(a1->components, beg2 - 1, a1->textSize);
            for (; col_end < a1->textSize - 1 && a1->components->text[col_end + 1] == '-'; col_end++)
                ;
            print_part_ali_col(a1, col_beg, col_end, fpw1, &t);
        }
        else if (beg2 < beg1&& beg1 - beg2 >= MIN_OUTPUT_WID && fpw2 != NULL) {
            col_beg = mafPos2Col(a2->components, beg2, a2->textSize);
            for (; col_beg > 0 && a2->components->text[col_beg - 1] == '-'; col_beg--)
                ;
            col_end = mafPos2Col(a2->components, beg1 - 1, a2->textSize);
            for (; col_end < a2->textSize - 1 && a2->components->text[col_end + 1] == '-'; col_end++)
                ;
            print_part_ali_col(a2, col_beg, col_end, fpw2, &t);
        }


        beg = beg1 > beg2 ? beg1 : beg2;
        end = end1 < end2 ? end1 : end2;

        if (beg == beg1) { // for gaps in front
            col_beg = mafPos2Col(a1->components, beg1, a1->textSize);
            if (col_beg != 0 && fpw1 != NULL)
            {
                print_part_ali_col(a1, 0, col_beg - 1, fpw1, &t);
            }
        }
        if (beg == beg2) { // for gaps in front
            col_beg = mafPos2Col(a2->components, beg2, a2->textSize);
            if (col_beg != 0 && fpw2 != NULL)
            {
                print_part_ali_col(a2, 0, col_beg - 1, fpw2, &t);
            }
        }
        new_ali = pre_yama(a1, a2, beg, end, radius, v, fpw2, &t);

        if (new_ali != NULL && new_ali->components->size >= MIN_OUTPUT_WID)
        {
            (*t)->next = new_ali; (*t) = new_ali;
        }
        else
            //mafWrite(stdout, new_ali);//ztzt
            mafAliFree(&new_ali);

        if (end1 < end2)
            a2 = keep_ali(a2, end1 + 1);

        if (end2 < end1)
            a1 = keep_ali(a1, end2 + 1);

        if (end1 <= end2) {
            col_end = mafPos2Col(a1->components, end1, a1->textSize);
            if (col_end < a1->textSize - 1 && fpw1 != NULL)
            {
                print_part_ali_col(a1, col_end + 1, a1->textSize - 1, fpw1, &t);
            }
            else
                mafAliFree(&a1);
            a1 = retrieve_first(wk_list1);
        }
        if (end2 <= end1) {
            col_end = mafPos2Col(a2->components, end2, a2->textSize);
            if (col_end < a2->textSize - 1 && fpw2 != NULL)
            {
                print_part_ali_col(a2, col_end + 1, a2->textSize - 1, fpw2, &t);
            }
            else
                mafAliFree(&a2);
            a2 = retrieve_first(wk_list2);
        }
    }
    return 0;
}

int main_(int argc, char** argv) {
    char USAGE[10000];
    struct mafFile* maf1, * maf2;
    struct mafAli* ali, * cp_list1, * cp_list2, * wk_list1, * wk_list2;
    int x, nohead = 0, v, i;
    FILE* fpw1 = NULL, * fpw2 = NULL, * fpw3 = NULL;
    char ref_chr[200], cmd[200], args[2000];
    
    struct mafAli* new_two_head = ckalloc(sizeof(struct mafAli));
    struct mafAli *tmp = NULL, * tmp1 = NULL;
    
    new_two_head->next = NULL;
    tmp = new_two_head;

    sprintf(cmd, "multiz.v%.1f", VERSION);
    argv0 = cmd;

    // ---- process arguments
    fpw1 = fpw2 = fopen(argv[4], "w");
    //fpw1 = fpw2 = stdout;

    v = atoi(argv[3]);   // v is used later to determine the way to use yama
    if (v != 0 && v != 1)
        fatal("v can only be value of 0, 1 ");
    // ---- end of processing arguments
    if (nohead == 0) {
        char str[7] = { 'm','u','l','t','i','z', '\0'};
        mafWriteStart(fpw1, str);
    }
    init_scores70();

    maf1 = mafReadAll(argv[1], 1);
    maf2 = mafReadAll(argv[2], 1);

    cp_list1 = maf1->alignments;
    cp_list2 = maf2->alignments;
    maf1->alignments = maf2->alignments = NULL;
    mafFileFree(&maf1);
    mafFileFree(&maf2);

    while (cp_list1 != NULL && cp_list2 != NULL) {
        wk_list1 = wk_list2 = NULL;
        strcpy(ref_chr, cp_list1->components->src);
        seperate_cp_wk(&cp_list1, &wk_list1, ref_chr);
        seperate_cp_wk(&cp_list2, &wk_list2, ref_chr);
        multiz(&wk_list1, &wk_list2, fpw1, fpw2, v, &tmp);

    }

    if (cp_list1 != NULL && fpw1 != NULL)
        for (ali = cp_list1; ali != NULL; ali = ali->next)
            if (row2 == 0 || ali->components->next != NULL)
                //mafWrite(fpw1, ali);//ztzt
            {
                tmp->next = ali; tmp = ali; 
            }
            else
                mafAliFree(&ali);

    if (cp_list2 != NULL && fpw2 != NULL)
        for (ali = cp_list2; ali != NULL; ali = ali->next)
            if (row2 == 0 || ali->components->next != NULL)
                //mafWrite(fpw2, ali);//ztzt
            {
                tmp->next = ali; tmp = ali;
            }
            else
                mafAliFree(&ali);
    tmp = new_two_head->next;
    while (tmp)
    {
        mafWrite(fpw1, tmp);
        tmp1 = tmp->next;
        mafAliFree(&tmp);
        tmp = tmp1;
    }
    mafWriteEnd(fpw1);
    if (fpw1 != NULL)
        fclose(fpw1);
    if (fpw2 != NULL)
        fclose(fpw2);
    ckfree(new_two_head);
    return 0;
}

struct mafAli* two_list_to_one(struct mafAli** cp_list1, struct mafAli** cp_list2, FILE* fpw1, FILE *fpw2)
{
    struct mafAli* ali, * wk_list1, * wk_list2;
    char ref_chr[200];
    int v = 1;
    struct mafAli* new_two_head = (struct mafAli*)malloc(sizeof(struct mafAli));
    struct mafAli* tmp = NULL, * tmp1 = NULL;
    
    new_two_head->next = NULL;
    tmp = new_two_head;

    while ((*cp_list1) != NULL && (*cp_list2) != NULL) {
        wk_list1 = wk_list2 = NULL;
        strcpy(ref_chr, (*cp_list1)->components->src);
        seperate_cp_wk(&(*cp_list1), &wk_list1, ref_chr);
        seperate_cp_wk(&(*cp_list2), &wk_list2, ref_chr);
        multiz(&wk_list1, &wk_list2, fpw1, fpw2, v, &tmp);
    }

    if ((*cp_list1) != NULL && fpw1 != NULL)
        for (ali = (*cp_list1); ali != NULL; ali = ali->next)
            if (row2 == 0 || ali->components->next != NULL)
            {
                tmp->next = ali; tmp = ali;
            }
            else
                mafAliFree(&ali);

    if ((*cp_list2) != NULL && fpw2 != NULL)
        for (ali = (*cp_list2); ali != NULL; ali = ali->next)
            if (row2 == 0 || ali->components->next != NULL)
            {
                tmp->next = ali; tmp = ali;
            }
            else
                mafAliFree(&ali);
    tmp = new_two_head;
    new_two_head = new_two_head->next;
    free(tmp);
    return new_two_head;
} 

struct mafAli* mul_two_list_to_one(int ii, struct mafAli** result, struct mafAli** cp_list1, struct mafAli** cp_list2, FILE* fpw1, FILE* fpw2)
{
    //printf("%dstart\n", ii);
    //printf("%d\t%d\t%d\n", result, cp_list1, cp_list2);
    struct mafAli* ali, * wk_list1, * wk_list2;
    char ref_chr[200];
    int v = 1;
    struct mafAli* new_two_head = (struct mafAli*)malloc(sizeof(struct mafAli));
    struct mafAli* tmp = NULL, * tmp1 = NULL;

    new_two_head->next = NULL;
    tmp = new_two_head;

    while ((*cp_list1) != NULL && (*cp_list2) != NULL) {
        wk_list1 = wk_list2 = NULL;
        strcpy(ref_chr, (*cp_list1)->components->src);
        seperate_cp_wk(&(*cp_list1), &wk_list1, ref_chr);
        seperate_cp_wk(&(*cp_list2), &wk_list2, ref_chr);
        multiz(&wk_list1, &wk_list2, fpw1, fpw2, v, &tmp);
    }

    if ((*cp_list1) != NULL && fpw1 != NULL)
        for (ali = (*cp_list1); ali != NULL; ali = ali->next)
            if (row2 == 0 || ali->components->next != NULL)
            {
                tmp->next = ali; tmp = ali;
            }
            else
                mafAliFree(&ali);

    if ((*cp_list2) != NULL && fpw2 != NULL)
        for (ali = (*cp_list2); ali != NULL; ali = ali->next)
            if (row2 == 0 || ali->components->next != NULL)
            {
                tmp->next = ali; tmp = ali;
            }
            else
                mafAliFree(&ali);
    tmp = new_two_head;
    new_two_head = new_two_head->next;
    free(tmp);
    (*cp_list1) = (*cp_list2) = NULL;
    (*result) = new_two_head;
    //printf("%dend\n", ii);
}

void mul_two_file_to_one(char* path, int ii, int jj, int jj1)
{
    struct mafAli* tmp = NULL;
    char file_ii[1000];
    char file_jj[1000];
    char file_jj1[1000];
    sprintf(file_ii, "%s%d_.tmp", path, ii);
    
    sprintf(file_jj, "%s%d_.maf", path, jj);
    sprintf(file_jj1, "%s%d_.maf", path, jj1);
   
    //printf("%s%d_.tmp\n", path, ii);
    //printf("%s%d_.maf\n", path, jj);
    //printf("%s%d_.maf\n\n", path, jj1);
    FILE* fpw1 = fopen(file_ii, "w");
    char str[7] = { 'm','u','l','t','i','z', '\0' };
    mafWriteStart(fpw1, str);
    //fpw1 = stdout;
    struct mafFile* mfjj = mafOpen(file_jj, 1);
    struct mafFile* mfjj1 = mafOpen(file_jj1, 1);
    struct mafAli* a1 = NULL, * a2 = NULL;

    struct mafAli * new_ali;
    int beg1, end1, beg2, end2, beg, end, col_beg, col_end;
    int test = 0;
    a1 = my_mafNext(mfjj);
    a2 = my_mafNext(mfjj1);
    while (1)
    {
        while (a1 != NULL && (a2 == NULL || a1->components->start + a1->components->size - 1 < a2->components->start)) {
            if (a1->components->size >= MIN_OUTPUT_WID && fpw1 != NULL && (row2 == 0 || a1->components->next != NULL))
            {
                mafWrite(fpw1, a1);
                mafAliFree(&a1);
            }
            //mafWrite(fpw1, a1);//ztzt
            else
                mafAliFree(&a1);
            a1 = my_mafNext(mfjj);
        }
        while (a2 != NULL && (a1 == NULL || a2->components->start + a2->components->size - 1 < a1->components->start)) {
            if (a2->components->size >= MIN_OUTPUT_WID && fpw1 != NULL && (row2 == 0 || a2->components->next != NULL))
            {
                mafWrite(fpw1, a2);
                mafAliFree(&a2);
            }
            else
                mafAliFree(&a2);
            a2 = my_mafNext(mfjj1);
        }
        if (a1 == NULL && a2 == NULL)
            break;
        if (a1 == NULL || a2 == NULL)
            continue;
        if (a1->components->start + a1->components->size - 1 < a2->components->start)
            continue;
        if (a2->components->start + a2->components->size - 1 < a1->components->start)
            continue;

        if (a1->components->start == 11305)
            test++;


        beg1 = a1->components->start;                           // at this point, a1 a2 overlap or
        end1 = a1->components->start + a1->components->size - 1;// cover, print uncovered front part
        beg2 = a2->components->start;                           // pre_yama middle/covered part,
        end2 = a2->components->start + a2->components->size - 1;// then add another ali to process end part

        if (beg1 < beg2&& beg2 - beg1 >= MIN_OUTPUT_WID && fpw1 != NULL) {
            col_beg = mafPos2Col(a1->components, beg1, a1->textSize);
            for (; col_beg > 0 && a1->components->text[col_beg - 1] == '-'; col_beg--)
                ;
            col_end = mafPos2Col(a1->components, beg2 - 1, a1->textSize);
            for (; col_end < a1->textSize - 1 && a1->components->text[col_end + 1] == '-'; col_end++)
                ;
            my_print_part_ali_col(a1, col_beg, col_end, fpw1);
        }
        else if (beg2 < beg1&& beg1 - beg2 >= MIN_OUTPUT_WID && fpw1 != NULL) {
            col_beg = mafPos2Col(a2->components, beg2, a2->textSize);
            for (; col_beg > 0 && a2->components->text[col_beg - 1] == '-'; col_beg--)
                ;
            col_end = mafPos2Col(a2->components, beg1 - 1, a2->textSize);
            for (; col_end < a2->textSize - 1 && a2->components->text[col_end + 1] == '-'; col_end++)
                ;
            my_print_part_ali_col(a2, col_beg, col_end, fpw1);
        }


        beg = beg1 > beg2 ? beg1 : beg2;
        end = end1 < end2 ? end1 : end2;

        if (beg == beg1) { // for gaps in front
            col_beg = mafPos2Col(a1->components, beg1, a1->textSize);
            if (col_beg != 0 && fpw1 != NULL)
            {
                my_print_part_ali_col(a1, 0, col_beg - 1, fpw1);
            }
        }
        if (beg == beg2) { // for gaps in front
            col_beg = mafPos2Col(a2->components, beg2, a2->textSize);
            if (col_beg != 0 && fpw1 != NULL)
            {
                my_print_part_ali_col(a2, 0, col_beg - 1, fpw1);
            }
        }
        new_ali = pre_yama0(a1, a2, beg, end, radius, 1, fpw1, &tmp);
        if (tmp != NULL)
        {
            mafWrite(fpw1, tmp);
            mafAliFree(&tmp);
            tmp = NULL;
        }
        if (new_ali != NULL && new_ali->components->size >= MIN_OUTPUT_WID)
        {
            mafWrite(fpw1, new_ali);
            mafAliFree(&new_ali);
        }
        else
            //mafWrite(stdout, new_ali);//ztzt
            mafAliFree(&new_ali);

        if (end1 < end2)
            a2 = keep_ali(a2, end1 + 1);

        if (end2 < end1)
            a1 = keep_ali(a1, end2 + 1);

        if (end1 <= end2) {
            col_end = mafPos2Col(a1->components, end1, a1->textSize);
            if (col_end < a1->textSize - 1 && fpw1 != NULL)
            {
                my_print_part_ali_col(a1, col_end + 1, a1->textSize - 1, fpw1);
            }
            else
                mafAliFree(&a1);
            a1 = my_mafNext(mfjj);
        }
        if (end2 <= end1) {
            col_end = mafPos2Col(a2->components, end2, a2->textSize);
            if (col_end < a2->textSize - 1 && fpw1 != NULL)
            {
                my_print_part_ali_col(a2, col_end + 1, a2->textSize - 1, fpw1);
            }
            else
                mafAliFree(&a2);
            a2 = my_mafNext(mfjj1);
        }
    }
    
    mfjj->alignments = NULL;
    mafFileFree(&mfjj);
    mfjj1->alignments = NULL;
    mafFileFree(&mfjj1);
    fclose(fpw1);
}

void mul_mafReadAll(const char* path, const char* _maf, int file_i, struct mafAli** cp_list_i)
{
    char file_name[1000];
    sprintf(file_name, "%s%d%s", path, file_i, _maf);
    //std::cout << file_name <<"\n";
    struct mafFile* maf = my_mafReadAll(file_name, 1);
    (*cp_list_i) = maf->alignments;
    maf->alignments = NULL;
    mafFileFree(&maf);
}

void mul_mafRead_sort_writeAll(const char* path, const char* _maf, int file_i)
{
    char file_name[1000];
    char ans_name[1000];
    sprintf(file_name, "%s%d%s", path, file_i, _maf);
    sprintf(ans_name, "%s%d_.maf", path, file_i);
    my_mafRead_sort_writeAll(file_name, ans_name, 1);
}

int multiz_main(int filenum, char* path) 
{
    struct mafAli** cp_list = (struct mafAli**)malloc(sizeof(struct mafAli*) * (filenum+10));
    struct mafFile* maf;
    struct mafAli* tmp = NULL, * tmp1 = NULL;
    int v=1, i, j;
    FILE* fpw1 = NULL;
    char cmd[200];
    char _maf[5] = { '.','m','a','f','\0' };
    int file_i=1;
    char file_name[1000];
    char ans_name[1000];

    sprintf(ans_name, "%ssmall%s", path, _maf);

    sprintf(cmd, "multiz.v%.1f", VERSION);
    argv0 = cmd;

    fpw1 = fopen(ans_name, "w");
    char str[7] = { 'm','u','l','t','i','z', '\0' };
    mafWriteStart(fpw1, str);
    init_scores70();
    
    if (filenum == 1)
    {
        sprintf(file_name, "%s%d%s", path, file_i, _maf);
        maf = mafReadAll(file_name, 1);
        cp_list[1] = maf->alignments;
        maf->alignments = NULL;
        mafFileFree(&maf);

        sprintf(file_name, "%ssmall_sv%s", path, _maf);
        maf = mafReadAll(file_name, 1);
        cp_list[2] = maf->alignments;
        maf->alignments = NULL;
        mafFileFree(&maf);

        tmp = two_list_to_one(&cp_list[1], &cp_list[2], fpw1, fpw1);
        cp_list[1] = cp_list[2] = NULL;

        while (tmp)
        {
            my_mafWrite(fpw1, tmp, 50, 2, 94, 0);
            //mafWrite(fpw1, tmp);
            tmp1 = tmp->next;
            mafAliFree(&tmp);
            tmp = tmp1;
        }

        fclose(fpw1);
        free(cp_list);
        /*
        remove(ans_name);
        if (rename(file_name, ans_name) == 0){
            printf("File renamed successfully\n");
            return 0;
        }
        else {
            printf("Error: unable to rename the file\n");
            return -1;
        } */
        return 0;
    }

    for (file_i = 1; file_i <= filenum+9; file_i++)
        cp_list[file_i] = NULL;

    for (file_i = 1; file_i <= filenum; file_i++)
    {
        sprintf(file_name, "%s%d%s", path, file_i, _maf);
        maf = mafReadAll(file_name, 1);
        cp_list[file_i] = maf->alignments;
        maf->alignments = NULL;
        mafFileFree(&maf);
    }
    file_i = filenum;
    while(cp_list[2]!=NULL)
    {
        i = j = 1;
        //printf("multiz-process:  %d\n", file_i);
        file_i = file_i / 2 + file_i % 2;
        
        while (cp_list[j] != NULL && cp_list[j + 1] != NULL)
        {
            tmp = two_list_to_one(&cp_list[j], &cp_list[j + 1], fpw1, fpw1);
            cp_list[j] = cp_list[j + 1] = NULL;
            cp_list[i] = tmp;
            //printf("%d\t%d\t%d\t%d\t%d\n", cp_list[1], cp_list[2], cp_list[3], cp_list[4], cp_list[5]);
            i = i + 1;
            j = j + 2;
        }
        if (cp_list[j] != NULL)
        {
            cp_list[i] = cp_list[j];
            cp_list[j] = NULL;
            //printf("%d\t%d\t%d\t%d\t%d\n", cp_list[1], cp_list[2], cp_list[3], cp_list[4], cp_list[5]);
        }
        
    }
    

    sprintf(file_name, "%ssmall_sv%s", path, _maf);
    maf = mafReadAll(file_name, 1);
    cp_list[2] = maf->alignments;
    tmp = two_list_to_one(&cp_list[1], &cp_list[2], fpw1, fpw1);
    cp_list[1] = cp_list[2] = NULL;


    while (tmp)
    {
        //my_mafWrite(fpw1, tmp, 30,2,94,0);
        mafWrite(fpw1, tmp);
        tmp1 = tmp->next;
        mafAliFree(&tmp);
        tmp = tmp1;
    }
    //mafWriteEnd(fpw1);
    free(cp_list);
    fclose(fpw1);
    //if (fpw2 != NULL)
       // fclose(fpw2);
    //printf("resule_file: %s\n", ans_name);
    return 0;
}

int multiz_main_sv(int filenum, char* path)
{
    struct mafAli** cp_list = (struct mafAli**)malloc(sizeof(struct mafAli*) * (filenum + 10));
    struct mafFile* maf;
    struct mafAli* tmp = NULL, * tmp1 = NULL;
    int v = 1, i, j;
    FILE* fpw1 = NULL;
    char cmd[200];
    char _maf[8] = { '_', 's', 'v', '.', 'm','a','f','\0' };
    int file_i = 1;
    char file_name[1000];
    char ans_name[1000];

    sprintf(ans_name, "%ssmall%s", path, _maf);

    if (filenum == 1)
    {
        sprintf(file_name, "%s%d%s", path, file_i, _maf);
        remove(ans_name);
        free(cp_list);
        if (rename(file_name, ans_name) == 0) {
            printf("File renamed successfully\n");
            return 0;
        }
        else {
            printf("Error: unable to rename the file\n");
            return -1;
        }
    }

    sprintf(cmd, "multiz.v%.1f", VERSION);
    argv0 = cmd;

    fpw1 = fopen(ans_name, "w");
    char str[7] = { 'm','u','l','t','i','z', '\0' };
    mafWriteStart(fpw1, str);
    init_scores70();

    for (file_i = 1; file_i <= filenum + 9; file_i++)
        cp_list[file_i] = NULL;

    for (file_i = 1; file_i <= filenum; file_i++)
    {
        sprintf(file_name, "%s%d%s", path, file_i, _maf);
        maf = mafReadAll(file_name, 1);
        cp_list[file_i] = maf->alignments;
        maf->alignments = NULL;
        mafFileFree(&maf);
    }
    file_i = filenum;
    while (cp_list[2] != NULL)
    {
        i = j = 1;
        //printf("multiz-process:  %d\n", file_i);
        file_i = file_i / 2 + file_i % 2;

        while (cp_list[j] != NULL && cp_list[j + 1] != NULL)
        {
            tmp = two_list_to_one(&cp_list[j], &cp_list[j + 1], fpw1, fpw1);
            cp_list[j] = cp_list[j + 1] = NULL;
            cp_list[i] = tmp;
            //printf("%d\t%d\t%d\t%d\t%d\n", cp_list[1], cp_list[2], cp_list[3], cp_list[4], cp_list[5]);
            i = i + 1;
            j = j + 2;
        }
        if (cp_list[j] != NULL)
        {
            cp_list[i] = cp_list[j];
            cp_list[j] = NULL;
            //printf("%d\t%d\t%d\t%d\t%d\n", cp_list[1], cp_list[2], cp_list[3], cp_list[4], cp_list[5]);
        }

    }

    tmp = cp_list[1];
    while (tmp)
    {
        mafWrite(fpw1, tmp);
        tmp1 = tmp->next;
        mafAliFree(&tmp);
        tmp = tmp1;
    }
    //mafWriteEnd(fpw1);
    free(cp_list);
    fclose(fpw1);
    //if (fpw2 != NULL)
       // fclose(fpw2);
    //printf("resule_file: %s\n", ans_name);
    return 0;
}

int multiz_main2(int result_maf, char* file_name1, char* file_name2, char* ans_name, int thresh1, int thresh2, int thresh3)
{
    struct mafAli** cp_list = (struct mafAli**)malloc(sizeof(struct mafAli*) * (5));
    struct mafFile* maf;
    struct mafAli* tmp = NULL, * tmp1 = NULL;
    int v = 1, i, j;
    FILE* fpw1 = NULL;
    char cmd[200];
    char _maf[5] = { '.','m','a','f','\0' };
    int file_i = 2;

    sprintf(cmd, "multiz.v%.1f", VERSION);
    argv0 = cmd;

    fpw1 = fopen(ans_name, "w");
    char str[7] = { 'm','u','l','t','i','z', '\0' };
    mafWriteStart(fpw1, str);
    init_scores70();

    for (file_i = 1; file_i <= 4; file_i++)
        cp_list[file_i] = NULL;

    maf = my_mafReadAll(file_name1, 1);
    cp_list[1] = maf->alignments;
    maf->alignments = NULL;
    mafFileFree(&maf);
    maf = my_mafReadAll(file_name2, 1);
    cp_list[2] = maf->alignments;
    maf->alignments = NULL;
    mafFileFree(&maf);
    file_i = 2;
    while (cp_list[2] != NULL)
    {
        i = j = 1;
        //printf("multiz-process:  %d\n", file_i);
        file_i = file_i / 2 + file_i % 2;

        while (cp_list[j] != NULL && cp_list[j + 1] != NULL)
        {
            tmp = two_list_to_one(&cp_list[j], &cp_list[j + 1], fpw1, fpw1);
            cp_list[j] = cp_list[j + 1] = NULL;
            cp_list[i] = tmp;
            //printf("%d\t%d\t%d\t%d\t%d\n", cp_list[1], cp_list[2], cp_list[3], cp_list[4], cp_list[5]);
            i = i + 1;
            j = j + 2;
        }
        if (cp_list[j] != NULL)
        {
            cp_list[i] = cp_list[j];
            cp_list[j] = NULL;
            //printf("%d\t%d\t%d\t%d\t%d\n", cp_list[1], cp_list[2], cp_list[3], cp_list[4], cp_list[5]);
        }

    }

    tmp = cp_list[1];
    if (result_maf)
    {
        while (tmp)
        {
            //my_mafWrite_only(fpw1, tmp, 0,0,0,0);
            my_mafWrite(fpw1, tmp, thresh1, thresh2, thresh3, 0);
            //mafWrite(fpw1, tmp);
            tmp1 = tmp->next;
            mafAliFree(&tmp);
            tmp = tmp1;
        }
    }
    else
    {
        while (tmp)
        {
            my_mafWrite_only(fpw1, tmp, 0,0,0,0);
            //mafWrite(fpw1, tmp);
            tmp1 = tmp->next;
            mafAliFree(&tmp);
            tmp = tmp1;
        }
    }
    //mafWriteEnd(fpw1);
    free(cp_list);
    fclose(fpw1);
    //if (fpw2 != NULL)
       // fclose(fpw2);
    //printf("resule_file: %s\n", ans_name);
    return 0;
}

void multiz_main3(char* file_name1, char* file_name2, char* ans_name, int thresh1, int thresh2, int thresh3)
{
    struct mafAli* tmp = NULL;
    FILE* fpw1 = fopen(ans_name, "w");
    char str[7] = { 'm','u','l','t','i','z', '\0' };
    mafWriteStart(fpw1, str);
    init_scores70();
    //fpw1 = stdout;
    struct mafFile* mfjj = mafOpen(file_name1, 1);
    struct mafFile* mfjj1 = mafOpen(file_name2, 1);
    struct mafAli* a1 = NULL, * a2 = NULL;

    struct mafAli* new_ali;
    int beg1, end1, beg2, end2, beg, end, col_beg, col_end;
    int test = 0;
    a1 = my_mafNext(mfjj);
    a2 = my_mafNext(mfjj1);
    while (1)
    {
        while (a1 != NULL && (a2 == NULL || a1->components->start + a1->components->size - 1 < a2->components->start)) {
            if (a1->components->size >= MIN_OUTPUT_WID && fpw1 != NULL && (row2 == 0 || a1->components->next != NULL))
            {
                my_mafWrite(fpw1, a1, thresh1, thresh2, thresh3, 0);
                mafAliFree(&a1);
            }
            //mafWrite(fpw1, a1);//ztzt
            else
                mafAliFree(&a1);
            a1 = my_mafNext(mfjj);
        }
        while (a2 != NULL && (a1 == NULL || a2->components->start + a2->components->size - 1 < a1->components->start)) {
            if (a2->components->size >= MIN_OUTPUT_WID && fpw1 != NULL && (row2 == 0 || a2->components->next != NULL))
            {
                my_mafWrite(fpw1, a2, thresh1, thresh2, thresh3, 0);
                mafAliFree(&a2);
            }
            else
                mafAliFree(&a2);
            a2 = my_mafNext(mfjj1);
        }
        if (a1 == NULL && a2 == NULL)
            break;
        if (a1 == NULL || a2 == NULL)
            continue;
        if (a1->components->start + a1->components->size - 1 < a2->components->start)
            continue;
        if (a2->components->start + a2->components->size - 1 < a1->components->start)
            continue;

        if (a1->components->start == 11305)
            test++;


        beg1 = a1->components->start;                           // at this point, a1 a2 overlap or
        end1 = a1->components->start + a1->components->size - 1;// cover, print uncovered front part
        beg2 = a2->components->start;                           // pre_yama middle/covered part,
        end2 = a2->components->start + a2->components->size - 1;// then add another ali to process end part

        if (beg1 < beg2&& beg2 - beg1 >= MIN_OUTPUT_WID && fpw1 != NULL) {
            col_beg = mafPos2Col(a1->components, beg1, a1->textSize);
            for (; col_beg > 0 && a1->components->text[col_beg - 1] == '-'; col_beg--)
                ;
            col_end = mafPos2Col(a1->components, beg2 - 1, a1->textSize);
            for (; col_end < a1->textSize - 1 && a1->components->text[col_end + 1] == '-'; col_end++)
                ;
            my_print_part_ali_col_(a1, col_beg, col_end, fpw1, thresh1, thresh2, thresh3);
        }
        else if (beg2 < beg1&& beg1 - beg2 >= MIN_OUTPUT_WID && fpw1 != NULL) {
            col_beg = mafPos2Col(a2->components, beg2, a2->textSize);
            for (; col_beg > 0 && a2->components->text[col_beg - 1] == '-'; col_beg--)
                ;
            col_end = mafPos2Col(a2->components, beg1 - 1, a2->textSize);
            for (; col_end < a2->textSize - 1 && a2->components->text[col_end + 1] == '-'; col_end++)
                ;
            my_print_part_ali_col_(a2, col_beg, col_end, fpw1, thresh1, thresh2, thresh3);
        }


        beg = beg1 > beg2 ? beg1 : beg2;
        end = end1 < end2 ? end1 : end2;

        if (beg == beg1) { // for gaps in front
            col_beg = mafPos2Col(a1->components, beg1, a1->textSize);
            if (col_beg != 0 && fpw1 != NULL)
            {
                my_print_part_ali_col_(a1, 0, col_beg - 1, fpw1, thresh1, thresh2, thresh3);
            }
        }
        if (beg == beg2) { // for gaps in front
            col_beg = mafPos2Col(a2->components, beg2, a2->textSize);
            if (col_beg != 0 && fpw1 != NULL)
            {
                my_print_part_ali_col_(a2, 0, col_beg - 1, fpw1, thresh1, thresh2, thresh3);
            }
        }
        new_ali = pre_yama0(a1, a2, beg, end, radius, 1, fpw1, &tmp);

        if (tmp != NULL)
        {
            my_mafWrite(fpw1, tmp, thresh1, thresh2, thresh3, 0);
            mafAliFree(&tmp);
            tmp = NULL;
        }

        if (new_ali != NULL && new_ali->components->size >= MIN_OUTPUT_WID)
        {
            my_mafWrite(fpw1, new_ali, thresh1, thresh2, thresh3, 0);
            mafAliFree(&new_ali);
        }
        else
            //mafWrite(stdout, new_ali);//ztzt
            mafAliFree(&new_ali);

        if (end1 < end2)
            a2 = keep_ali(a2, end1 + 1);

        if (end2 < end1)
            a1 = keep_ali(a1, end2 + 1);

        if (end1 <= end2) {
            col_end = mafPos2Col(a1->components, end1, a1->textSize);
            if (col_end < a1->textSize - 1 && fpw1 != NULL)
            {
                my_print_part_ali_col_(a1, col_end + 1, a1->textSize - 1, fpw1, thresh1, thresh2, thresh3);
            }
            else
                mafAliFree(&a1);
            a1 = my_mafNext(mfjj);
        }
        if (end2 <= end1) {
            col_end = mafPos2Col(a2->components, end2, a2->textSize);
            if (col_end < a2->textSize - 1 && fpw1 != NULL)
            {
                my_print_part_ali_col_(a2, col_end + 1, a2->textSize - 1, fpw1, thresh1, thresh2, thresh3);
            }
            else
                mafAliFree(&a2);
            a2 = my_mafNext(mfjj1);
        }
    }

    mfjj->alignments = NULL;
    mafFileFree(&mfjj);
    mfjj1->alignments = NULL;
    mafFileFree(&mfjj1);
    fclose(fpw1);
    return ;
}
