/*************************************************************************
  > File Name: mydirls.cpp
  > Author: caoyuchao
  > Mail: cycbhbjxd@gmail.com 
  > Created Time: 2017年11月29日 星期三 21时04分12秒
 ************************************************************************/

#include<iostream>
#include<string>
#include<algorithm>
#include<vector>
#include<cstring>
#include<cstdlib>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<dirent.h>
#include<queue>
#include<iomanip>
#include<ctime>
#include<pwd.h>
#include<grp.h>
char buf[20];
std::string filePermission(mode_t mode)
{
	std::string perStr;
	if(S_ISSOCK(mode))perStr.push_back('s');//套接字文件
	else if(S_ISLNK(mode))perStr.push_back('l');//符号链接文件
	else if(S_ISREG(mode))perStr.push_back('-');//普通文件
	else if(S_ISBLK(mode))perStr.push_back('b');//块设备文件
	else if(S_ISDIR(mode))perStr.push_back('d');//目录
	else if(S_ISCHR(mode))perStr.push_back('c');//字符设备文件

	S_IRUSR&mode?perStr.push_back('r'):perStr.push_back('-');//文件所有者是否有读权限
	S_IWUSR&mode?perStr.push_back('w'):perStr.push_back('-');//文件所有者是否有写权限
	S_IXUSR&mode?perStr.push_back('x'):perStr.push_back('-');//文件所有者是否有执行权限
	
	S_IRGRP&mode?perStr.push_back('r'):perStr.push_back('-');//文件所有者组是否有读权限
	S_IWGRP&mode?perStr.push_back('w'):perStr.push_back('-');//文件所有者组是否有写权限
	S_IXGRP&mode?perStr.push_back('x'):perStr.push_back('-');//文件所有者组是否有执行权限

	S_IROTH&mode?perStr.push_back('r'):perStr.push_back('-');//其他人是否有读权限
	S_IWOTH&mode?perStr.push_back('w'):perStr.push_back('-');//其他人是否有写权限
	S_IXOTH&mode?perStr.push_back('x'):perStr.push_back('-');//其他人是否有执行权限

	return perStr;
}
	

void showInfo(const struct stat& statbuf)
{
	std::cout<<filePermission(statbuf.st_mode);//输出文件类型和权限信息
	std::cout<<" "<<statbuf.st_nlink;//输出文件硬链接数
	std::cout<<" "<<std::left<<std::setw(12)<<getpwuid(statbuf.st_uid)->pw_name;//输出所有者名称
	std::cout<<" "<<std::left<<std::setw(12)<<getgrgid(statbuf.st_gid)->gr_name;//输出组名称
	std::cout<<" "<<std::right<<std::setw(10)<<statbuf.st_size<<"B";//输出文件对应字节数
	
	strftime(buf,sizeof(buf),"%Y-%m-%d %H:%M:%S",localtime(&statbuf.st_mtime));
	std::cout<<" "<<std::right<<std::setw(22)<<buf;//输出文件最后被访问的时间
	//std::cout<<" "<<std::right<<std::setw(30)<<ctime(&statbuf.st_atime);
}


void coutdir(const char* dir,std::string fullpath)
{
	DIR* dp;
	struct dirent* entry;
	struct stat statbuf;
	if((dp=opendir(dir))==NULL)//打开目录流
	{
		std::cout<<dir<<std::endl;
		std::cout<<"open dir failure..."<<std::endl;
	return;
	}

	if(fullpath[fullpath.size()-1]!='/')
		fullpath.push_back('/');
	std::cout<<fullpath<<std::endl;//输出当前路径
	std::queue<std::string> dirqueue; //目录名队列
	std::vector<std::string> namelist;
	
	chdir(dir);//切换到当前目录

	while(entry=readdir(dp))
	{
		if(strcmp(entry->d_name,".")==0||strcmp(entry->d_name,"..")==0)
			continue;
		namelist.push_back(entry->d_name);//记录文件名
	}

	std::sort(namelist.begin(),namelist.end());//升序排列


	for(auto d_name:namelist)//遍历
	{
		lstat(d_name.c_str(),&statbuf);//获取文件信息
		showInfo(statbuf);//显示当前文件信息
		
		if(S_ISDIR(statbuf.st_mode))
		{	
			//dirqueue.push(currentPath+d_name);//目录入队，等待当前目录所有信息输出之后，进行递归
			dirqueue.push(d_name);//目录入队，等待当前目录所有信息输出之后，进行递归
			d_name="\033[34m\033[1m"+d_name+"\033[0m";
		}
		else if(S_IXUSR&statbuf.st_mode)
		{
			d_name="\033[32m\033[1m"+d_name+"\033[0m";
		}
		//else if(压缩文件){显示红色，采用读取文件头的魔数即可，file命令}
		std::cout<<" "<<d_name<<std::endl;//输出文件名
	}
	while(!dirqueue.empty())
	{	
		coutdir(dirqueue.front().c_str(),fullpath+dirqueue.front());//递归输出当前目录下的子目录
		dirqueue.pop();
	}
		
	chdir("..");//切到上级目录
	closedir(dp);//关闭目录流
	return;
}

int main(int argc,char* argv[])
{
	std::string currentdir=".";
	if(argc>=2)
		currentdir=argv[1];//如果给了参数，则参数认为时路径名

	std::cout<<"Directory -> "<<currentdir<<std::endl;
	coutdir(currentdir.c_str(),currentdir);//递归输出目录信息，栈的大小只有8096KB,因此目录层次很深很多，将导致栈溢出，操作系统会报错。核心已转储，ulimit -a查看，ulimit -s修改即可。
	std::cout<<"over..."<<std::endl;
	return 0;
}
