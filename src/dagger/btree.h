#ifndef BTREE2_H
#define BTREE2_H
#include <cstring>
#include <vector>
#include <cassert>
#include <cstdlib>
#include <iostream>

using namespace std;


class BtreeNode {
    public:
    vector<char> data;
    BtreeNode* root;
    bool is_leaf;
    static const size_t pointer_size= sizeof(BtreeNode*);
    const size_t  item_size;
    BtreeNode(size_t item_size):item_size(item_size){
        root= NULL;
        is_leaf = true;
    }
    size_t isz(){
        size_t res = item_size;
        if (!is_leaf){
            res += pointer_size;
        }
        return res;
    }
    size_t size(){
        size_t sz = data.size();
        if (!is_leaf){
            assert(sz >= pointer_size * 2 + item_size);
            sz -= pointer_size;
        }
        return sz / isz();
    }
    void delete_children(){
        if (!is_leaf){
            assert(data.size() >= pointer_size);
            size_t index = 0;
            while(index < data.size()){
                BtreeNode* child = *(((BtreeNode**)(data.data() + index)));
                if (child != NULL){
                    child->delete_children();
                    delete child;
                }
                index += isz();
            }
        }
    }
    virtual ~BtreeNode(){}
};

class BtreeIterator {
    public:
    bool is_item;
    BtreeNode* node;
    //BtreeNode* node;
    int index;
    const size_t key_size;
static const size_t max_node_size = 1024;
    BtreeIterator(BtreeNode* node,size_t key_size):
        key_size(key_size)
    {
        load(node);
     }
    BtreeIterator(const BtreeIterator& other) : key_size(other.key_size){
        node = other.node;
        index = other.index;
        is_item = other.is_item; 
    }
    BtreeIterator& operator=(const BtreeIterator& other){
        assert(&other != this);
        assert(other.key_size == key_size);
        node = other.node;
        index = other.index;
        is_item = other.is_item;
        return *this;
    }

BtreeNode* child(){
        BtreeNode* res = NULL;
        if (!node->is_leaf && !is_item){
            assert((index + BtreeNode::pointer_size) <= node->data.size());
            res = *((BtreeNode**)(node->data.data() + index));
        }
        return res;
    }
        
    
    int key_lesser2(int* src,int* fkey) const{
        assert(src != fkey);
		size_t i = 0;
		while ( i < (key_size-1) && src[i] == fkey[i]){
			i++;
		}
        //cout <<  src[i] << " >>> " << fkey[i] - src[i] << endl;
		return fkey[i] - src[i];
	}

    int key_lesser(int* fkey) const{
        int* src = key();
        assert(src != fkey);
		size_t i = 0;
		while ( i < (key_size-1) && src[i] == fkey[i]){
			i++;
		}
        //cout <<  src[i] << " >>> " << fkey[i] - src[i] << endl;
		return fkey[i] - src[i];
	}
    void first(){
        assert(node->size() > 0);
        index = 0;
        if (!node->is_leaf)
            index += BtreeNode::pointer_size;
        is_item =true;
        assert(index < (long)node->data.size());
    }
    void last(){
        assert(node->size() > 0);
        index = 0;
        index = node->isz() * ((long)node->size()-1);
        if (!node->is_leaf)
           index += BtreeNode::pointer_size;
        is_item =true;
        assert(index < (long)node->data.size());
    }
    void mid(char* from_key,char* to_key){
        assert(node->size() > 2);
        assert(from_key < to_key);
        size_t fi = from_key - node->data.data();
        size_t ti = to_key - node->data.data();
        size_t dist = (ti - fi) / node->isz();
        //cout << "???" << dist << endl;
        assert(dist > 1);
        index = fi + (dist/2) * node->isz(); 
        is_item = true;
    }
    // Non Recursive
    void find_in_node(int* fkey){
        if (node->size() == 0){
            assert(node->root == NULL);
            assert(node->is_leaf == true);
            // First position
            index = 0;
            is_item = false;
            assert(!is_item);
            assert(child() == NULL);
            return;
        }
        first();
        int* from_key = key();
        int res = key_lesser(fkey);
        if (res > 0 && node->size()  > 1){
            last();
            int* to_key = key();
            res = key_lesser(fkey);
            if (res < 0 && node->size() > 2) {
                // Vieme ze vysledok je vecsi ako from_key a mensi ako to_key 
                // Pokial je nejaky uzol v strede
                while (((char*)to_key - (char*)from_key) > (long)node->isz()){
                    assert(key_lesser2(to_key,fkey) < 0);
                    assert(key_lesser2(from_key,fkey) > 0);
                    mid((char*)from_key,(char*)to_key);
                    res = key_lesser(fkey);
                    if (res <0){
                        to_key = key();
                    }
                    else if (res > 0){
                        from_key = key();
                    }
                    else{
                        break;
                    }
                }
           }
        }
        if (res > 0){
            next_position();
        }
        else if (res < 0){
            previous_position();
        }

        if (res != 0){
            assert(!is_item);
        }
        else{
            assert(is_item);
        }
    }
    void find(int* fkey){
        find_in_node(fkey);
        if (child() != NULL){
            assert(!is_item);
#ifndef NDEBUG
            BtreeNode* nd = node;
#endif
            load(child());
            assert(nd == node->root);
            find(fkey);
        }
    }
     bool next_position(){
        assert(node->data.size() > 0);
        if (is_item){
            index += node->item_size;
        }
        else if (!node->is_leaf){
            index += BtreeNode::pointer_size;
        }
        is_item = !is_item;
        if (is_item && index >= (long)node->data.size())
            return false;
        return true;
    }
    bool previous_position(){
        assert(index >= 0);
        if (index < 0){
            return false;
        }
        if (!is_item){
            index -= node->item_size;
        }
        else if (!node->is_leaf){
            index -= BtreeNode::pointer_size;
        }
        is_item = !is_item;
        return true;
    }
    void load(BtreeNode* parent){
        node = parent;
        is_item = true;
        index = -node->item_size;
    }

    bool prev(){
        assert(is_item);
        int* lk =  NULL;
        if (index >= 0 && index < (long)node->data.size())
           lk = key();
        // Descnd one down
        if (previous_position() &&   child() != NULL){
            // Last position of the left child
            load(child());
            index = node->data.size();
            is_item = true;
            return prev(); 
        }
        else if (previous_position() && index >= 0)
            return true;
        assert(is_item);
        // If is the first position, go one up
        while(node->root != NULL){
            assert(index < 0);
#ifndef NDEBUG
            BtreeNode* nd = node;
#endif
            load(node->root);
            assert(lk != NULL);
            find_in_node(lk);
            assert(child() == nd);
            if (previous_position() && index >= 0){
                    return true;
            }
        }
        return false;
 }
    
    bool next(){
        assert(is_item);
        if (node->size() == 0)
            return false;
        int* lk =  NULL;
        if (index >= 0)
           lk = key();
        bool r = next_position();
        if (!r)
            abort();
        if (child() != NULL){
            load(child());
            return next();
        }
        if(next_position())
            return true;
        while(node->root != NULL){
            load(node->root);
            assert(lk != NULL);
            find_in_node(lk);
            if (next_position())
                return true;
        }
        return false;
    }
    void plot(){
        load(node);
        int i = 0;
        while (next_position()){
            cout << ".";
            if (i% 2 == 0){
                if(child() == NULL){
                    cout << " - ";
                }
                else{
                    assert(child()->root == node);
                    cout << " * ";
                }
            }
            else{
                cout << "{";
                for (size_t j = 0 ; j < key_size; j++ ){
                    cout << key()[j] << " ";
                }
                cout << "}";
            }
            i++;

        }
        cout << endl;
    }
    
    int* key() const{
        assert(is_item);
        assert(index >= 0);
        assert(index < (long)node->data.size());
        return (int*)(node->data.data() + index);
    }
    double* double_value(){
        assert(is_item);
        assert(index >= 0);
        assert(index < (long)node->data.size());
        return (double*)(node->data.data() + index + key_size * sizeof(int));
    }
    void split2(){
        if (node->data.size() <= max_node_size)
            return;
        first();
        char* fk = (char*)key();
        last();
        char* lk = (char*)key();
        mid(fk,lk);

        vector<char> olddata;
        olddata.swap(node->data);
        // Node je teraz prazdne
        size_t lsz = index;
        size_t rsz = (long)olddata.size() - index - node->item_size;
        char* val = olddata.data() + index;
        BtreeNode* left = new BtreeNode(node->item_size);
        left->is_leaf = node->is_leaf;
        BtreeNode* right = new BtreeNode(node->item_size);
        right->is_leaf = node->is_leaf;
        char* dest;
        if (node->root == NULL){
            node->data.resize(BtreeNode::pointer_size * 2 + node->item_size);
            dest = node->data.data();
            node->is_leaf = false;
        }
        else{
            BtreeNode* nd = node;
            // Node je teraz root
            load(nd->root);
            find_in_node((int*)val);
            assert(!node->is_leaf);
            assert(!is_item);
            assert(child() == nd);
            assert(nd->root == node);
            // Msz je velkost na pravo od miesta
            long msz = node->data.size() - index - BtreeNode::pointer_size;
            assert(msz >= 0);
            node->data.resize(node->data.size() + BtreeNode::pointer_size + node->item_size);
            assert(nd->data.size() == 0);
            delete nd;
            dest = node->data.data() + index;
            memmove(dest + BtreeNode::pointer_size*2  + node->item_size,dest + BtreeNode::pointer_size,msz);
        }
        left->root = node;
        right->root = node;
        // Lavy syn
        memcpy(dest,(void*)&left,BtreeNode::pointer_size);
        // Hodnota
        memcpy(dest+ BtreeNode::pointer_size, val,node->item_size); 
        // Pravy syn
        memcpy(dest+ BtreeNode::pointer_size + node->item_size,(void*)&right,BtreeNode::pointer_size);
        // Nastavenie obsahu laveho a praveho syna
        right->data.resize(rsz);
        memcpy(right->data.data(),olddata.data() + lsz + node->item_size,rsz);
        left->data.swap(olddata);
        left->data.resize(lsz);
        assert(!node->is_leaf);
        load(left);
        while(next_position()){
            if (child() != NULL){
                child()->root = left;
            }
        }
        //plot();
        load(right);
        while(next_position()){
            if (child() != NULL){
                child()->root = right;
            }
        }
        //plot();
        load(right->root);
        //plot();
        split2();
    }
    size_t size(){
        size_t res = node->size();
        if (res > 0){
            load(node);
            while(next_position()){
                if (child() != NULL){
                    BtreeIterator it(child(),key_size);
                    res += it.size();
                }
            }
        }
        return res;
    }
    void insert(int* item){
        assert(!is_item);
        assert(child()== NULL);
        if (node->size() == 0){
            assert(node->root == NULL);
            assert(node->is_leaf);
        }
        size_t nsz = node->item_size;
        if (!node->is_leaf){
            nsz +=  2* BtreeNode::pointer_size;
        }
        size_t rsz = (long)node->data.size() - index;
        node->data.resize(node->data.size() + nsz);
        if (rsz > 0){
            memmove(node->data.data() + index + nsz,node->data.data() + index,rsz);
        }
        memset(node->data.data() + index, 0, nsz);
        if (!node->is_leaf){
            index += BtreeNode::pointer_size;
        }
        // Kopirovanie hodnoty
        memcpy(node->data.data() + index,(char*)item,key_size*sizeof(int));
        is_item = true;
        bool r= false;
        if (node->data.size() > max_node_size)
            r = true;
        split2();
        if (r){
            find(item);
        }
        assert(is_item);
        // Na konci by mal byt iterator nastaveny na vkladany prvok
       
    }

    bool has_value(){
        return is_item;
    }
};
 class BtreeMap {
     public:
     BtreeNode root;
     const size_t key_size;
     BtreeMap(size_t key_size,size_t value_size):
        root(value_size + key_size*sizeof(int)),
         key_size(key_size)
     {
         
     }
     virtual ~BtreeMap(){
         root.delete_children();
     }

     void swap(BtreeMap& other){
         other.root.data.swap(root.data);
         assert(key_size == other.key_size);
         bool isleaf = other.root.is_leaf;
         other.root.is_leaf = root.is_leaf;
         root.is_leaf = isleaf;
     }

     BtreeIterator iterator() const{
         return BtreeIterator((BtreeNode*)&root,key_size);
     }
    BtreeIterator iterator() {
         return BtreeIterator(&root,key_size);
     }
     pair<bool,BtreeIterator> insert(int* key){
         BtreeIterator it = iterator();
         it.find(key);
         bool r = it.is_item;
         if (!r){
             it.insert(key);
         }
         return make_pair(!r,it);
         
     }

     BtreeIterator find(int* key) const {
         BtreeIterator it = iterator();
         it.find(key);
         return it;
     }

     BtreeIterator find_previous(int* key) const{
         BtreeIterator it = find(key);
         if (!it.is_item) {
             if (it.node->size() == 0){
                 it.previous_position();
             }
             else{
                it.next_position();
                it.prev();
             }
         }
         assert(it.is_item);
         return it;
     }



     size_t size() const{
         BtreeIterator it = iterator();
         return it.size();
     }

     void serialize(ostream& of){
         size_t sz = size();
         of.write((char*)&key_size,sizeof(size_t));
         of.write((char*)&sz,sizeof(size_t));
         of.write((char*)&root.item_size,sizeof(size_t));
         BtreeIterator it = iterator();
         while (it.next()){
             of.write((char*)it.key(),root.item_size);
         }
     }
    void deserialize(istream& of){
         size_t sz = 0;
         of.read((char*)&key_size,sizeof(size_t));
         of.read((char*)&sz,sizeof(size_t));
         size_t isze= 0;
         of.read((char*)&isze,sizeof(size_t));
         assert(isze == root.item_size);
         root.is_leaf = true;
         root.data.resize(sz * root.item_size);
         of.read(root.data.data(),sz * root.item_size);
         
     }
 };

#endif /* BTREE2_H */
