#include "../include/tokenizer.h"
#include "../include/prefix_tree.h"
#include "../include/file_registery.h"
#include <iostream> // std::istream std::ostream
#include <vector>
#include <string>
#include <queue>
#include <unordered_map>
#include <unordered_set>
/*
    Writes a trivially copyable value to a binary file.

    Example:
        int x = 42;
        write_binary(file, x);

    The bytes of x are copied directly into the file.
*/
template<typename T>
void write_binary(std::ostream& file, const T& value) {
    file.write(
        reinterpret_cast<const char*>(&value),
        sizeof(T)
    );
}

/*
    Reads a value of type T from a binary file.

    Example:
        int x;
        read_binary(file, x);

    Reads sizeof(T) bytes from the file and stores them in x.
*/
template<typename T>
void read_binary(std::istream& file, T& value)
{
    file.read(
        reinterpret_cast<char*>(&value),
        sizeof(T)
    );
}


TrieNode::~TrieNode() {
    for(auto& [c, child] : links) {
        delete child; // avoid mem leaks
    }
}

bool TrieNode::contains_key(char c) const {
    return links.find(c) != links.end();
}

void TrieNode::put_key(char c, TrieNode* newNode) {
    links[c] = newNode;
}

TrieNode* TrieNode::get_key(char c) const {
    return links.at(c); // [] operator can modify map -> at() safely returns key
}

void TrieNode::set_terminal(int id) {
    file_ids.insert(id);
    is_terminal_flag = true;
}

bool TrieNode::is_terminal() const {
    return is_terminal_flag;
}

PrefixTree::PrefixTree() {root = new TrieNode();}

PrefixTree::~PrefixTree() {delete root;}

void PrefixTree::insert(int id, const std::string& token) {
    TrieNode* node = root;
    for(char c : token) {
        if(!node->contains_key(c)) node->put_key(c, new TrieNode());
        node = node->get_key(c);
    }
    node->set_terminal(id);
}

void PrefixTree::index_file_name(int id, const std::string& file_name) {
    std::vector<std::string> tokens = tokenize(file_name, false);
    for(auto& token : tokens) {
        insert(id, token);
    }
}

std::unordered_set<int> PrefixTree::search(const std::string& token) const {
    TrieNode* node = root;
    for(char c : token) {
        if(!node->contains_key(c)) {
            return {}; // return empty if word not found
        }
        node = node->get_key(c);
    }
    return node->file_ids;
}

std::unordered_set<int> PrefixTree::search_matching(const std::string& prefix) const {
    TrieNode* node = root;
    for(char c : prefix) {
        if(!node->contains_key(c)) {
            return {}; // return empty set if prefix not found
        }
        node = node->get_key(c);
    }

    std::unordered_set<int> f_ids;
    get_file_ids(node, f_ids);
    return f_ids;
}

void PrefixTree::get_file_ids(TrieNode* node, std::unordered_set<int>& f_ids) const {
    if(node->is_terminal()) {
        f_ids.insert(node->file_ids.begin(), node->file_ids.end());
    }
    for(auto& link : node->links) {
        get_file_ids(node->get_key(link.first), f_ids);
    }
}

void PrefixTree::get_words(TrieNode* node, std::vector<std::string>& words, std::string word = "") const {
    if(node->is_terminal()) {
        words.push_back(word);
    }
    for(auto& link : node->links) {
        word.push_back(link.first);
        get_words(node->get_key(link.first), words, word);
        word.pop_back(); // backtrack
    }
}

void PrefixTree::list_all() const {
    std::vector<std::string> words;
    get_words(root, words);
    for(auto& word : words) {
        std::cout << word << std::endl;
    }
}

void PrefixTree::serialize(std::ostream& out) const {
    /*
        We cannot store raw pointers.

        Example:

            root = 0x1234
            child = 0x5678

        Those addresses are only valid during the current run.

        Therefore we assign every node an integer index:

            root  -> 0
            child -> 1
            child -> 2

        and store those indices instead.
    */
    std::unordered_map<TrieNode*, int> node_to_index;

    /*
        Allows us to iterate through nodes later.

        nodes[0] -> root
        nodes[1] -> first child
        nodes[2] -> second child
    */
    std::vector<TrieNode*> nodes;

    /*
        Breadth-first traversal.

        We visit every node exactly once and assign it
        an index.
    */
    std::queue<TrieNode*> q;
    q.push(root);

    while(!q.empty()) {
        TrieNode* node = q.front();
        q.pop();

        // Skip nodes already assigned an index.
        if(node_to_index.count(node)) {
            continue;
        }

        node_to_index[node] = static_cast<int>(nodes.size());
        nodes.push_back(node);

        for(const auto& [c, child] : node->links) {
            q.push(child);
        }
    }

    // Total number of nodes in the trie.
    int node_count = static_cast<int>(nodes.size());

    write_binary(out, node_count);

    /*
        Serialize every node.

        Format:

            is_terminal
            child_count

            child_char
            child_index

            child_char
            child_index

            ...

            id_count

            id
            id
            id
    */
    for(TrieNode* node : nodes) {
        write_binary(out, node->is_terminal_flag);

        int child_count = static_cast<int>(node->links.size());

        write_binary(out, child_count);

        /*
            Store children as:

                character
                index

            Example:

                'a' -> node 7

            becomes:

                'a'
                 7
        */
        for(const auto& [c, child] : node->links) {
            write_binary(out, c);

            int child_index = node_to_index[child];

            write_binary(out, child_index);
        }

        int id_count = static_cast<int>(node->file_ids.size());

        write_binary(out, id_count);

        for(int id : node->file_ids) {
            write_binary(out, id);
        }
    }
}

void PrefixTree::deserialize(std::istream& in) {
 // Delete existing trie if one exists.
    delete root;

    int node_count;
    read_binary(in, node_count);

    /*
        Create all nodes first.

        Suppose node_count = 3

        nodes[0]
        nodes[1]
        nodes[2]

        We don't know their relationships yet.
    */
    std::vector<TrieNode*> nodes(node_count); // create a vector of nodes

    for(int i = 0; i < node_count; i++)
    {
        nodes[i] = new TrieNode();
    }

    /*
        Now read data for every node.

        Since every node already exists,
        child indices can safely be converted
        into pointers.
    */
    for(int i = 0; i < node_count; i++)
    {
        TrieNode* node = nodes[i]; // read from vector created above

        read_binary(in, node->is_terminal_flag);

        int child_count;
        read_binary(in, child_count);

        for(int j = 0; j < child_count; j++)
        {
            char c;
            int child_index;

            read_binary(in, c);
            read_binary(in, child_index);

            /*
                Convert index back into pointer.

                Example:

                    child_index = 5

                becomes

                    nodes[5]
            */
            node->links[c] = nodes[child_index];
        }

        int id_count;
        read_binary(in, id_count);

        for(int j = 0; j < id_count; j++)
        {
            int id;
            read_binary(in, id);

            node->file_ids.insert(id);
        }
    }

    /*
        Root was serialized first (bfs),
        therefore it always gets index 0.
    */
    root = nodes[0];
}