# Halign-G: A rapid and low-memory multiple-genome aligner for thousands of human genomes
Halign-G is a tool written in C++ for aligning multiple gemones. It runs on Linux and Windows.

## Usage
```
halign-g Input_file Output_file [-r/--reference val] [-p/--threads val] [-bwt/--bwt val] [-dc/--divide_conquer val] [-sv/--svLen val] [-h/--help]
        Input_file      :  Input file/folder path[Please use .fasta as the file suffix or a forder]
        Output_file     :  Output file path[Please use .maf or .fasta as the file suffix]
        -r/--reference            : The reference sequence name [Please delete all whitespace] (defualt value:[Longest])
        -p/--threads              : The number of threads (defualt value:1)
        -bwt/--bwt                : The global BWT threshold (defualt value:15)
        -dc/--divide_conquer      : The divide & conquer Kband threshold (defualt value:10000)
        -sv/--svLen               : The structure variation length threshold (defualt value:200)
        -h/--help                 : Show this help message (defualt value:false)
```

## Installation
### Linux/WSL (Windows Subsystem for Linux) - from Anaconda
1.Install WSL for Windows. Instructional video [1](https://www.youtube.com/watch?v=X-DHaQLrBi8&t=5s) or [2](http://lab.malab.cn/%7Etfr/1.mp4) (Copyright belongs to the original work).

2.Download and install Anaconda. Download Anaconda for different systems [here](https://www.anaconda.com/products/distribution#Downloads). Instructional video of anaconda installation [1](https://www.youtube.com/watch?v=AshsPB3KT-E) or [2](http://lab.malab.cn/%7Etfr/Install_anaconda_in_Linux.mp4) (Copyright belongs to the original work).

3.Install Halign-G.
```bash
#1 Create and activate a conda environment for TPMA
conda create -n haligng_env
conda activate haligng_env

#2 Add channels to conda
conda config --add channels malab
conda config --add channels conda-forge

#3 Install Halign-G
conda install -c malab -c conda-forge halign-g

#4 Test Halign-G
halign-g -h
```

### Linux/WSL(Windows Subsystem for Linux) - from the source code

1. Download and Compile the source code. (Make sure your version of gcc >= 9.4.0 or clang >= 13.0.0)
```bash
#1 Download
git clone https://github.com/malabz/Halign-G.git

#2 Open the folder
cd Halign-G

#3 Compile
make -j16

#4 Test
./halign-g -h
```

### Windows - from Visual Studio 2022

1. First, download [zip](https://github.com/malabz/Halign-G/archive/refs/heads/main.zip) and use `Bandizip`, `WinRAR` or any other archive manager software to extract this file.

2. Make programs from `Visual Studio 2022`:
- First of all, download and install [`Visual Studio 2022`](https://visualstudio.microsoft.com/vs/).
- Open `halign-g.sln` in `Visual Studio 2022`, then choose `Build` -> `Build Solution`.
- Open `x64\Debug` folder, you will find `halign-g.exe`.
  - Note: If you want to release this solution, please switch it to `Release` mode. Select `Properties`, choose `Configuration Properties`, then press `Configuration Manager...` button, change `Active Solution configuration` to `Release`. You can find `halign-g.exe` in `x64\Release` folder.

Or, install via `conda` and simple test:
```powershell
conda install -c malab -c conda-forge halign-g
halign-g.exe -h
```



# Halign-G tools in `tools` folder
|tool id|file name|description|
|:-:|:-:|:-:|
|0|time_mem.py|Calculate the maximum time and memory, can process multiple threads/processes|
|1|quN.cpp|Remove the degenerate base|
|2|dotplot.py|Generate the alignment graph for two sequences in `maf` and `fasta` format file|
|3|maf-tongji.py|Calculate the M-score for multiple genome alignment in `MAF` format file|
|4|fasta-tongji.cpp|Calculate the M-score for multiple sequence alignment in `fasta` format file|
|5|maf-tree/|Generate the M-score similarity matrix for `maf` format|
|6|jhtree.py|Generate and draw evolutionary tree based on similarity matrix|
|7|simulate.py|Generate genome sequences with structural variations. The reference file is `sv.maf` |
|8|r.py|Remove all `\r` in files in a specific folder (`Catctus` can not process `\r`)|
|9|xmfa2maf.cpp|Convert `xmfa` format file to `maf` format file|
|10|delta2maf.py|Convert `delta` format from `MUMmer` to `maf` format file |
|11|split-maf/|Convert blocks in `maf` format with multiple sequences to two sequences, which has only one center sequence and one non-center sequence|
|12|maf-filter/|Filter the `maf` format file with the sequence in block, the length of block and the continuity of block|
|13|ref-sort-maf/|Resort the `maf` format file by a specific sequence in `block`|
|14|shiyan1.py|Generate the graph of Lab 1|
|15|shiyan2.py|Generate the graph of Lab 2|
|16|shiyan4.py|Generate the graph of Lab 4|
|17|cov100w.py|Generate M-score histogram using 100w SARS-CoV-2 sequences|
|18|fast_read_file/|A fast read `txt` file module without reading file to memory|


## Contacts
The software tools are developed and maintained by [ZOU's lab](http://lab.malab.cn/~zq/en/index.html).

If you find any bug, welcome to contact us on the [issues page](https://github.com/malabz/halign-g/issues) or [email us](mailto:zhoutong_uestc@163.com).

More tools and infomation can visit our [github](https://github.com/malabz).
