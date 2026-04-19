#include <unistd.h>
#include <stdlib.h>
#include <string.h>

// Build JSON response dynamically
char* build_json_response(const char *name, int count) {
    // Allocate buffer for JSON
    char *json = malloc(256);
    if (!json) return NULL;
    
    // Build JSON string
    strcpy(json, "{\"message\":\"Hello, ");
    strcat(json, name);
    strcat(json, "!\",\"count\":");
    
    // Convert count to string
    char num[20];
    int i = 0;
    int n = count;
    do {
        num[i++] = '0' + (n % 10);
        n /= 10;
    } while (n > 0);
    num[i] = '\0';
    
    // Reverse number
    for (int j = 0; j < i / 2; j++) {
        char tmp = num[j];
        num[j] = num[i - j - 1];
        num[i - j - 1] = tmp;
    }
    
    strcat(json, num);
    strcat(json, "}");
    
    return json;
}

int main() {
    // Simulate multiple requests with dynamic allocation
    const char *names[] = {"Alice", "Bob", "Charlie"};
    
    write(1, "HTTP/1.1 200 OK\r\n", 17);
    write(1, "Content-Type: application/json\r\n\r\n", 35);
    write(1, "[", 1);
    
    for (int i = 0; i < 3; i++) {
        char *response = build_json_response(names[i], (i + 1) * 10);
        if (response) {
            write(1, response, strlen(response));
            free(response);  // Clean up
        }
        if (i < 2) write(1, ",", 1);
    }
    
    write(1, "]\n", 2);
    return 0;
}
