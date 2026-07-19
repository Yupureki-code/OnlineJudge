#include <iostream>

int main()
{
    while(1)
    {
        std::string fullPath;
        std::cin>>fullPath;
        std::string filePath = fullPath.substr(0,fullPath.rfind('/')); 
        std::cout<<filePath<<std::endl; 
    }
    return 0;
}