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
#include<ctime>
#include<iomanip>
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


void show_fileinfo(file_info info)
{
    std::string format_str;
    format_str.push_back(info.i_type==DIREC?'d':'-');
    format_str.push_back(info.i_mode&O_READ?'r':'-');
    format_str.push_back(info.i_mode&O_WRITE?'w':'-');
    format_str.push_back('-');

    std::cout<<format_str;
    std::cout<<" "<<std::left<<std::setw(6)<<info.i_length;
    char buf[20];
    strftime(buf,sizeof(buf),"%Y-%m-%d %H:%M:%S",localtime(&info.time));
    std::cout<<" "<<std::right<<std::setw(22)<<buf;
    if(info.i_type==DIREC)
        std::cout<<" "<<"\033[34m\033[1m"<<info.file_name<<"\033[0m"<<std::endl;
    else
        std::cout<<" "<<info.file_name<<std::endl;
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

        std::cout<<"\033[32m\033[1mcaoyuchao@cj\033[0m:"<<"\033[34m\033[1m"+current_path+"\033[0m"<<"$ ";
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
            if(arg=="")
            {
                arg=current_path;
                //std::cout<<current_path<<" "<<current_path.size()<<std::endl;
            }
            else if(arg[arg.size()-1]=='/')
            {
                arg=arg.substr(0,arg.size()-1);
            }
            //std::cout<<"arg "<<arg<<" "<<arg.size()<<std::endl;
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
            if(arg=="")
            {
                current_path="/";
                cur_inum=2;
                continue;
            }
            if(arg[arg.size()-1]=='/')
            {
                arg=arg.substr(0,arg.size()-1);
            }
            int i_num=find_inode(arg.c_str());
            //std::cout<<"cur_inum "<<i_num<<std::endl;
            if(i_num)
            {
                if(is_direc(i_num))
                {
                    //std::cout<<current_path<<" "<<arg<<std::endl;
                    current_path=parse_path(current_path,arg);
                    cur_inum=i_num;
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
        else if(cmd=="mkdir")
        {
            if(arg=="")
            {
                std::cout<<"should mkdir something"<<std::endl;
            }
            else if(arg[arg.size()-1]=='/')
            {
                arg=arg.substr(0,arg.size()-1);
            }
            else if(mkdir(arg.c_str()))
            {
                std::cout<<"check your path & dirname"<<std::endl;
            }
        }
        else if(cmd=="touch")
        {
            if(arg=="")
            {
                std::cout<<"should touch somthing"<<std::endl;
            }
            else if(arg[arg.size()-1]=='/')
            {
                std::cout<<"need a filename"<<std::endl;
            }
            else if(touch(arg.c_str()))
            {
                std::cout<<"check your path & filename"<<std::endl;
            }

        }
        else if(cmd=="cp")
        {
            trim(arg);
            if(arg=="")
            {
                
            }
            else
            {
                int pos=arg.find_first_of(' ');
                if(pos==std::string::npos)
                {
                    std::cout<<"too few arguments"<<std::endl;
                }
                else
                {
                    std::string arg1=arg.substr(0,pos);
                    std::string arg2=arg.substr(pos+1,arg.size());
                    trim(arg1);
                    trim(arg2);
                    if(arg1==""||arg1[arg1.size()-1]=='/')
                    {
                        std::cout<<"check your path & filename"<<std::endl;
                    }
                    else if(arg2==""||arg2[arg2.size()-1]=='/')
                    {
                        std::cout<<"check your path & filename"<<std::endl;
                    }
                    else
                    {
                        //std::cout<<arg1<<std::endl;
                        //std::cout<<arg2<<std::endl;
                        int fd1;
                        if((fd1=openf(arg1.c_str(),O_READ))!=-1)
                        {
                            int fd2;
                            if((fd2=openf(arg2.c_str(),O_WRITE))!=-1)
                            {
                                char buf[BLOCK_SIZE];
                                memset(buf,0,BLOCK_SIZE);
                                readf(fd1,buf,BLOCK_SIZE);
                                //std::cout<<buf<<std::endl;
                                //std::cout<<fd2<<std::endl;
                                if(writef(fd2,buf,strlen(buf))==0)
                                {
                                    std::cout<<"no space"<<std::endl;
                                }
                                closef(fd2);
                            }
                            else
                            {
                                if(touch(arg2.c_str()))
                                {
                                    std::cout<<"can not touch "<<arg2<<std::endl;
                                }
                                else
                                {
                                    if((fd2=openf(arg2.c_str(),O_WRITE))!=-1)
                                    {
                                        char buf[BLOCK_SIZE];
                                        memset(buf,0,BLOCK_SIZE);
                                        readf(fd1,buf,BLOCK_SIZE);
                                        //std::cout<<buf<<std::endl;
                                        if(writef(fd2,buf,strlen(buf))==0)
                                        {
                                            std::cout<<"no space"<<std::endl;
                                        }
                                        closef(fd2);
                                    }
                                    else
                                    {
                                        std::cout<<"can not open "<<arg2.size()<<std::endl;
                                    }
                                }
                            }
                            closef(fd1);
                        }
                        else
                        {
                            std::cout<<"can not open "<<arg1<<std::endl;
                        }
                    }

                }
            }
        }
        else if(cmd=="rm")
        {
            if(arg=="")
            {

            }
            else if(arg[arg.size()-1]=='/')
            {
                arg=arg.substr(0,arg.size()-1);
            }
            else
            {
                if(remove(arg.c_str())==-1)
                {
                    std::cout<<"check your path & filename"<<std::endl;
                }
            }
        }
        else if(cmd=="echo")
        {
            if(arg=="")
            {
                std::cout<<"should echo something"<<std::endl;
            }
            else if(arg[arg.size()-1]=='/')
            {
                std::cout<<"need a filename"<<std::endl;
            }
            else
            {
                std::string buf=arg.substr(arg.find_first_of('"')+1,arg.find_last_of('"')-1);
                arg=arg.substr(arg.find_last_of('"')+1);
                trim(arg);
                int fd;
                if((fd=openf(arg.c_str(),O_WRITE))!=-1)
                {
                    //std::cout<<"fd "<<fd<<std::endl;
                    //std::cout<<file_table.size()<<std::endl;
                    //std::cout<<file_table[0].i_num<<std::endl;
                    if(writef(fd,buf.c_str(),buf.size())==0)
                    {
                        std::cout<<"no space"<<std::endl;
                    }
                    closef(fd);
                }
                else
                {
                    std::cout<<"check your path & filename"<<std::endl;
                }
            }
        }
        else if(cmd=="cat")
        {
            if(arg=="")
            {
                std::cout<<"should cat something"<<std::endl;
            }
            else if(arg[arg.size()-1]=='/')
            {
                std::cout<<"need a filename"<<std::endl;
            }
            else
            {
                char buf[BLOCK_SIZE*2];
                memset(buf,0,sizeof(char)*BLOCK_SIZE*2);
                int fd;
                if((fd=openf(arg.c_str(),O_READ))!=-1)
                {
                    if(readf(fd,buf,BLOCK_SIZE*2)==0)
                    {
                       std::cout<<"nothing"<<std::endl;
                    }
                    else
                    {
                        std::cout<<buf<<std::endl;
                    }
                    closef(fd);
                }
                else
                {
                    std::cout<<"check your path & filename"<<std::endl;
                }
            }
        }
        else if(cmd=="ll")
        {
            if(arg=="")
            {
                arg=current_path;
                //std::cout<<current_path<<" "<<current_path.size()<<std::endl;
            }
            else if(arg[arg.size()-1]=='/')
            {
                arg=arg.substr(0,arg.size()-1);
            }
            //std::cout<<"arg "<<arg<<" "<<arg.size()<<std::endl;
            int i_num=find_inode(arg.c_str());
            if(i_num)
            {
                if(is_direc(i_num))
                {
                    std::vector<file_info> list=lldir(i_num);
                    for(int i=0;i<list.size();i++)
                    {
                       show_fileinfo(list[i]);
                    }
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
        else if(cmd=="clear")
        {
            system("clear");
        }
        else if(cmd=="exit")
        {
            break;
        }
        else if(cmd=="")
        {

        }
        else if(cmd=="help")
        {
            std::cout<<"help   how to operate\n"<<"clear  clear screen\n"<<"exit   quit\n";
            std::cout<<"ls     [path]\n"<<"ll     [path]\n"<<"cd     [path]\n"<<"touch  [path]filename\n";
            std::cout<<"mkdir  [path]directory\n"<<"cat    [path]filename\n";
            std::cout<<"echo   \"string\"           [path]filename\n"<<"rm     [path]directory or [path]filename\n";
            std::cout<<"cp     [path]filename     [path]filename\n";
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
