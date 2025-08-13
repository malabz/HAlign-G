#ifndef SEQUENCE_H
#define SEQUENCE_H

#include <stdexcept>
#include <string>
#include <vector>
#include <map>
#include <fstream>

#ifdef _WIN32
#include <Windows.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

extern std::map<unsigned char, unsigned char> dick;



class Sequence {
    using value_type = unsigned char; // 序列中元素的类型
public:
    explicit Sequence(const std::string& filename)
        : file_map_view_(nullptr), file_size_(0), data(nullptr), now(0), is_modified_(false) {
#ifdef _WIN32
        // 将 ANSI 编码的字符串转换为 Unicode 编码的字符串
        int wstr_size = MultiByteToWideChar(CP_ACP, 0, filename.c_str(), -1, nullptr, 0);
        std::vector<wchar_t> wstr(wstr_size);
        MultiByteToWideChar(CP_ACP, 0, filename.c_str(), -1, wstr.data(), wstr_size);

        file_handle_ = CreateFileW(wstr.data(), GENERIC_READ | GENERIC_WRITE, 0,
            nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
            nullptr);
        if (file_handle_ == INVALID_HANDLE_VALUE) {
            throw std::runtime_error("Failed to open file.");
        }
        file_size_ = GetFileSize(file_handle_, nullptr);

        file_map_handle_ = CreateFileMappingW(file_handle_, nullptr, PAGE_READWRITE, 0, 0, nullptr);
        if (file_map_handle_ == nullptr) {
            CloseHandle(file_handle_);
            throw std::runtime_error("Failed to create file mapping.");
        }

        file_map_view_ = MapViewOfFile(file_map_handle_, FILE_MAP_WRITE, 0, 0, 0);
        if (file_map_view_ == nullptr) {
            CloseHandle(file_map_handle_);
            CloseHandle(file_handle_);
            throw std::runtime_error("Failed to map view of file.");
        }


#else
        // 在 Linux 中直接使用 std::string 类型的字符串
        file_handle_ = open(filename.c_str(), O_RDWR);
        if (file_handle_ == -1) {
            throw std::runtime_error("Failed to open file.");
        }

        struct stat sb;
        if (fstat(file_handle_, &sb) == -1) {
            close(file_handle_);
            throw std::runtime_error("Failed to get file size.");
        }

        file_size_ = static_cast<size_t>(sb.st_size);
        file_map_view_ = mmap(nullptr, file_size_, PROT_READ | PROT_WRITE, MAP_SHARED, file_handle_, 0);
        if (file_map_view_ == MAP_FAILED) {
            close(file_handle_);
            throw std::runtime_error("Failed to map view of file.");
        }
#endif
        data = static_cast<value_type*>(file_map_view_);
    }

    ~Sequence() {
        if (is_modified_) {
#ifdef _WIN32
            FlushViewOfFile(file_map_view_, 0);
#else
            msync(file_map_view_, file_size_, MS_SYNC);
#endif
        }

#ifdef _WIN32
        UnmapViewOfFile(file_map_view_);
        CloseHandle(file_map_handle_);
        CloseHandle(file_handle_);
#else
        munmap(file_map_view_, file_size_);
        close(file_handle_);
#endif
        }
    void refresh() {
        if (is_modified_) {
#ifdef _WIN32
            FlushViewOfFile(file_map_view_, 0);
#else
            msync(file_map_view_, file_size_, MS_SYNC);
#endif
            is_modified_ = false; // 刷新后将标记重置为 false
        }
    }

    size_t size() const {
        return file_size_ / sizeof(value_type);
    }

    const value_type& operator[](size_t index) {
        if (index >= size()) {
            throw std::out_of_range("Index out of range.");
        }
        //std::cout << size() << "  size\n";
        now = index + 1;
        return data[index];
    }
    
    inline unsigned char getStr(size_t i)
    {
        now = i + 1;
        return dick[data[i]];
    }

    inline void set_begin(size_t begin)
    {
        now = begin;
    }

    inline unsigned char next()
    {
        return dick[data[now++]];
    }

    void get100(std::vector<unsigned char>& seq100)
    {
        for (int j = 0; j < 100; j++)
            seq100[j] = dick[data[now + j]];
        now = now + 100;
    }

    void get_str(char* seq1w, size_t start, size_t end)
    {
        for (size_t j = 0; j < (end-start); j++)
            seq1w[j] = (char)data[start+j];
        seq1w[end - start] = '\0';
        now = end;
    }

    void get1w(char* seq1w, size_t len)
    {
        for (int j = 0; j < len; j++)
            seq1w[j] = data[now + j];
        now = now + len;
    }
    void get_put(char* t10000, std::ofstream& os, size_t begin, size_t length)
    {
        now = begin;
        while (true)
        {
            if (length > 10000)
            {
                this->get1w(t10000, 10000);
                os.write(t10000, 10000);
                length -= 10000;
            }
            else
            {
                this->get1w(t10000, length);
                os.write(t10000, length);
                break;
            }
        }
    }
    void replace_character(size_t position, value_type new_char) {
        if (position >= size()) {
            throw std::out_of_range("Position out of range.");
    }

        data[position] = new_char;
        is_modified_ = true;
}

    size_t now;
private:
    value_type* data;

#ifdef WIN32
    void* file_map_view_; // 内存映射文件的指针
    size_t file_size_; // 文件大小
    HANDLE file_map_handle_; // 文件映射的句柄
    HANDLE file_handle_; // 文件句柄
#else
    int file_handle_; // 文件描述符
    void* file_map_view_; // 内存映射文件的指针
    size_t file_size_; // 文件大小
#endif
    bool is_modified_; // 标记文件是否被修改过

    // 禁止复制和赋值
    Sequence(const Sequence&) = delete;
    Sequence& operator=(const Sequence&) = delete;
};


#endif // SEQUENCE_H
