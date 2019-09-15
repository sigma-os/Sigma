#ifndef ZETA_TREE
#define ZETA_TREE

#include <iostream>
#include <vector>
#include <string>

template<typename T>
struct tree_node {
    tree_node() {}
    tree_node(T item): item(item) {} 
    ~tree_node(){
        for(auto* child : this->children) delete child;
    }
    T item;
    std::vector<tree_node<T>*> children;
    tree_node<T>* parent;
};

template<typename T>
class tree {
    public:
    tree(): root(new tree_node<T>) {}
    ~tree() {
        delete root;
    }

    tree_node<T>* get_root(){
        return root;
    }

    tree_node<T>* insert(tree_node<T>& node, T item){
        //T& ref = item;
        node.children.push_back(new tree_node<T>);
        tree_node<T>* child = node.children.back();

        child->item = item;
        child->parent = &node;

        return child;
    }

    void print(){
        std::string prefix = "";
        this->print_internal(prefix, this->get_root());
    }

    private:
    tree_node<T>* root;

    void print_internal(std::string& prefix, tree_node<T>* node){
        std::cout << prefix << "├──";
        std::cout << node->item << std::endl;

        for(auto* child : node->children){
            std::string next_prefix;
            next_prefix.append(prefix);
            next_prefix.append("     ");

            print_internal(next_prefix, child);
        }
    }
};

#endif