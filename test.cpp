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
#include<string.h>
#include<string>
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

std::string& trim(std::string& s)
{
    if(s.empty())
        return s;
    s.erase(0,s.find_first_not_of(' '));
    s.erase(s.find_last_not_of(' ')+1);
    return s;
}

int is_rm_valid(const char* path)
{
    std::string real_path=parse_path(current_path,path);
    if(real_path.find(current_path)<real_path.size())
        return -1;
    return 0;
}

int main()
{
    init();
    std::string cmd;
    std::string arg;
    system("clear");
    while(true)
    {
        cmd="";
        arg="";
        std::cout<<current_path<<"$";
        std::getline(std::cin,cmd);
        trim(cmd);
        int pos;
        if((pos=cmd.find_first_of(' '))!=std::string::npos)
        {
            arg=cmd.substr(pos+1,cmd.size());
            trim(arg);
            cmd=cmd.substr(0,pos);
        }

        if(cmd=="ls")
        {
            if(arg[arg.size()-1]=='/')
            {
                arg=arg.substr(0,arg.size()-1);
            }
            //std::cout<<"arg "<<arg<<" "<<arg.size()<<std::endl;
            if(arg=="")
            {
                arg=current_path;
                //std::cout<<current_path<<" "<<current_path.size()<<std::endl;
            }
            int i_num=find_inode(arg.c_str());
            if(i_num)
            {
                if(is_direc(i_num))
                {
                    std::vector<std::string> list=lsdir(i_num);
                    for(int i=0;i<list.size();i++)
                    {
                        std::cout<<list[i]<<"   ";
                    }
                    std::cout<<std::endl;
                }
                else
                {
                    std::cout<<"need a directory"<<std::endl;
                }
            }
            else
            {
                std::cout<<"check your path"<<std::endl;
            }
        }
        else if(cmd=="cd")
        {
            if(arg[arg.size()-1]=='/')
            {
                arg=arg.substr(0,arg.size()-1);
            }
            int i_num=find_inode(arg.c_str());
            //std::cout<<"cur_inum "<<i_num<<std::endl;
            if(i_num)
            {
                //std::cout<<current_path<<" "<<arg<<std::endl;
                current_path=parse_path(current_path,arg);
                cur_inum=i_num;
            }
            else
            {
                std::cout<<"check your path"<<std::endl;
            }

        }
        else if(cmd=="mkdir")
        {
            std::cout<<"mkdir "<<arg<<std::endl;

        }
        else if(cmd=="touch")
        {
            std::cout<<"touch "<<arg<<std::endl;

        }
        else if(cmd=="echo")
        {
            std::cout<<"write "<<arg<<std::endl;

        }
        else if(cmd=="cat")
        {
            std::cout<<"read "<<arg<<std::endl;

        }
        else if(cmd=="rm")
        {
            std::cout<<"remove "<<arg<<std::endl;

        
        }
        else if(cmd=="clear")
        {
            system("clear");
        }
        else if(cmd=="exit")
        {
            break;
        }
        else
        {
            std::cout<<"error"<<std::endl;

        }
    }
    //int state=touch("/hello");
    //std::cout<<"state "<<state<<std::endl;
    //int fd=openf("/hello",O_RD_WR);
    //std::cout<<"fd "<<fd<<std::endl;
    //const char* str="hello world\n";
    //int state=writef(fd,str,strlen(str));
    //std::cout<<"state "<<state<<std::endl;

    //int state=mkdir("/home");
    //std::cout<<"state "<<state<<std::endl;
    //state=mkdir("./home/caoyuchao");
    //std::cout<<"state "<<state<<std::endl;
    //state=touch("./home/caoyuchao/hello");
    //int fd=openf("/home/caoyuchao/hello",O_RD_WR);
    //std::cout<<"fd "<<std::endl;
    //const char* str="hello world";
    //state=writef(fd,str,strlen(str));
    //std::cout<<"write state "<<state<<std::endl;
    //char buf[20];
    //memset(buf,0,sizeof(char)*20);
    //state=readf(fd,buf,strlen(str));
    //std::cout<<"buf:"<<buf<<std::endl;
    //std::cout<<"read state "<<state<<std::endl;
    //state=remove("/home/caoyuchao");
    //std::cout<<"state "<<state<<std::endl;
    //fd=openf("/home/caoyuchao/hello",O_RDONLY);
    //std::cout<<"fd "<<fd<<std::endl;
    //std::string tmp;
    //tmp=parse_path("/file/hello/world","../../../.././../file/olleh/dlrow/ekil/.././././..");
    //std::cout<<tmp<<std::endl;
    //sublock.dbitmap[0]=0xffffffff;
    //sublock.dbitmap[1]=0xffffffff;
    //setdbitmap(disk,&sublock,6,0);
    //int tmp1=find_inode_bypath("../././..",2);
    //std::cout<<tmp1<<std::endl;
    //int tmp2=find_inode_bypath("./../../../../.",2);
    //std::cout<<tmp2<<std::endl;

    //unsigned short num=get_free_block();
    //std::cout<<num<<std::endl;
    //setdbitmap(disk,&sublock,6,1);
    //setdbitmap(disk,&sublock,37,0);
    //num=get_free_block();
    //std::cout<<num<<std::endl;
    fclose(disk);
    //std::cout<<sizeof(i_node)<<std::endl;
    //std::cout<<sizeof(time_t)<<std::endl;
    //std::cout<<sizeof(directory)<<std::endl;
    //std::cout<<sizeof(file_item)<<std::endl;
    return 0;
}