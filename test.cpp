/*------------------------------------------------------------------------
	=> File Name: main.cpp
	=> Author: caoyuchao
	=> Mail: cycbhbjxd@gmail.com 
	=> Created Time: 2018年03月17日 星期六 13时20分18秒
----------------------------------------------------------------------*/

#include<iostream>
#include"sysfile.h"
#include<stdio.h>
#include<fcntl.h>
#include<unistd.h>

extern FILE* disk;
extern super_block sublock;
extern const char* diskname;
extern std::vector<file_item> file_table;
extern std::string current_path;
extern unsigned short cur_inum;
void init()
{
    int fd;
    if((fd=open(diskname,O_RDONLY,0))<0)
    {
        format_disk();
    }
    else
    {
        close(fd);
    }
    
    if((disk=fopen(diskname,"rb+"))==NULL)
    {
        printf("file.dat can not open\n");
        exit(0);
    }
    fseek(disk,BLOCK_SIZE,SEEK_SET);
    fread(&sublock,sizeof(sublock),1,disk);
}

int main()
{
    init();
    //sublock.dbitmap[0]=0xffffffff;
    //sublock.dbitmap[1]=0xffffffff;
    //setdbitmap(disk,&sublock,6,0);
    int tmp1=find_inode_bypath("../././..",2);
    std::cout<<tmp1<<std::endl;
    int tmp2=find_inode_bypath("./../../../../.",2);
    std::cout<<tmp2<<std::endl;

    unsigned short num=get_free_block();
    std::cout<<num<<std::endl;
    //setdbitmap(disk,&sublock,6,1);
    //setdbitmap(disk,&sublock,37,0);
    num=get_free_block();
    std::cout<<num<<std::endl;
    fclose(disk);
    //std::cout<<sizeof(i_node)<<std::endl;
    //std::cout<<sizeof(time_t)<<std::endl;
    std::cout<<sizeof(directory)<<std::endl;
    //std::cout<<sizeof(file_item)<<std::endl;
    return 0;
}
