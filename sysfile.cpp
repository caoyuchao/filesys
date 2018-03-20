/*------------------------------------------------------------------------
	=> File Name: sysfile.cpp
	=> Author: caoyuchao
	=> Mail: cycbhbjxd@gmail.com 
	=> Created Time: 2018年03月17日 星期六 13时33分24秒
----------------------------------------------------------------------*/

//#include<iostream>
#include<stdio.h>
#include"sysfile.h"
#include<string.h>
#include<iostream>
//size_t fread ( void *buffer, size_t size, size_t count, FILE *stream );
//size_t fwrite(const void* buffer, size_t size, size_t count, FILE* stream);
//FILE * fopen(const char * path, const char * mode);
//int fclose( FILE *fp);
//int fseek(FILE *stream, long offset, int fromwhere);
//void *memset(void *s, int ch, size_t n);

FILE* disk;
super_block sublock;
const char* diskname="file.dat";
std::vector<file_item> file_table;
std::string current_path="/";
unsigned short cur_inum=2;


void format_disk()
{
    printf("formating...\n");
    disk=fopen(diskname,"wb");
    char end=EOF;
    fseek(disk,(BLOCK_NUM+BLOCK_INODE_NUM)*BLOCK_SIZE,SEEK_SET);
    fwrite(&end,1,1,disk);
    fseek(disk,0,SEEK_SET);
    load_block ldblock;
    fwrite(&ldblock,sizeof(ldblock),1,disk);
    
    sublock.ibitmap=0;
    memset(sublock.dbitmap,0,sizeof(int)*64);
    fseek(disk,BLOCK_SIZE,SEEK_SET);
    fwrite(&sublock,sizeof(sublock),1,disk);
    fflush(disk);
   
    int i_num=get_free_inode();
    setibitmap(i_num,1);
    i_node node;
    node.use_count=1;
    node.i_type=DIREC;
    node.i_mode=O_READ;
    node.i_length=0;
    node.time=time(NULL);
    
    int d_num=get_free_block();
    setdbitmap(d_num,1);
    node.i_addr[0]=d_num;
    
    directory direc;
    strcpy(direc.file_name,"..");
    direc.i_num=i_num;

        
    fseek(disk,GetDBlcokOffSet(d_num),SEEK_SET);
    fwrite(&direc,sizeof(direc),1,disk);
    node.i_length+=sizeof(direc);
    
    fseek(disk,GetINodeOffSet(i_num),SEEK_SET);
    fwrite(&node,sizeof(node),1,disk);

    fflush(disk);
    fclose(disk);
}

unsigned short get_free_block()
{
    for(int i=0;i<BITMAP_NUM;i++)
    {
        int mask=0x1;
        int j=0;
        while(mask)
        {
            if(!(sublock.dbitmap[i]&mask))
            {
                return 32*i+j+BLOCK_INODE_NUM;
            }
            j++;
            mask<<=1;
        }
    }
    return 0;
}

void setdbitmap(unsigned short num,int flag)
{
    num-=BLOCK_INODE_NUM;
    int mask=0x1<<(num%32);
    if(flag)
        sublock.dbitmap[num/32]|=mask;
    else
        sublock.dbitmap[num/32]&=~mask;
    fseek(disk,BLOCK_SIZE,SEEK_SET);
    fwrite(&sublock,sizeof(sublock),1,disk);
    fflush(disk);
}

unsigned short get_free_inode()
{
    int mask=0x1;
    int j=0;
    while(mask)
    {
        if(!(sublock.ibitmap&mask))
            return j+2;//512*2+64*(j-2)
        j++;
        mask<<=1;
    }
    return 0;//full
}

void setibitmap(unsigned short num,int flag)
{
    num-=2;
    int mask=0x1<<num;
    if(flag)
        sublock.ibitmap|=mask;
    else
        sublock.ibitmap&=~mask;
    fseek(disk,BLOCK_SIZE,SEEK_SET);
    fwrite(&sublock,sizeof(sublock),1,disk);
    fflush(disk);
}

void copybuf(char* buf,const char* buffer,int n)
{
    for(int i=0;i<n;i++)
    {
        buf[i]=buffer[i];
    }
}

unsigned short find_inode_bypath(const char* cur_path,unsigned short i_num)
{
    i_node node;
    fseek(disk,GetINodeOffSet(i_num),SEEK_SET);
    fread(&node,sizeof(node),1,disk);
    int i_bound=node.i_length/BLOCK_SIZE;
    int i_bound_remain=(node.i_length%BLOCK_SIZE)/sizeof(directory);
    int i_more_bound=0;
    int direc_num=BLOCK_SIZE/sizeof(directory);
    directory direcArr[BLOCK_SIZE/sizeof(directory)];

    std::string path=cur_path;
    int pos=path.find_first_of('/');
    
    std::string cur_filename;
    std::string newpath;
    if(pos!=std::string::npos)
    {
        cur_filename=path.substr(0,pos);
        newpath=path.substr(pos+1);
    }
    if(path==".")
    {
    //    std::cout<<"from last. "<<std::endl;
        return i_num;
    }
    if(cur_filename==".")
    {
    //    std::cout<<"from.   "<<cur_filename<<"  "<<newpath<<std::endl;
        return find_inode_bypath(newpath.c_str(),i_num);
    }
    if(i_bound>7)
    {
        i_more_bound=i_bound-7;
        i_bound=7;
    }
    for(int i=0;i<i_bound;i++)
    {
        fseek(disk,GetDBlcokOffSet(node.i_addr[i]),SEEK_SET);
        fread(direcArr,sizeof(directory),direc_num,disk);
        for(int j=0;j<direc_num;j++)
        {
            if(strcmp(direcArr[j].file_name,cur_filename.c_str())==0)
            {
                return find_inode_bypath(newpath.c_str(),direcArr[j].i_num);
            }
            if(strcmp(direcArr[j].file_name,cur_path)==0)
            {
                return direcArr[j].i_num;
            }
        }
    }
    int flag=i_more_bound||(i_bound==7&&i_bound_remain);
    if(flag)
    {
        unsigned short dblocks[BLOCK_SIZE/sizeof(unsigned short)];
        fseek(disk,GetINodeOffSet(node.i_addr[7]),SEEK_SET);
        fread(dblocks,sizeof(unsigned short),BLOCK_SIZE/sizeof(unsigned short),disk);
        for(int i=0;i<i_more_bound;i++)
        {
            fseek(disk,GetDBlcokOffSet(dblocks[i]),SEEK_SET);
            fread(direcArr,sizeof(directory),direc_num,disk);
            for(int j=0;j<direc_num;j++)
            {
                if(strcmp(direcArr[j].file_name,cur_filename.c_str())==0)
                {
                    return find_inode_bypath(newpath.c_str(),direcArr[j].i_num);
                }
                if(strcmp(direcArr[j].file_name,cur_path)==0)
                {
                    return direcArr[j].i_num;
                }
            }
        }
        if(i_bound_remain)
        {
            fseek(disk,GetDBlcokOffSet(dblocks[i_more_bound]),SEEK_SET);
            fread(direcArr,sizeof(directory),direc_num,disk);
            for(int i=0;i<i_bound_remain;i++)
            {
                if(strcmp(direcArr[i].file_name,cur_filename.c_str())==0)
                {
                    return find_inode_bypath(newpath.c_str(),direcArr[i].i_num);
                }
                if(strcmp(direcArr[i].file_name,cur_path)==0)
                {
                    return direcArr[i].i_num;
                }
            }
        }
    }
    else if(!flag&&i_bound_remain)
    {
        fseek(disk,GetDBlcokOffSet(node.i_addr[i_bound]),SEEK_SET);
        fread(direcArr,sizeof(directory),direc_num,disk);
        for(int i=0;i<i_bound_remain;i++)
        {
            if(strcmp(direcArr[i].file_name,cur_filename.c_str())==0)
            {
            //    std::cout<<"from..  "<<cur_filename<<"  "<<newpath<<"a"<<std::endl;
                return find_inode_bypath(newpath.c_str(),direcArr[i].i_num);
            }
            if(strcmp(direcArr[i].file_name,cur_path)==0)
            {
                return direcArr[i].i_num;
            }
        }
    }
    return 0;
}

int is_open(unsigned short i_num,unsigned short o_mode)
{
    for(int i=0;i<file_table.size();i++)
    {
        if(file_table[i].i_num==i_num&&file_table[i].o_mode==o_mode&&file_table[i].valid)
        {
            return i;
        }
    }
    return -1;
}

int openf(const char* path,unsigned short o_mode)
{
    unsigned short i_num;
    if(path[0]=='/')
    {
        i_num=find_inode_bypath(path+1,2);
    }
    else if(path[0]=='.'&&path[1]=='.'&&path[2]=='/')
    {
        i_num=find_inode_bypath(path,cur_inum);
    }
    else if(path[0]=='.'&&path[1]=='/')
    {
        i_num=find_inode_bypath(path+2,cur_inum);
    }
    else return -1;
    if(i_num==0)
        return -1;
    int fd;
    if((fd=is_open(i_num,o_mode))!=-1)
    {
        file_table[fd].use_count++;
        return fd;
    }
    i_node node;
    fseek(disk,GetINodeOffSet(i_num),SEEK_SET);
    fread(&node,sizeof(node),1,disk);
    if(node.i_type==DIREC)
        return -1;
    file_item item;
    item.o_mode=o_mode;
    item.i_num=i_num;
    item.use_count=1;
    item.valid=1;
    file_table.push_back(item);
    return file_table.size()-1;
}
unsigned int readf(int fd,void* buf,unsigned int count)
{
    if(fd>=file_table.size())
        return 0;
    if(file_table[fd].valid&&file_table[fd].use_count&&(file_table[fd].o_mode&O_READ))
    {
        int i_num=file_table[fd].i_num;
        i_node node;
        fseek(disk,GetINodeOffSet(i_num),SEEK_SET);
        fread(&node,sizeof(node),1,disk);
        if(count>node.i_length)
            count=node.i_length;

        int i_bound=count/BLOCK_SIZE;
        int i_bound_remain=count/BLOCK_SIZE;
        int i_more_bound=0;
        if(i_bound>7)
        {
            i_more_bound=i_bound-7;
            i_bound=7;
        }

        char buffer[BLOCK_SIZE];
        for(int i=0;i<i_bound;i++)
        {
            fseek(disk,GetDBlcokOffSet(node.i_addr[i]),SEEK_SET);
            fread(buffer,sizeof(char),BLOCK_SIZE,disk);
            copybuf((char*)buf+i*BLOCK_SIZE,buffer,BLOCK_SIZE);
        }
        int flag=i_more_bound||(i_bound==7&&i_bound_remain);
        if(flag)
        {
            unsigned short dblocks[BLOCK_SIZE/sizeof(unsigned short)];
            fseek(disk,GetINodeOffSet(node.i_addr[7]),SEEK_SET);
            fread(dblocks,sizeof(unsigned short),BLOCK_SIZE/sizeof(unsigned short),disk);
            for(int i=0;i<i_more_bound;i++)
            {
                fseek(disk,GetDBlcokOffSet(dblocks[i]),SEEK_SET);
                fread(buffer,sizeof(char),BLOCK_SIZE,disk);
                copybuf((char*)buf+(i+i_bound)*BLOCK_SIZE,buffer,BLOCK_SIZE);
            }
            if(i_bound_remain)
            {
                fseek(disk,GetDBlcokOffSet(dblocks[i_more_bound]),SEEK_SET);
                fread(buffer,sizeof(char),BLOCK_SIZE,disk);
                copybuf((char*)buf+(i_more_bound+i_bound)*BLOCK_SIZE,buffer,i_bound_remain);
            }
        }
        else if(!flag&&i_bound_remain)
        {
            fseek(disk,GetDBlcokOffSet(node.i_addr[i_bound]),SEEK_SET);
            fread(buffer,sizeof(char),BLOCK_SIZE,disk);
            copybuf((char*)buf+i_bound*BLOCK_SIZE,buffer,i_bound_remain);
        }
        return count;
    }
    return 0;
}
unsigned int writef(int fd,const void* buf,unsigned int count)
{
    if(fd>=file_table.size())
        return 0;
    if(file_table[fd].valid&&file_table[fd].use_count&&(file_table[fd].o_mode&O_WRITE))
    {
        unsigned short i_num=file_table[fd].i_num;
        i_node node;
        fseek(disk,GetINodeOffSet(i_num),SEEK_SET);
        fread(&node,sizeof(node),1,disk);
        int i_bound=node.i_length/BLOCK_SIZE;
        int i_bound_remain=node.i_length%BLOCK_SIZE?1:0;
        int i_more_bound=0;
        if(i_bound>7)
        {
            i_more_bound=i_bound-7;
            i_bound=7;
        }
        for(int i=0;i<i_bound;i++)
        {
            setdbitmap(node.i_addr[i],0);
        }
        int flag=i_more_bound||(i_bound==7&&i_bound_remain);
        if(flag)
        {
            unsigned short dblocks[BLOCK_SIZE/sizeof(unsigned short)];
            fseek(disk,GetINodeOffSet(node.i_addr[7]),SEEK_SET);
            fread(dblocks,sizeof(unsigned short),BLOCK_SIZE/sizeof(unsigned short),disk);
            for(int i=0;i<i_more_bound;i++)
            {
                setdbitmap(dblocks[i],0);
            }
            if(i_bound_remain)
            {
                setdbitmap(dblocks[i_more_bound],0);
            }
            setdbitmap(node.i_addr[7],0);
        }
        else if(!flag&&i_bound_remain)
        {
            setdbitmap(node.i_addr[i_bound],0);
        }
        node.i_length=0;
        
        i_bound=count/BLOCK_SIZE;
        i_bound_remain=count%BLOCK_SIZE;
        i_more_bound=0;
        if(i_bound>7)
        {
            i_more_bound=i_bound-7;
            i_bound=7;
        }
        for(int i=0;i<i_bound;i++)
        {
            int dblock_num=get_free_block();
            if(dblock_num)
            {
                setdbitmap(dblock_num,1);
                node.i_addr[i]=dblock_num;
                fseek(disk,GetDBlcokOffSet(dblock_num),SEEK_SET);
                fwrite((char*)buf+i*BLOCK_SIZE,sizeof(char),BLOCK_SIZE,disk);
                fflush(disk);
                node.i_length+=BLOCK_SIZE;
            }
            else
            {
                fseek(disk,GetINodeOffSet(i_num),SEEK_SET);
                fwrite(&node,sizeof(node),1,disk);
                fflush(disk);
                return node.i_length;
            }
        }
        flag=i_more_bound||(i_bound==7&&i_bound_remain);
        if(flag)
        {
            unsigned short dblocks[BLOCK_SIZE/sizeof(unsigned short)];
            int db_addr8=get_free_block();
            if(!db_addr8)
            {
                fseek(disk,GetINodeOffSet(i_num),SEEK_SET);
                fwrite(&node,sizeof(node),1,disk);
                fflush(disk);
                return node.i_length;
            }
            else
            {
                setdbitmap(db_addr8,1);
                node.i_addr[7]=db_addr8;
            }
            for(int i=0;i<i_more_bound;i++)
            {
                int dblock_num=get_free_block();
                if(dblock_num)
                {
                    setdbitmap(dblock_num,1);
                    dblocks[i]=dblock_num;
                    fseek(disk,GetDBlcokOffSet(dblock_num),SEEK_SET);
                    fwrite((char*)buf+(i_bound+i)*BLOCK_SIZE,sizeof(char),BLOCK_SIZE,disk);
                    fflush(disk);
                    node.i_length+=BLOCK_SIZE;
                }
                else
                {
                    if(i==0)
                    {
                        setdbitmap(db_addr8,0);
                        return node.i_length;
                    }
                    else
                    {
                        fseek(disk,GetDBlcokOffSet(db_addr8),SEEK_SET);
                        fwrite(dblocks,sizeof(unsigned short),BLOCK_SIZE/sizeof(unsigned short),disk);
                        fflush(disk);
                        fseek(disk,GetINodeOffSet(i_num),SEEK_SET);
                        fwrite(&node,sizeof(node),1,disk);
                        fflush(disk);
                        return node.i_length;
                    }
                }
            }
            int dblock_num=get_free_block();
            if(i_bound_remain&&dblock_num)
            {
                setdbitmap(dblock_num,1);
                dblocks[i_more_bound]=dblock_num;
                fseek(disk,GetDBlcokOffSet(dblock_num),SEEK_SET);
                fwrite((char*)buf+(i_bound+i_more_bound)*BLOCK_SIZE,sizeof(char),i_bound_remain,disk);
                fflush(disk);
                node.i_length+=i_bound_remain;
            }
        }
        int dblock_num=get_free_block();
        if(!flag&&dblock_num&&i_bound_remain)
        {
            setdbitmap(dblock_num,1);
            node.i_addr[i_bound]=dblock_num;
            fseek(disk,GetDBlcokOffSet(dblock_num),SEEK_SET);
            fwrite((char*)buf+i_bound*BLOCK_SIZE,sizeof(char),i_bound_remain,disk);
            fflush(disk);
            node.i_length+=i_bound_remain;
        }
        fseek(disk,GetINodeOffSet(i_num),SEEK_SET);
        fwrite(&node,sizeof(node),1,disk);
        fflush(disk);
        return node.i_length;
    }
    return 0;
}
int closef(int fd)
{
    if(fd>=file_table.size())
        return -1;
    if(file_table[fd].use_count)
        file_table[fd].use_count--;
    return 0;
}

int sys_create(unsigned short i_num,const char* filename,unsigned short i_type)
{
    i_node p_node;
    fseek(disk,GetINodeOffSet(i_num),SEEK_SET);
    fread(&p_node,sizeof(p_node),1,disk);

    if(p_node.i_length==BLOCK_SIZE)
    {
        int dblock_num=get_free_block();
        if(dblock_num)
        {
            setdbitmap(dblock_num,1);
            p_node.i_addr[1]=dblock_num;
        }
        else
        {
            return -1;
        }
    }

    int new_i_num=get_free_inode();
    if(!new_i_num)
    {
        return -1;
    }
    setibitmap(new_i_num,1);

    i_node node;
    node.use_count=1;
    node.i_type=i_type;
    node.time=time(NULL);
    if(i_type==DIREC)
    {
        int dblock_num=get_free_block();
        if(dblock_num)
        {
            setdbitmap(dblock_num,1);
            node.i_addr[0]=dblock_num;
            
            directory direc;
            strcpy(direc.file_name,"..");
            direc.i_num=i_num;

            fseek(disk,GetDBlcokOffSet(dblock_num),SEEK_SET);
            fwrite(&direc,sizeof(direc),1,disk);
            fflush(disk);
            node.i_length+=sizeof(direc);
            node.i_type=DIREC;
            node.i_mode=O_READ;
        }
        else
        {
            return -1;
        }

    }
    else
    {
        node.i_length=0;
        node.i_type=DOCUM;
        node.i_mode=O_RD_WR;
    }

    directory direc;
    strncpy(direc.file_name,filename,29);
    direc.file_name[29]='\0';
    direc.i_num=new_i_num;

    directory direcArr[BLOCK_SIZE/sizeof(directory)];
    int i_bound=p_node.i_length/sizeof(directory);
    int dir_count=BLOCK_SIZE/sizeof(directory);
    
    if(i_bound>=dir_count)
    {
        fseek(disk,GetDBlcokOffSet(p_node.i_addr[1]),SEEK_SET);
        fread(direcArr,sizeof(directory),dir_count,disk);
        direcArr[i_bound-dir_count]=direc;
        fseek(disk,GetDBlcokOffSet(p_node.i_addr[1]),SEEK_SET);
        fwrite(direcArr,sizeof(directory),dir_count,disk);
        fflush(disk);
        return 0;
    }
    else
    {
        fseek(disk,GetDBlcokOffSet(p_node.i_addr[0]),SEEK_SET);
        fread(direcArr,sizeof(directory),dir_count,disk);
        direcArr[i_bound]=direc;
        fseek(disk,GetDBlcokOffSet(p_node.i_addr[0]),SEEK_SET);
        fwrite(direcArr,sizeof(directory),dir_count,disk);
        fflush(disk);
        return 0;
    }
}

int create(const char* path,unsigned short i_type)
{
    std::string cur_path=path;
    if(cur_path.find_first_of('/')==std::string::npos)
    {
        return sys_create(cur_inum,path,i_type);
    }
    else if(cur_path.find_first_of('/')==cur_path.find_last_of('/'))
    {
        if(path[0]=='/')
        {
            return sys_create(2,path+1,i_type);
        }
        else if(path[0]=='.'&&path[1]=='.'&&path[2]=='/')
        {
            int i_num=find_inode_bypath("..",cur_inum);
            return sys_create(i_num,path+3,i_type);
        }
        else if(path[0]=='.'&&path[1]=='/')
        {
            return sys_create(cur_inum,path+2,i_type);
        }
        else return -1;
    }
    else
    {
        int pos=cur_path.find_last_of('/');
        if(pos==cur_path.size()-1)
            return -1;
        std::string newpath=cur_path.substr(0,pos);
        int i_num;
        if(newpath[0]=='/')
        {
            i_num=find_inode_bypath(newpath.c_str()+1,2);
        }
        else if(newpath[0]=='.'&&newpath[1]=='.'&&newpath[2]=='/')
        {
            i_num=find_inode_bypath(newpath.c_str(),cur_inum);
        }
        else if(newpath[0]=='.'&&newpath[1]=='/')
        {
            i_num==find_inode_bypath(newpath.c_str()+2,cur_inum);
        }
        else return -1;

        return sys_create(i_num,cur_path.substr(pos,cur_path.size()).c_str(),i_type);
    }

}

int mkdir(const char* path)
{
    return create(path,DIREC);
}

int touch(const char* path)
{
    return create(path,DOCUM);
}

int remove(const char* path)
{
    return 0;
}

int sys_remove(unsigned short i_num)
{
    return 0;
}







