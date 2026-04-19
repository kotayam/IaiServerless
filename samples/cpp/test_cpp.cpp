#include <string>
#include <vector>
#include <unistd.h>

void print(const char* s) {
    size_t len = 0;
    while (s[len]) len++;
    write(1, s, len);
}

int main() {
    print("Testing C++ STL...\n");
    
    // Test std::string
    std::string str = "Hello from C++!";
    print("std::string: ");
    write(1, str.c_str(), str.length());
    write(1, "\n", 1);
    
    str += " More text.";
    print("After append: ");
    write(1, str.c_str(), str.length());
    write(1, "\n", 1);
    
    // Test std::vector
    std::vector<int> vec;
    vec.push_back(10);
    vec.push_back(20);
    vec.push_back(30);
    
    print("std::vector: [");
    for (size_t i = 0; i < vec.size(); i++) {
        char buf[20];
        int val = vec[i];
        int len = 0;
        do {
            buf[len++] = '0' + (val % 10);
            val /= 10;
        } while (val > 0);
        // Reverse
        for (int j = 0; j < len / 2; j++) {
            char tmp = buf[j];
            buf[j] = buf[len - j - 1];
            buf[len - j - 1] = tmp;
        }
        write(1, buf, len);
        if (i < vec.size() - 1) write(1, ", ", 2);
    }
    write(1, "]\n", 2);
    
    print("C++ test complete!\n");
    return 0;
}
