/*------------------------------------------------------------------------
	=> File Name: str.cpp
	=> Author: caoyuchao
	=> Mail: cycbhbjxd@gmail.com 
	=> Created Time: 2018年03月20日 星期二 19时49分04秒
----------------------------------------------------------------------*/

#include<iostream>
#include<string>
int main()
{
    std::string test;
    std::getline(std::cin,test);
    std::cout<<test.size()<<std::endl;
    std::cout<<test<<std::endl;
    return 0;
}
