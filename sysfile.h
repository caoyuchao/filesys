/*------------------------------------------------------------------------
	=> File Name: sysfile.h
	=> Author: caoyuchao
	=> Mail: cycbhbjxd@gmail.com 
	=> Created Time: 2018年03月17日 星期六 12时54分13秒
----------------------------------------------------------------------*/
#pragma once
#include<time.h>
#include<stdio.h>
#include<vector>
#include<string.h>
#include<string>
#define BLOCK_SIZE 512
#define BLOCK_NUM 2048
#define BLOCK_INODE_NUM 6
#define BITMAP_NUM 64

#define GetINodeOffSet(num) (BLOCK_SIZE*2+64*((num)-2))
#define GetDBlcokOffSet(num) (BLOCK_SIZE*(num))


#define DIREC 1
#define DOCUM 0

#define O_READ 0x1
#define O_WRITE 0x2
#define O_RD_WR (O_READ|O_WRITE)



struct load_block
{

};


struct super_block
{
    int ibitmap;
    int dbitmap[BITMAP_NUM];
};


struct i_node
{
    unsigned short use_count;
    unsigned short i_type;
    unsigned short i_mode;
    unsigned int i_length;
    time_t time;
    unsigned short i_addr[8];
    char bubble[24];
};

struct file_item
{
  unsigned short o_mode;
  unsigned short i_num;
  unsigned short use_count;
  unsigned short valid;//i_node still exists
  //unsigned short f_type
};

struct directory
{
    unsigned short i_num;
    char file_name[30];
};



void format_disk();
unsigned short get_free_block();
unsigned short get_free_inode();
void setdbitmap(unsigned short num,int flag);
void setibitmap(unsigned short num,int flag);
void copybuf(char* buf,const char* buffer,int n);
int openf(const char* path,unsigned short o_mode);
unsigned short find_inode_bypath(const char* cur_path,unsigned short i_num);
int is_open(unsigned short i_num,unsigned short o_mode);
unsigned int readf(int fd,void* buf,unsigned int count);
unsigned int writef(int fd,const void* buf,unsigned int count);
int closef(int fd);
int mkdir(const char* path);
int sys_create(unsigned short i_num,const char* filename,unsigned short i_type);
int create(const char* path,unsigned short i_type);
int touch(const char* path);
int remove(const char* path);
int sys_remove(unsigned short i_num);
