//简单的测试代码
#include <iostream>
#include "skiplist.h"
#define FILE_PATH "./store/dumpFile"

int main() {

    SkipList<std::string, std::string> skipList(6);
	skipList.insert_element("1", "a"); 
	skipList.insert_element("3", "bc"); 
	skipList.insert_element("7", "def"); 
	skipList.insert_element("8", "ghij"); 
	skipList.insert_element("9", "klmn"); 
	skipList.insert_element("18", "opqr"); 
	skipList.insert_element("19", "uvwxyz"); 

    std::cout << "skipList size:" << skipList.size() << std::endl;

    skipList.dump_file();

    // skipList.load_file();

    skipList.search_element("9");
    skipList.search_element("18");


    skipList.display_list();

    skipList.delete_element("3");
    skipList.delete_element("7");

    std::cout << "skipList size:" << skipList.size() << std::endl;

    skipList.display_list();
}
