#include "multi.hpp"

ThreadPool* threadPool0 = NULL;
ThreadPool* threadPool1 = NULL;
ThreadPool* threadPool2 = NULL;


int mul_main_sv(int filenum, char* path)
{
    int v = 1, i, j;
    char cmd[200];
    char _maf[8] = { '_', 's', 'v', '.', 'm','a','f','\0' };
    int file_i = 1;
    char file_name[1000];
    char ans_name[1000];
    int* cp_list = (int*)malloc(sizeof(int) * (filenum + 3));
    cp_list[0] = cp_list[filenum + 1] = cp_list[filenum + 2] = 0;
    sprintf(ans_name, "%ssmall%s", path, _maf);
    sprintf(cmd, "multiz.v%.1f", VERSION);
    argv0 = cmd;
    init_scores70();
    if (filenum == 1)
    {
        free(cp_list);
        cp_list = NULL;
        sprintf(file_name, "%s%d%s", path, file_i, _maf);
        my_mafRead_sort_writeAll(file_name, ans_name, 1);
        //remove(file_name);
        return 0;
    }

    //并行读取maf文件
    for (file_i = 1; file_i <= filenum; file_i++)
    {
        threadPool2->execute(mul_mafRead_sort_writeAll, path, _maf, file_i);
        cp_list[file_i] = file_i;
    }
    threadPool2->waitFinished();

    file_i = filenum;
    while (cp_list[2])
    {
        i = j = 1;
        std::cout << "                    | Info : multiz process i_sv.maf: " << file_i << "\n";
        file_i = file_i / 2 + file_i % 2;
        //printf("@%d %d %d %d %d %d\n", cp_list[1], cp_list[2], cp_list[3], cp_list[4], cp_list[5], cp_list[6]);
        while (cp_list[j] && cp_list[j + 1])
        {
            //mul_two_list_to_one(&tmp_list[i], &cp_list[j], &cp_list[j + 1], fpw1, fpw1);
            threadPool2->execute(mul_two_file_to_one, path, i, j, j + 1);
            cp_list[j] = cp_list[j + 1] = 0;
            i = i + 1;
            j = j + 2;
        }
        threadPool2->waitFinished();
        if (cp_list[j] && i != j)
        {
            cp_list[i] = cp_list[j];
            cp_list[j] = 0;
            sprintf(file_name, "%s%d_.maf", path, j);
            sprintf(ans_name, "%s%d_.maf", path, i);
            remove(ans_name);
            rename(file_name, ans_name);
        }
        i = i - 1;
        for (; i >= 1; i--)
        {
            cp_list[i] = i;
            sprintf(file_name, "%s%d_.tmp", path, i);
            sprintf(ans_name, "%s%d_.maf", path, i);
            remove(ans_name);
            rename(file_name, ans_name);
        }
        //printf("@%d %d %d %d %d %d\n\n", cp_list[1], cp_list[2], cp_list[3], cp_list[4], cp_list[5], cp_list[6]);

    }
    sprintf(file_name, "%s1_.maf", path);
    sprintf(ans_name, "%ssmall_sv.maf", path);
    remove(ans_name);
    rename(file_name, ans_name);

    free(cp_list);
    cp_list = NULL;
    return 0;
}

int mul_main(int filenum, char* path)
{
    int v = 1, i, j;
    char cmd[200];
    int file_i = 1;
    char file_name1[1000];
    char file_name2[1000];
    char ans_name[1000];
    int* cp_list = (int*)malloc(sizeof(int) * (filenum + 3));
    cp_list[0] = cp_list[filenum + 1] = cp_list[filenum + 2] = 0;
    char _maf[5] = { '.','m','a','f','\0' };
    sprintf(ans_name, "%ssmall%s", path, _maf);

    sprintf(cmd, "multiz.v%.1f", VERSION);
    argv0 = cmd;
    //init_scores70();

    if (filenum == 1)
    {
        free(cp_list);
        cp_list = NULL;
        sprintf(file_name1, "%s%d%s", path, file_i, _maf);
        my_mafRead_sort_writeAll(file_name1, ans_name, 1);

        return 0;
    }
    char file[1000];
    char ans[1000];
    struct mafFile* filefp;
    FILE* ansfp;
    //并行读取maf文件
    for (file_i = 1; file_i <= filenum; file_i++)
    {
        {
            std::lock_guard<std::mutex> lock(threadPool2->mutex_fp); // 在申请file-number时加锁
            sprintf(file, "%s%d%s", path, file_i, _maf);
            sprintf(ans, "%s%d_.maf", path, file_i);

            filefp = mafOpen(file, 1);
            ansfp = fopen(ans, "w");
        }
        threadPool2->execute(my_mafRead_sort_writeAll_p, filefp, ansfp);
        cp_list[file_i] = file_i;
    }
    threadPool2->waitFinished();

    char file_ii[1000];
    char file_jj[1000];
    char file_jj1[1000];
    FILE* filepf;
    struct mafFile* mfjj;
    struct mafFile* mfjj1;
    file_i = filenum;
    while (cp_list[2])
    {
        i = j = 1;
        std::cout << "                    | Info : multiz process i.maf: " << file_i << "\n";
        file_i = file_i / 2 + file_i % 2;
        //printf("@%d %d %d %d %d %d\n", cp_list[1], cp_list[2], cp_list[3], cp_list[4], cp_list[5], cp_list[6]);
        while (cp_list[j] && cp_list[j + 1])
        {
            //mul_two_list_to_one(&tmp_list[i], &cp_list[j], &cp_list[j + 1], fpw1, fpw1);
            {
                std::lock_guard<std::mutex> lock(threadPool2->mutex_fp);
                sprintf(file_ii, "%s%d_.tmp", path, i);
                sprintf(file_jj, "%s%d_.maf", path, j);
                sprintf(file_jj1, "%s%d_.maf", path, j + 1);
                filepf = fopen(file_ii, "w");
                mfjj = mafOpen(file_jj, 1);
                mfjj1 = mafOpen(file_jj1, 1);
            }
            threadPool2->execute(mul_two_file_to_one_p, filepf, mfjj, mfjj1);
            cp_list[j] = cp_list[j + 1] = 0;
            i = i + 1;
            j = j + 2;
        }
        threadPool2->waitFinished();
        if (cp_list[j] && i != j)
        {
            cp_list[i] = cp_list[j];
            cp_list[j] = 0;
            sprintf(file_name1, "%s%d_.maf", path, j);
            sprintf(ans_name, "%s%d_.maf", path, i);
            remove(ans_name);
            rename(file_name1, ans_name);
        }
        i = i - 1;
        for (; i >= 1; i--)
        {
            cp_list[i] = i;
            sprintf(file_name1, "%s%d_.tmp", path, i);
            sprintf(ans_name, "%s%d_.maf", path, i);
            remove(ans_name);
            rename(file_name1, ans_name);
        }
        //printf("@%d %d %d %d %d %d\n\n", cp_list[1], cp_list[2], cp_list[3], cp_list[4], cp_list[5], cp_list[6]);

    }

    sprintf(file_name1, "%s1_.maf", path);
    sprintf(ans_name, "%ssmall%s", path, _maf);
    remove(ans_name);
    rename(file_name1, ans_name);

    for (file_i = 1; file_i <= filenum; file_i++)
    {
        sprintf(file_name1, "%s%d_.maf", path, file_i);
        remove(file_name1);
        sprintf(file_name1, "%s%d_.tmp", path, file_i);
        remove(file_name1);
        sprintf(file_name1, "%s%d.tmp", path, file_i);
        remove(file_name1);
    }

    free(cp_list);
    cp_list = NULL;
    return 0;
}

int my_mul_main(char* path, int filenum,int chr, int chunk)
{
    ThreadPool* thread2 = new ThreadPool(8);
    std::string ref = "";
    int v = 1;
    char cmd[200];
    int file_i = 1;
    char ans_name[1000];
    char tmp_name[1000];
    struct mafAli** maf_array = (struct mafAli**)malloc((filenum + 1) * sizeof(struct mafAli*));
    // 初始化所有指针为NULL
    for (int i = 0; i < filenum + 1; i++) {
        maf_array[i] = NULL;
    }
    struct mafAli* tmp;
    char _maf[5] = { '.','m','a','f','\0' };
    sprintf(tmp_name, "%s%d_%s", path, chr, _maf);
    sprintf(ans_name, "%s%d%s", path, chr, _maf);
    //init_scores70();
    sprintf(cmd, "multiz.v%.1f", VERSION);
    argv0 = cmd;
   
    char file[1000];
    struct mafFile* filefp;
    //并行读取maf文件
    for (file_i = 1; file_i <= filenum; file_i++)
    {
        {
            std::lock_guard<std::mutex> lock2(thread2->mutex_fp); // 在申请file-number时加锁
            sprintf(file, "%smaf/%d/%d%s", path, file_i, chr, _maf);
            filefp = mafOpen(file, 1);
        }
        thread2->execute(my_mafReadAll_p, filefp, &(maf_array[file_i]));
        //my_mafReadAll_p(filefp, &(maf_array[file_i]));
    }

    thread2->waitFinished();
    //std::cout << "1  " << getPeakRSS()/1024 << " kB\n";//输出内存耗费
    //合并链表到 all-> maf_array[0]
    file_i = 1;
    tmp = NULL;
    while (file_i <= filenum)
    {
        if (maf_array[file_i] != NULL)
        {
            maf_array[0] = maf_array[file_i++];
            tmp = maf_array[0];
            break;
        }
        file_i++;
    }
    for (; file_i <= filenum; file_i++)
    {
        if (maf_array[file_i] != NULL)
        {
            while (tmp->next != NULL)
                tmp = tmp->next;
            tmp->next = maf_array[file_i];
            tmp = maf_array[file_i];
        }
    }
    for (int i = 1; i < filenum + 1; i++)
        maf_array[i] = NULL;
    //排序链表 all-> maf_array[0]
    //std::cout << "2  " << getPeakRSS() / 1024  << " kB\n";//输出内存耗费
    maf_array[0] = mergeSort(maf_array[0]);
    if (maf_array[0] != NULL)
        ref = maf_array[0]->components->src;
   // std::cout << "3  " << getPeakRSS() / 1024 << " kB\n";//输出内存耗费
    //切割 所有位点，然后排序
    
    Stream::cut_maf(maf_array[0], tmp_name, ref);
    //std::cout << "4  " << getPeakRSS() / 1024  << " kB\n";//输出内存耗费
   
    //外部分块合并
    sort_maf_file(tmp_name, chunk);
     //合并
    std::string s1 = tmp_name;
    std::string s2 = ans_name;
    Stream::sort_hebing(s1, s2, ref);
    deleteFile(tmp_name);
   // std::cout << "5  " << getPeakRSS() / 1024  << " kB\n";//输出内存耗费
    delete thread2;
    //释放内存
    free(maf_array);
    return 0;
}
