import os

def remove_carriage_return(folder_path):
    for subdir, dirs, files in os.walk(folder_path):
        for file_name in files:
            file_path = os.path.join(subdir, file_name)
            try:
                with open(file_path, 'r') as f:
                    content = f.read()
                new_content = content.replace('\r', '')
                with open(file_path, 'w') as f:
                    f.write(new_content)
                print(f"Processed file: {file_name}")
            except:
                pass


# 输入文件夹路径
folder_path = "/mnt/sda/zhoutong0/simulate"
remove_carriage_return(folder_path)
