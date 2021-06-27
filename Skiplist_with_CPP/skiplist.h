#include <iostream> 
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <mutex>
#include <fstream>

#define STORE_FILE "store/dumpFile"

std::mutex mtx;     // mutex for critical section
std::string delimiter = ":";  //分界符

//Class template to implement node
//跳表节点的类模板
template<typename K, typename V> 
class Node {

public:
    
    Node() {} 

    Node(K k, V v, int); 

    ~Node();

    K get_key() const;

    V get_value() const;

    void set_value(V);
    
    // Linear array to hold pointers to next node of different level
    //用于保存指向不同级别的下一个节点的指针的数组
    Node<K, V> **forward;

    int node_level; //当前节点在第几层

private:
    K key;
    V value;
};

template<typename K, typename V> 
Node<K, V>::Node(const K k, const V v, int level) {
    this->key = k;
    this->value = v;
    this->node_level = level; 

    // level + 1, because array index is from 0 - level
    //数组索引是0-level，所以设置为level + 1
    this->forward = new Node<K, V>*[level+1];
    
	// Fill forward array with 0(NULL) 
    //将forward数组内存初始化为0
    memset(this->forward, 0, sizeof(Node<K, V>*)*(level+1));
};

//节点析构函数
template<typename K, typename V> 
Node<K, V>::~Node() {
    delete []forward;
};

//得到节点对应的key
template<typename K, typename V> 
K Node<K, V>::get_key() const {
    return key;
};

//得到节点对应的value
template<typename K, typename V> 
V Node<K, V>::get_value() const {
    return value;
};

//设置节点的value
template<typename K, typename V> 
void Node<K, V>::set_value(V value) {
    this->value=value;
};

// Class template for Skip list
//跳表的类模板
template <typename K, typename V> 
class SkipList {

public: 
    SkipList(int);
    ~SkipList();
    int get_random_level();
    Node<K, V>* create_node(K, V, int);
    int insert_element(K, V);
    void display_list();
    bool search_element(K);
    void delete_element(K);
    void dump_file();
    void load_file();
    int size();

private:
    void get_key_value_from_string(const std::string& str, std::string* key, std::string* value);
    bool is_valid_string(const std::string& str);

private:    
    // Maximum level of the skip list
    //设置跳表中节点的最大层数
    int _max_level;

    // current level of skip list 
    //保存跳表的当前层数
    int _skip_list_level;

    // pointer to header node 
    //跳表的头节点
    Node<K, V> *_header;

    // file operator
    //设置跳表的读写文件
    std::ofstream _file_writer;
    std::ifstream _file_reader;

    // skiplist current element count
    //统计跳表的当前层数
    int _element_count;
};

// create new node 
//新建跳表节点，返回指向新节点的指针，这里使用移动语义
template<typename K, typename V>
Node<K, V>* SkipList<K, V>::create_node(const K k, const V v, int level) {
    Node<K, V> *n = new Node<K, V>(k, v, level);
    return n;
}

// Insert given key and value in skip list 
// return 1 means element exists  
// return 0 means insert successfully
/* 
                           +------------+
                           |  insert 50 |
                           +------------+
level 4     +-->1+                                                      100
                 |
                 |                      insert +----+
level 3         1+-------->10+---------------> | 50 |          70       100
                                               |    |
                                               |    |
level 2         1          10         30       | 50 |          70       100
                                               |    |
                                               |    |
level 1         1    4     10         30       | 50 |          70       100
                                               |    |
                                               |    |
level 0         1    4   9 10         30   40  | 50 |  60      70       100
                                               +----+

*/
//向跳表中插入新节点
template<typename K, typename V>
int SkipList<K, V>::insert_element(const K key, const V value) {
    
    //首先上锁，进入临界区
    mtx.lock();
    Node<K, V> *current = this->_header;

    // create update array and initialize it 
    // update is array which put node that the node->forward[i] should be operated later
    //update数组用于保存新元素应该被插入的位置
    Node<K, V> *update[_max_level+1];
    memset(update, 0, sizeof(Node<K, V>*)*(_max_level+1));  

    // start form highest level of skip list 
    for(int i = _skip_list_level; i >= 0; i--) {
        while(current->forward[i] != NULL && current->forward[i]->get_key() < key) {
            current = current->forward[i]; 
        }
        update[i] = current;
    }

    // reached level 0 and forward pointer to right node, which is desired to insert key.
    //新元素的插入都要从底层开始
    current = current->forward[0];

    // if current node have key equal to searched key, we get it
    //如果存在相同的元素，就解锁，退出插入函数
    if (current != NULL && current->get_key() == key) {
        std::cout << "key: " << key << ", exists" << std::endl;
        mtx.unlock();
        return 1;
    }

    // if current is NULL that means we have reached to end of the level 
    // if current's key is not equal to key that means we have to insert node between update[0] and current node 
    //如果插入位置在尾部或跳表中不存在相同的元素
    if (current == NULL || current->get_key() != key ) {
        
        // Generate a random level for node
        //生成插入节点的随机层数
        int random_level = get_random_level();

        // If random level is greater thar skip list's current level, initialize update value with pointer to header
        //如果随机层数超过了跳表的当前最大层数，那么高层的update数组中的指针指向
        //header，并在最后更新跳表的当前最大层数
        if (random_level > _skip_list_level) {
            for (int i = _skip_list_level+1; i < random_level+1; i++) {
                update[i] = _header;
            }
            _skip_list_level = random_level;
        }

        // create new node with random level generated 
        //生成相应的节点
        Node<K, V>* inserted_node = create_node(key, value, random_level);
        
        // insert node 
        //将新节点插入跳表中
        for (int i = 0; i <= random_level; i++) {
            inserted_node->forward[i] = update[i]->forward[i];
            update[i]->forward[i] = inserted_node;
        }
        std::cout << "Successfully inserted key:" << key << ", value:" << value << std::endl;
        //更新跳表的节点总数
        _element_count ++;
    }
    //解锁并退出插入函数
    mtx.unlock();
    return 0;
}

// Display skip list
//逐层打印跳表元素
template<typename K, typename V> 
void SkipList<K, V>::display_list() {

    std::cout << "\n*****Skip List*****"<<"\n"; 
    for (int i = 0; i <= _skip_list_level; i++) {
        Node<K, V> *node = this->_header->forward[i]; 
        std::cout << "Level " << i << ": ";
        while (node != NULL) {
            std::cout << node->get_key() << ":" << node->get_value() << ";";
            node = node->forward[i];
        }
        std::cout << std::endl;
    }
}

// Dump data in memory to file 
//将跳表中的元素保存到dumpfile中
template<typename K, typename V> 
void SkipList<K, V>::dump_file() {

    std::cout << "dump_file-----------------" << std::endl;
    _file_writer.open(STORE_FILE);
    Node<K, V> *node = this->_header->forward[0]; 

    while (node != NULL) {
        _file_writer << node->get_key() << ":" << node->get_value() << "\n";
        std::cout << node->get_key() << ":" << node->get_value() << ";\n";
        node = node->forward[0];
    }

    _file_writer.flush();
    _file_writer.close();
    return ;
}

// Load data from disk
//从磁盘中保存的文件恢复跳表
template<typename K, typename V> 
void SkipList<K, V>::load_file() {

    _file_reader.open(STORE_FILE);
    std::cout << "load_file-----------------" << std::endl;
    std::string line;
    std::string* key = new std::string();
    std::string* value = new std::string();
    while (getline(_file_reader, line)) {
        get_key_value_from_string(line, key, value);
        if (key->empty() || value->empty()) {
            continue;
        }
        insert_element(*key, *value);
        std::cout << "key:" << *key << "value:" << *value << std::endl;
    }
    _file_reader.close();
}

// Get current SkipList size 
//得到跳表的当前总节点数
template<typename K, typename V> 
int SkipList<K, V>::size() { 
    return _element_count;
}

//从保存文件的一行字符串中取得key和value
template<typename K, typename V>
void SkipList<K, V>::get_key_value_from_string(const std::string& str, std::string* key, std::string* value) {

    if(!is_valid_string(str)) {
        return;
    }
    *key = str.substr(0, str.find(delimiter));
    *value = str.substr(str.find(delimiter)+1, str.length());
}

//判断保存文件的一行字符串是否合法
template<typename K, typename V>
bool SkipList<K, V>::is_valid_string(const std::string& str) {

    //如果为空，判定非法
    if (str.empty()) {
        return false;
    }
    //如果不存在分界符，判定非法
    if (str.find(delimiter) == std::string::npos) {
        return false;
    }
    return true;
}


// Delete element from skip list 
//删除跳表中的指定元素
template<typename K, typename V> 
void SkipList<K, V>::delete_element(K key) {

    mtx.lock();
    Node<K, V> *current = this->_header; 
    Node<K, V> *update[_max_level+1];
    memset(update, 0, sizeof(Node<K, V>*)*(_max_level+1));

    // start from highest level of skip list
    for (int i = _skip_list_level; i >= 0; i--) {
        while (current->forward[i] !=NULL && current->forward[i]->get_key() < key) {
            current = current->forward[i];
        }
        update[i] = current;
    }

    current = current->forward[0];
    if (current != NULL && current->get_key() == key) {
       
        // start for lowest level and delete the current node of each level
        //更新跳表的连接，即删除此节点后的前后连接重新生成
        for (int i = 0; i <= _skip_list_level; i++) {

            // if at level i, next node is not target node, break the loop.
            if (update[i]->forward[i] != current) 
                break;

            update[i]->forward[i] = current->forward[i];
        }

        // Remove levels which have no elements
        //如果移除此节点后跳表的当前最大层数减少，更新相应计数
        while (_skip_list_level > 0 && _header->forward[_skip_list_level] == 0) {
            _skip_list_level --; 
        }

        std::cout << "Successfully deleted key "<< key << std::endl;
        //更新节点总数
        _element_count --;
    }
    //解锁
    mtx.unlock();
    return;
}

// Search for element in skip list 
/*
                           +------------+
                           |  select 60 |
                           +------------+
level 4     +-->1+                                                      100
                 |
                 |
level 3         1+-------->10+------------------>50+           70       100
                                                   |
                                                   |
level 2         1          10         30         50|           70       100
                                                   |
                                                   |
level 1         1    4     10         30         50|           70       100
                                                   |
                                                   |
level 0         1    4   9 10         30   40    50+-->60      70       100
*/

//查询跳表中是否存在某元素
template<typename K, typename V> 
bool SkipList<K, V>::search_element(K key) {

    std::cout << "search_element-----------------" << std::endl;
    Node<K, V> *current = _header;

    // start from highest level of skip list
    for (int i = _skip_list_level; i >= 0; i--) {
        while (current->forward[i] && current->forward[i]->get_key() < key) {
            current = current->forward[i];
        }
    }

    //reached level 0 and advance pointer to right node, which we search
    current = current->forward[0];

    // if current node have key equal to searched key, we get it
    if (current and current->get_key() == key) {
        std::cout << "Found key: " << key << ", value: " << current->get_value() << std::endl;
        return true;
    }

    std::cout << "Not Found Key:" << key << std::endl;
    return false;
}

// construct skip list
//跳表构造函数
template<typename K, typename V> 
SkipList<K, V>::SkipList(int max_level) {

    this->_max_level = max_level;
    this->_skip_list_level = 0;
    this->_element_count = 0;

    // create header node and initialize key and value to null
    //构造头节点
    K k;
    V v;
    this->_header = new Node<K, V>(k, v, _max_level);
};

//跳表析构函数，关闭文件描述符，释放头节点
template<typename K, typename V> 
SkipList<K, V>::~SkipList() {

    if (_file_writer.is_open()) {
        _file_writer.close();
    }
    if (_file_reader.is_open()) {
        _file_reader.close();
    }
    delete _header;
}

//得到新建节点的随机层数
template<typename K, typename V>
int SkipList<K, V>::get_random_level(){

    int k = 1;
    while (rand() % 2) {
        k++;
    }
    k = (k < _max_level) ? k : _max_level;
    return k;
};
// vim: et tw=100 ts=4 sw=4 cc=120
