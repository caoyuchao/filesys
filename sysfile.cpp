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

void setfdinvalid(unsigned short i_num)
{
    for(int i=0;i<file_table.size();i++)
    {
        if(file_table[i].i_num==i_num)
            file_table[i].valid=0;
    }
}

void copybuf(char* buf,const char* buffer,int n)
{
    for(int i=0;i<n;i++)
    {
        buf[i]=buffer[i];
    }
}

unsigned short find_inode(const char* path)
{
    unsigned short i_num=0;
    if(path[0]=='/')
    {
        //std::cout<<"path "<<path<<std::endl;
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
    else
    {
        i_num=find_inode_bypath(path,cur_inum);
    }
    return i_num;
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
    //std::cout<<"debug "<<cur_path<<"  "<<i_num<<std::endl;
    std::string path=cur_path;
    int pos=path.find_first_of('/');
    
    std::string cur_filename;
    std::string newpath;
    if(pos!=std::string::npos)
    {
        cur_filename=path.substr(0,pos);
        newpath=path.substr(pos+1);
    }
    if(path=="."||path=="")
    {
    //    std::cout<<"from last. "<<std::endl;
        return i_num;
    }
    if(cur_filename==".")
    {
    //    std::cout<<"from.   "<<cur_filename<<"  "<<newpath<<std::endl;
        return find_inode_bypath(newpath.c_str(),i_num);
    }
    //std::cout<<"i_bound"<<i_bound<<std::endl;
    //std::cout<<"i_bound_remain"<<i_bound_remain<<std::endl;
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
    if(i_bound_remain)
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
    i_num=find_inode(path);
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
    //std::cout<<"file_table size "<<file_table.size()<<std::endl;
    return file_table.size()-1;
}
unsigned int readf(int fd,void* buf,unsigned int count)
{
    if(fd>=file_table.size())
        return 0;
    if(!is_docum(file_table[fd].i_num))
    {
        return 0;
    }
    if(file_table[fd].valid&&file_table[fd].use_count&&(file_table[fd].o_mode&O_READ))
    {
        //std::cout<<"permmit read..."<<std::endl;
        int i_num=file_table[fd].i_num;
        i_node node;
        fseek(disk,GetINodeOffSet(i_num),SEEK_SET);
        fread(&node,sizeof(node),1,disk);
        if(count>node.i_length)
            count=node.i_length;
        
        //std::cout<<"count "<<count<<std::endl;
        int i_bound=count/BLOCK_SIZE;
        int i_bound_remain=count%BLOCK_SIZE;
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
        //std::cout<<"reading "<<i_more_bound<<" "<<i_bound<<" "<<i_bound_remain<<std::endl;
        int flag=i_more_bound||(i_bound==7&&i_bound_remain);
        if(flag)
        {
            unsigned short dblocks[BLOCK_SIZE/sizeof(unsigned short)];
            fseek(disk,GetDBlcokOffSet(node.i_addr[7]),SEEK_SET);
            fread(dblocks,sizeof(unsigned short),BLOCK_SIZE/sizeof(unsigned short),disk);
            for(int i=0;i<i_more_bound;i++)
            {
                fseek(disk,GetDBlcokOffSet(dblocks[i]),SEEK_SET);
                fread(buffer,sizeof(char),BLOCK_SIZE,disk);
                copybuf((char*)buf+(i+i_bound)*BLOCK_SIZE,buffer,BLOCK_SIZE);
            }
            //std::cout<<"i_bound_remain "<<i_bound_remain<<std::endl;
            //std::cout<<"node.iaddr[7] "<<node.i_addr[7]<<std::endl;
            if(i_bound_remain)
            {
                fseek(disk,GetDBlcokOffSet(dblocks[i_more_bound]),SEEK_SET);
                //std::cout<<"dblocks[0]"<<dblocks[i_more_bound]<<std::endl;
                fread(buffer,sizeof(char),BLOCK_SIZE,disk);
                //std::cout<<buffer<<std::endl;
                copybuf((char*)buf+(i_more_bound+i_bound)*BLOCK_SIZE,buffer,i_bound_remain);
            }
        }
        if(!flag&&i_bound_remain)
        {
            //std::cout<<node.i_addr[i_bound]<<std::endl;
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
    //std::cout<<fd<<std::endl;
    if(file_table[fd].valid&&file_table[fd].use_count&&(file_table[fd].o_mode&O_WRITE))
    {
        //std::cout<<"permmit..."<<std::endl;
        unsigned short i_num=file_table[fd].i_num;
        clear_dblock(i_num);
        i_node node;
        fseek(disk,GetINodeOffSet(i_num),SEEK_SET);
        fread(&node,sizeof(node),1,disk);
        //std::cout<<"node i_length "<<node.i_length<<std::endl;  
        int i_bound=count/BLOCK_SIZE;
        int i_bound_remain=count%BLOCK_SIZE;
        int i_more_bound=0;
        //std::cout<<"i_bound"<<i_bound<<std::endl;
        //std::cout<<"i_bound_remain"<<i_bound_remain<<std::endl;
        //std::cout<<"i_more_bound"<<i_more_bound<<std::endl;
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
        int flag=i_more_bound||(i_bound==7&&i_bound_remain);
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
                //std::cout<<"i_addr "<<db_addr8<<std::endl;
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
                //std::cout<<"writting db_num"<<dblock_num<<"i_more_bound "<<i_more_bound<<"db_addr8 "<<db_addr8<<std::endl;
                fseek(disk,GetDBlcokOffSet(db_addr8),SEEK_SET);
                fwrite(dblocks,sizeof(unsigned short),BLOCK_SIZE/sizeof(unsigned short),disk);
                fflush(disk);
                //std::cout<<" "<<i_bound_remain<<std::endl;
                //std::cout<<(char*)buf+(i_bound+i_more_bound)*BLOCK_SIZE<<std::endl;
                fseek(disk,GetDBlcokOffSet(dblock_num),SEEK_SET);
                fwrite((char*)buf+(i_bound+i_more_bound)*BLOCK_SIZE,sizeof(char),i_bound_remain,disk);
                fflush(disk);
                node.i_length+=i_bound_remain;
            }
        }
        int dblock_num=get_free_block();
        //std::cout<<"dblock num "<<dblock_num<<std::endl;

        if(!flag&&dblock_num&&i_bound_remain)
        {
            setdbitmap(dblock_num,1);
            node.i_addr[i_bound]=dblock_num;
            fseek(disk,GetDBlcokOffSet(dblock_num),SEEK_SET);
            //std::cout<<"debug "<<(char*)buf+i_bound*BLOCK_SIZE<<std::endl;
            fwrite((char*)buf+i_bound*BLOCK_SIZE,sizeof(char),i_bound_remain,disk);
            fflush(disk);
            node.i_length+=i_bound_remain;
            //std::cout<<"file length "<<node.i_length<<std::endl;
        }
        fseek(disk,GetINodeOffSet(i_num),SEEK_SET);
        fwrite(&node,sizeof(node),1,disk);
        fflush(disk);
        //std::cout<<"i_length "<<node.i_length<<std::endl;
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
    if((fd==file_table.size()-1)&&file_table[fd].use_count==0)
        file_table.pop_back();
    return 0;
}

int sys_create(unsigned short i_num,const char* filename,unsigned short i_type)
{
    i_node p_node;
    fseek(disk,GetINodeOffSet(i_num),SEEK_SET);
    fread(&p_node,sizeof(p_node),1,disk);

    int tmp=0;
    if(p_node.i_length==BLOCK_SIZE)
    {
        int dblock_num=get_free_block();
        tmp=dblock_num;
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
        if(tmp)setdbitmap(tmp,0);
        return -1;
    }
    setibitmap(new_i_num,1);
    //std::cout<<"use i_num "<<new_i_num<<std::endl;
    i_node node;
    node.use_count=1;
    node.i_type=i_type;
    node.time=time(NULL);
    int tmp2=0;
    if(i_type==DIREC)
    {
        int dblock_num=get_free_block();
        if(dblock_num)
        {
            tmp2=dblock_num;
            //std::cout<<"dir block "<<dblock_num<<std::endl;
            setdbitmap(dblock_num,1);
            node.i_addr[0]=dblock_num;
            
            directory direc;
            strcpy(direc.file_name,"..");
            direc.i_num=i_num;
            //std::cout<<"its father i_num "<<i_num<<std::endl;

            fseek(disk,GetDBlcokOffSet(dblock_num),SEEK_SET);
            fwrite(&direc,sizeof(direc),1,disk);
            fflush(disk);
            node.i_length=sizeof(direc);
            node.i_type=DIREC;
            node.i_mode=O_READ;
        }
        else
        {
            setibitmap(new_i_num,0);
            if(tmp)setdbitmap(tmp,0);
            return -1;
        }

    }
    else
    {
        node.i_length=0;
        node.i_type=DOCUM;
        node.i_mode=O_RD_WR;
    }

    fseek(disk,GetINodeOffSet(new_i_num),SEEK_SET);
    fwrite(&node,sizeof(node),1,disk);
    fflush(disk);

    directory direc;
    strncpy(direc.file_name,filename,29);
    direc.file_name[29]='\0';
    direc.i_num=new_i_num;
    
    //std::cout<<"filename "<<direc.file_name<<std::endl;
    directory direcArr[BLOCK_SIZE/sizeof(directory)];
    int i_bound=p_node.i_length/sizeof(directory);
    int dir_count=BLOCK_SIZE/sizeof(directory);
    //std::cout<<"i_bound "<<i_bound<<std::endl; 
    if(i_bound>=dir_count)
    {

        fseek(disk,GetDBlcokOffSet(p_node.i_addr[0]),SEEK_SET);
        fread(direcArr,sizeof(directory),dir_count,disk);
        for(int i=0;i<dir_count;i++)
        {
            if(strcmp(direcArr[i].file_name,filename)==0)
            {
                setibitmap(new_i_num,0);
                if(tmp)setdbitmap(tmp,0);
                if(tmp2)setdbitmap(tmp2,0);
                return -1;
            }
        }
        

        fseek(disk,GetDBlcokOffSet(p_node.i_addr[1]),SEEK_SET);
        fread(direcArr,sizeof(directory),dir_count,disk);
        
        for(int i=0;i<i_bound-dir_count;i++)
        {
            if(strcmp(direcArr[i].file_name,filename)==0)
            {
                setibitmap(new_i_num,0);
                if(tmp)setdbitmap(tmp,0);
                if(tmp2)setdbitmap(tmp2,0);
                return -1;
            }
        }
        
        
        direcArr[i_bound-dir_count]=direc;
        fseek(disk,GetDBlcokOffSet(p_node.i_addr[1]),SEEK_SET);
        fwrite(direcArr,sizeof(directory),dir_count,disk);
        fflush(disk);
    }
    else
    {
        fseek(disk,GetDBlcokOffSet(p_node.i_addr[0]),SEEK_SET);
        fread(direcArr,sizeof(directory),dir_count,disk);
        for(int i=0;i<i_bound;i++)
        {
            if(strcmp(direcArr[i].file_name,filename)==0)
            {
                //std::cout<<"the same name "<<filename<<std::endl;
                setibitmap(new_i_num,0);
                if(tmp)setdbitmap(tmp,0);
                if(tmp2)setdbitmap(tmp2,0);
                //std::cout<<"tmp2 "<<tmp2<<std::endl;
                return -1;
            }
        }

        direcArr[i_bound]=direc;
        fseek(disk,GetDBlcokOffSet(p_node.i_addr[0]),SEEK_SET);
        fwrite(direcArr,sizeof(directory),dir_count,disk);
        fflush(disk);
    }
    p_node.i_length+=sizeof(directory);

    //std::cout<<"fater length "<<p_node.i_length<<std::endl;

    fseek(disk,GetINodeOffSet(i_num),SEEK_SET);
    fwrite(&p_node,sizeof(p_node),1,disk);
    fflush(disk);
    return 0;
}

int create(const char* path,unsigned short i_type)
{
    std::string cur_path=path;
    unsigned short i_num;
    int pos=cur_path.find_last_of('/');
    //std::cout<<path<<std::endl;
    if(pos==std::string::npos)
    {
        if(cur_path==".."||cur_path=="."||cur_path=="/")
            return -1;
        return sys_create(cur_inum,path,i_type);
    }
    i_num=find_inode(cur_path.substr(0,pos+1).c_str());
    //std::cout<<"path fater inode "<<i_num<<std::endl;
    if(i_num==0)
        return -1;
    std::string cur_filename=cur_path.substr(pos+1,cur_path.size()).c_str();
    //std::cout<<"filename "<<cur_filename<<std::endl;
    if(cur_filename==".."||cur_filename=="."||cur_filename=="/")
        return -1;
    return sys_create(i_num,cur_path.substr(pos+1,cur_path.size()).c_str(),i_type);
}

int mkdir(const char* path)
{
    return create(path,DIREC);
}

int touch(const char* path)
{
    return create(path,DOCUM);
}

int p_remove_c(unsigned short p_i_num,unsigned short i_num)
{
    i_node node;
    fseek(disk,GetINodeOffSet(p_i_num),SEEK_SET);
    fread(&node,sizeof(node),1,disk);
    directory direcArr[BLOCK_SIZE/sizeof(directory)];
    int i_bound=node.i_length/BLOCK_SIZE;
    int i_bound_remain=(node.i_length%BLOCK_SIZE)/sizeof(directory);

    directory tmp;
    if(i_bound_remain)
    {
        fseek(disk,GetDBlcokOffSet(node.i_addr[i_bound]),SEEK_SET);
        fread(direcArr,sizeof(directory),BLOCK_SIZE/sizeof(directory),disk);
        tmp=direcArr[i_bound_remain-1];
    }
    else
    {
        fseek(disk,GetDBlcokOffSet(node.i_addr[i_bound-1]),SEEK_SET);
        fread(direcArr,sizeof(directory),BLOCK_SIZE/sizeof(directory),disk);
        tmp=direcArr[31];
    }

    for(int i=0;i<i_bound;i++)
    {
        fseek(disk,GetDBlcokOffSet(node.i_addr[i]),SEEK_SET);
        fread(direcArr,sizeof(directory),BLOCK_SIZE/sizeof(directory),disk);
        for(int j=0;j<BLOCK_SIZE/sizeof(directory);j++)
        {
            if(direcArr[j].i_num==i_num)
            {
                direcArr[j]=tmp;
            }
        }
        fseek(disk,GetDBlcokOffSet(node.i_addr[i]),SEEK_SET);
        fwrite(direcArr,sizeof(directory),BLOCK_SIZE/sizeof(directory),disk);
        fflush(disk);
    }
    if(i_bound_remain)
    {
        fseek(disk,GetDBlcokOffSet(node.i_addr[i_bound]),SEEK_SET);
        fread(direcArr,sizeof(directory),BLOCK_SIZE/sizeof(directory),disk);
        for(int i=0;i<i_bound_remain;i++)
        {
            if(direcArr[i].i_num==i_num)
            {
                direcArr[i]=tmp;
            }
        }
        fseek(disk,GetDBlcokOffSet(node.i_addr[i_bound]),SEEK_SET);
        fwrite(direcArr,sizeof(directory),BLOCK_SIZE/sizeof(directory),disk);
        fflush(disk);
    }
    setibitmap(i_num,0);
    node.i_length-=sizeof(directory);
    fseek(disk,GetINodeOffSet(p_i_num),SEEK_SET);
    fwrite(&node,sizeof(node),1,disk);
    fflush(disk);
    return 0;
}

int remove(const char* path)
{
    unsigned short i_num;
    i_num=find_inode(path);
    if(i_num==0)
        return -1;
    //std::cout<<"remove i_num"<<i_num<<std::endl;
    unsigned short p_i_num;
    std::string cur_path=path;
    int pos=cur_path.find_last_of('/');
    if(pos==std::string::npos)
    {
        if(cur_path==".."||cur_path=="."||cur_path=="/")
            return -1;
        p_i_num=cur_inum;
        //std::cout<<"p_i_num "<<p_i_num<<std::endl;
    }
    else
    {
        p_i_num=find_inode(cur_path.substr(0,pos+1).c_str());
        //std::cout<<"p_i_num "<<p_i_num<<std::endl;
    }
    if(p_i_num==0)
        return -1;
    int state=sys_remove(i_num);
    if(state==-1)
        return -1;
    state=p_remove_c(p_i_num,i_num);
    return state;
}

int sys_remove(unsigned short i_num)
{
    i_node node;
    fseek(disk,GetINodeOffSet(i_num),SEEK_SET);
    fread(&node,sizeof(node),1,disk);
    if(node.i_type==DOCUM)
    {
        clear_dblock(i_num);
        setfdinvalid(i_num);
        return 0;
    }
    else if(node.i_type==DIREC)
    {
        int i_bound=node.i_length/BLOCK_SIZE;
        int i_more_bound=0;
        int i_bound_remain=(node.i_length%BLOCK_SIZE)/sizeof(directory);
        if(i_bound>32)
        {
            i_more_bound=i_bound-32;
            i_bound=32;
        }
        directory direcArr[BLOCK_SIZE/sizeof(directory)];
        for(int i=0;i<i_bound;i++)
        {
            fseek(disk,GetDBlcokOffSet(node.i_addr[i]),SEEK_SET);
            fread(direcArr,sizeof(directory),BLOCK_SIZE/sizeof(directory),disk);
            for(int j=0;j<BLOCK_SIZE/sizeof(directory);j++)
            {
                if(strcmp(direcArr[j].file_name,"..")!=0)
                {
                    sys_remove(direcArr[j].i_num);
                    setibitmap(direcArr[j].i_num,0);
                }
            }
            setdbitmap(node.i_addr[i],0);
        }
        if(i_bound_remain)
        {
            fseek(disk,GetDBlcokOffSet(node.i_addr[i_bound]),SEEK_SET);
            fread(direcArr,sizeof(directory),BLOCK_SIZE/sizeof(directory),disk);
            for(int i=0;i<i_bound_remain;i++)
            {
                if(strcmp(direcArr[i].file_name,"..")!=0)
                {
                    sys_remove(direcArr[i].i_num);
                    setibitmap(direcArr[i].i_num,0);
                }
            }
            setdbitmap(node.i_addr[i_bound],0);
        }
        return 0;
    }
    else return -1;
}

void clear_dblock(unsigned short i_num)
{
        i_node node;
        //std::cout<<"clear block "<<i_num<<std::endl;
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
            fseek(disk,GetDBlcokOffSet(node.i_addr[7]),SEEK_SET);
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
        fseek(disk,GetINodeOffSet(i_num),SEEK_SET);
        fwrite(&node,sizeof(node),1,disk);
        fflush(disk);
}

//tmp=parse_path("/file/hello/world","../../../.././../hello/world/like");
//tmp=parse_path("/file/hello/world",".././../hello/world/like");
std::string parse_path(std::string path,std::string str_format)
{
    if(str_format.find_first_of('/')==std::string::npos)//.. or . should be processed before call this function.    ..->../ .->./
    {
        if(str_format=="..")
        {
            int pos=path.find_last_of('/');
            std::string tmppath=path.substr(0,pos);
            return pos?tmppath:"/";
        }
        else if(str_format==".")
        {
            return path;
        }
        else if(path=="/")
        {
            return path+str_format;
        }
        else return path+"/"+str_format;
    }
    else if(str_format.find_first_of('/')==str_format.find_last_of('/'))
    {
        if(str_format[0]=='/')
        {
            return str_format;
        }
        else if(str_format[0]=='.'&&str_format[1]=='.'&&str_format[2]=='/'&&str_format[3]!='.')
        {
            int pos=path.find_last_of('/');
            return path.substr(0,pos+1)+str_format.substr(3,str_format.size());
            
        }
        else if(str_format[0]=='.'&&str_format[1]=='/'&&str_format[3]!='.')
        {
            if(path=="/")
                return path+str_format.substr(2,str_format.size());
            else
                return path+"/"+str_format.substr(2,str_format.size());
        }
        else
        {
            int format_pos=str_format.find_first_of('/');
            std::string cur_filename=str_format.substr(0,format_pos);
            std::string tmppath=parse_path(path,cur_filename);
            //std::cout<<tmppath<<std::endl;
            return parse_path(tmppath,str_format.substr(format_pos+1,str_format.size()));
        }
    }
    else
    {
        if(str_format[0]=='/')
        {
            return parse_path("/",str_format.substr(1,str_format.size()));
        }
        else
        {
            //std::cout<<"OK"<<std::endl;
            int format_pos=str_format.find_first_of('/');
            std::string cur_filename=str_format.substr(0,format_pos);
            std::string tmppath=parse_path(path,cur_filename);
            //std::cout<<tmppath<<std::endl;
            //std::string tmp=str_format.substr(format_pos+1,str_format.size());
            //std::cout<<tmp<<std::endl;
            return parse_path(tmppath,str_format.substr(format_pos+1,str_format.size()));
        }
    }
}

std::vector<std::string> lsdir(unsigned short i_num)
{
    i_node node;
    fseek(disk,GetINodeOffSet(i_num),SEEK_SET);
    fread(&node,sizeof(node),1,disk);
    std::vector<std::string> list;
    int i_bound=node.i_length/BLOCK_SIZE;
    int i_bound_remain=(node.i_length%BLOCK_SIZE)/sizeof(directory);
    directory direcArr[BLOCK_SIZE/sizeof(directory)];
    i_node tmpnode;
    for(int i=0;i<i_bound;i++)
    {
        fseek(disk,GetDBlcokOffSet(node.i_addr[i]),SEEK_SET);
        fread(direcArr,sizeof(directory),BLOCK_SIZE/sizeof(directory),disk);
        for(int j=0;j<BLOCK_SIZE/sizeof(directory);j++)
        {
            fseek(disk,GetINodeOffSet(direcArr[j].i_num),SEEK_SET);
            fread(&tmpnode,sizeof(tmpnode),1,disk);
            std::string tmp=direcArr[j].file_name;
            if(tmpnode.i_type==DIREC)
            {
                tmp.push_back('/');
            }
            list.push_back(tmp);
        }
    }
    if(i_bound_remain)
    {
        fseek(disk,GetDBlcokOffSet(node.i_addr[i_bound]),SEEK_SET);
        fread(direcArr,sizeof(directory),BLOCK_SIZE/sizeof(directory),disk);
        for(int i=0;i<i_bound_remain;i++)
        {
            fseek(disk,GetINodeOffSet(direcArr[i].i_num),SEEK_SET);
            fread(&tmpnode,sizeof(tmpnode),1,disk);
            std::string tmp=direcArr[i].file_name;
            if(tmpnode.i_type==DIREC)
            {
                tmp.push_back('/');
            }
            list.push_back(tmp);
        }
    }
    return list;
}

int is_direc(unsigned short i_num)
{
    i_node node;
    fseek(disk,GetINodeOffSet(i_num),SEEK_SET);
    fread(&node,sizeof(node),1,disk);
    return node.i_type==DIREC;
}

int is_docum(unsigned short i_num)
{
    i_node node;
    fseek(disk,GetINodeOffSet(i_num),SEEK_SET);
    fread(&node,sizeof(node),1,disk);
    return node.i_type==DOCUM;
}


std::vector<file_info> lldir(unsigned short i_num)
{
    i_node node;
    fseek(disk,GetINodeOffSet(i_num),SEEK_SET);
    fread(&node,sizeof(node),1,disk);
    std::vector<file_info> list;
    int i_bound=node.i_length/BLOCK_SIZE;
    int i_bound_remain=(node.i_length%BLOCK_SIZE)/sizeof(directory);
    directory direcArr[BLOCK_SIZE/sizeof(directory)];
    i_node tmpnode;
    for(int i=0;i<i_bound;i++)
    {
        fseek(disk,GetDBlcokOffSet(node.i_addr[i]),SEEK_SET);
        fread(direcArr,sizeof(directory),BLOCK_SIZE/sizeof(directory),disk);
        for(int j=0;j<BLOCK_SIZE/sizeof(directory);j++)
        {
            fseek(disk,GetINodeOffSet(direcArr[j].i_num),SEEK_SET);
            fread(&tmpnode,sizeof(tmpnode),1,disk);
            if(strcmp(direcArr[j].file_name,"..")==0)
                continue;
            std::string tmp=direcArr[j].file_name;
            file_info info;
            memset(info.file_name,0,30);
            strncpy(info.file_name,tmp.c_str(),tmp.size());
            info.i_type=tmpnode.i_type;
            info.i_mode=tmpnode.i_mode;
            info.i_length=get_length(direcArr[j].i_num);
            info.i_num=direcArr[j].i_num;
            info.time=tmpnode.time;

            list.push_back(info);
        }
    }
    if(i_bound_remain)
    {
        fseek(disk,GetDBlcokOffSet(node.i_addr[i_bound]),SEEK_SET);
        fread(direcArr,sizeof(directory),BLOCK_SIZE/sizeof(directory),disk);
        for(int i=0;i<i_bound_remain;i++)
        {
            fseek(disk,GetINodeOffSet(direcArr[i].i_num),SEEK_SET);
            fread(&tmpnode,sizeof(tmpnode),1,disk);
            if(strcmp(direcArr[i].file_name,"..")==0)
                continue;
            std::string tmp=direcArr[i].file_name;
            file_info info;
            memset(info.file_name,0,30);
            strncpy(info.file_name,tmp.c_str(),tmp.size());
            info.i_type=tmpnode.i_type;
            info.i_mode=tmpnode.i_mode;
            info.i_length=get_length(direcArr[i].i_num);
            info.i_num=direcArr[i].i_num;
            info.time=tmpnode.time;

            list.push_back(info);
        }
    }
    return list;
}

unsigned int get_length(unsigned short i_num)
{
    i_node node;
    fseek(disk,GetINodeOffSet(i_num),SEEK_SET);
    fread(&node,sizeof(node),1,disk);

    if(node.i_type==DOCUM)
    {
        return node.i_length;
    }
    else if(node.i_type==DIREC)
    {
        unsigned short length=0;
        int i_bound=node.i_length/BLOCK_SIZE;
        int i_bound_remain=(node.i_length%BLOCK_SIZE)/sizeof(directory);
        directory direcArr[BLOCK_SIZE/sizeof(directory)];
        i_node tmpnode;
        for(int i=0;i<i_bound;i++)
        {
            fseek(disk,GetDBlcokOffSet(node.i_addr[i]),SEEK_SET);
            fread(direcArr,sizeof(directory),BLOCK_SIZE/sizeof(directory),disk);
            for(int j=0;j<BLOCK_SIZE/sizeof(directory);j++)
            {
                if(strcmp(direcArr[j].file_name,"..")==0)
                    continue;
                length+=get_length(direcArr[j].i_num);
            }
        }
        if(i_bound_remain)
        {
            fseek(disk,GetDBlcokOffSet(node.i_addr[i_bound]),SEEK_SET);
            fread(direcArr,sizeof(directory),BLOCK_SIZE/sizeof(directory),disk);
            for(int i=0;i<i_bound_remain;i++)
            {
                if(strcmp(direcArr[i].file_name,"..")==0)
                    continue;
                length+=get_length(direcArr[i].i_num);
            } 
        }
        return length;
    }
    return 0;
}
