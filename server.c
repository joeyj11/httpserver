#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <arpa/inet.h>
#include <sys/socket.h>

/**
 * Project 1 starter code
 * All parts needed to be changed/added are marked with TODO
 */

#define BUFFER_SIZE 1024
#define DEFAULT_SERVER_PORT 8081
#define DEFAULT_REMOTE_HOST "127.0.0.1"
#define DEFAULT_REMOTE_PORT 5001

struct server_app {
    // Parameters of the server
    // Local port of HTTP server
    uint16_t server_port;

    // Remote host and port of remote proxy
    char *remote_host;
    uint16_t remote_port;
};

// The following function is implemented for you and doesn't need
// to be change
void parse_args(int argc, char *argv[], struct server_app *app);

// The following functions need to be updated
bool need_proxy(char* file_name);
void handle_request(struct server_app *app, int client_socket);
void serve_local_file(int client_socket, const char *path);
void proxy_remote_file(struct server_app *app, int client_socket, const char *path);

// The main function is provided and no change is needed
int main(int argc, char *argv[])
{
    struct server_app app;
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    int ret;

    parse_args(argc, argv, &app);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(app.server_port);

    // The following allows the program to immediately bind to the port in case
    // previous run exits recently
    int optval = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 10) == -1) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", app.server_port);

    while (1) {
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket == -1) {
            perror("accept failed");
            continue;
        }
        
        printf("Accepted connection from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        handle_request(&app, client_socket);
        close(client_socket);
    }

    close(server_socket);
    return 0;
}

void parse_args(int argc, char *argv[], struct server_app *app)
{
    int opt;

    app->server_port = DEFAULT_SERVER_PORT;
    app->remote_host = NULL;
    app->remote_port = DEFAULT_REMOTE_PORT;

    while ((opt = getopt(argc, argv, "b:r:p:")) != -1) {
        switch (opt) {
        case 'b':
            app->server_port = atoi(optarg);
            break;
        case 'r':
            app->remote_host = strdup(optarg);
            break;
        case 'p':
            app->remote_port = atoi(optarg);
            break;
        default: /* Unrecognized parameter or "-?" */
            fprintf(stderr, "Usage: server [-b local_port] [-r remote_host] [-p remote_port]\n");
            exit(-1);
            break;
        }
    }

    if (app->remote_host == NULL) {
        app->remote_host = strdup(DEFAULT_REMOTE_HOST);
    }
}

bool need_proxy(char* file_name) {
    if (strstr(file_name, ".ts") != NULL && strlen(strstr(file_name, ".ts")) == 3) { // call proxy if the user wants to request .ts file
        return true;
    }
    else {
        return false;
    }

}

void handle_request(struct server_app *app, int client_socket) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    // Read the request from HTTP client
    // Note: This code is not ideal in the real world because it
    // assumes that the request header is small enough and can be read
    // once as a whole.
    // However, the current version suffices for our testing.
    bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read <= 0) {
        return;  // Connection closed or error
    }

    buffer[bytes_read] = '\0';
    // copy buffer to a new string
    char *request = malloc(strlen(buffer) + 1);
    strcpy(request, buffer);
    free(request);
    // TODO: Parse the header and extract essential fields, e.g. file name
    // Hint: if the requested path is "/" (root), default to index.html
    char *request_line = strtok(buffer, "\r\n");  // Get the first line
    char *method = strtok(request_line, " ");
    char *uri = strtok(NULL, " ");  // Get the second token, URI
    if (uri) {
        char *out_ptr = uri;
        for (const char *ptr = uri; *ptr; ptr++) {
            if (*ptr == '%' && *(ptr + 1) == '2' && *(ptr + 2) == '0') {
                *out_ptr++ = ' ';  
                ptr += 2; // Skip next two characters
            } else {
                *out_ptr++ = *ptr;
            }
        }
        *out_ptr = '\0'; // Null-terminate the decoded URI
    }

    // Default file name
    char file_name[BUFFER_SIZE] = "index.html";
    if (uri && strcmp(uri, "/") != 0) {
        // Remove leading '/' and copy the rest to file_name
        strncpy(file_name, uri + 1, BUFFER_SIZE - 1);
        file_name[BUFFER_SIZE - 1] = '\0';  // Ensure null-termination
    }

    printf("Filename %s\n", file_name);

    // Assuming you have a function to decide whether to proxy

    // TODO: Implement proxy and call the function under condition
    // specified in the spec
    if (need_proxy(file_name)) {
        proxy_remote_file(app, client_socket, file_name);
    } else {
        serve_local_file(client_socket, file_name);
    }
}

void serve_local_file(int client_socket, const char *path) {
    // TODO: Properly implement serving of local files
    // The following code returns a dummy response for all requests
    // but it should give you a rough idea about what a proper response looks like
    // What you need to do 
    // (when the requested file exists):
    // * Open the requested file
    // * Build proper response headers (see details in the spec), and send them
    // * Also send file content
    // (When the requested file does not exist):
    // * Generate a correct response

    // const char *mime_type = "application/octet-stream"; // default to binary data
    // const char *extension = strrchr(path, '.');

    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        // File not found, send 404
        char *response = "HTTP/1.0 404 Not Found\r\n"
                         "Content-Length: 0\r\n"
                         "Connection: close\r\n\r\n";
        send(client_socket, response, strlen(response), 0);
    } else {
        // File found, prepare to send
        fseek(file, 0, SEEK_END);
        long fsize = ftell(file);
        fseek(file, 0, SEEK_SET); // same as rewind(f);

        const char *mime_type;
        // Determine the MIME type from the file extension
        const char *ext = strrchr(path, '.');
        if (ext) {
            if (strcmp(ext, ".html") == 0) mime_type = "text/html";
            else if (strcmp(ext, ".txt") == 0) mime_type = "text/plain";
            else if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) mime_type = "image/jpeg";
            else {
                char *response = "HTTP/1.0 404 Not Found\r\n"
                         "Content-Length: 0\r\n"
                         "Connection: close\r\n\r\n";
                send(client_socket, response, strlen(response), 0);
                return;
            } // default to binary data
        } else {
            mime_type = "application/octet-stream"; // default to binary data
        }

        char header[512];
        snprintf(header, sizeof(header),
                 "HTTP/1.0 200 OK\r\n"
                 "Content-Type: %s\r\n"
                 "Content-Length: %ld\r\n"
                 "Connection: close\r\n\r\n", mime_type, fsize);
        send(client_socket, header, strlen(header), 0);

        // Send file content
        char *buffer = malloc(fsize);
        if (buffer) {
            fread(buffer, 1, fsize, file);
            send(client_socket, buffer, fsize, 0);
            free(buffer);
        } else {
            // Handle error for malloc here
        }
        fclose(file);
    }
}

void proxy_remote_file(struct server_app *app, int client_socket, const char *request) {
    // TODO: Implement proxy request and replace the following code
    // What's needed:
    // * Connect to remote server (app->remote_server/app->remote_port)
    // * Forward the original request to the remote server
    // * Pass the response from remote server back
    // Bonus:
    // * When connection to the remote server fail, properly generate
    // HTTP 502 "Bad Gateway" response
    int remote_socket;
    struct sockaddr_in remote_addr;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    printf("Inside of proxy\n");
    // Only proxy for .ts files
    if (strstr(request, ".ts") == NULL) {
        char *bad_request_response = "HTTP/1.0 400 Bad Request\r\nConnection: close\r\n\r\n";
        send(client_socket, bad_request_response, strlen(bad_request_response), 0);
        return;
    }

    // Create socket for communication with the remote video server
    remote_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (remote_socket == -1) {
        perror("Cannot create socket");
        char *bad_gateway_response = "HTTP/1.0 502 Bad Gateway\r\nConnection: close\r\n\r\n";
        send(client_socket, bad_gateway_response, strlen(bad_gateway_response), 0);
        return;
    }

    // Set up the remote video server address structure
    memset(&remote_addr, 0, sizeof(remote_addr));
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(app->remote_port);
    printf("Attempting to connect to remote server at %s:%d\n", app->remote_host, app->remote_port);
    if (inet_pton(AF_INET, app->remote_host, &remote_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        close(remote_socket);
        char *bad_gateway_response = "HTTP/1.0 502 Bad Gateway\r\nConnection: close\r\n\r\n";
        send(client_socket, bad_gateway_response, strlen(bad_gateway_response), 0);
        return;
    }

    // Connect to the remote video server
    if (connect(remote_socket, (struct sockaddr *)&remote_addr, sizeof(remote_addr)) < 0) {
        perror("Connection Failed");
        close(remote_socket);
        char *bad_gateway_response = "HTTP/1.0 502 Bad Gateway\r\nConnection: close\r\n\r\n";
        send(client_socket, bad_gateway_response, strlen(bad_gateway_response), 0);
        return;
    }

    // Forward the original request to the video server
    snprintf(buffer, sizeof(buffer), "GET %s HTTP/1.0\r\nHost: %s\r\n\r\n", request, app->remote_host);
    if (send(remote_socket, buffer, strlen(buffer), 0) < 0) {
        perror("Failed to send to video server");
        close(remote_socket);
        char *bad_gateway_response = "HTTP/1.0 502 Bad Gateway\r\nConnection: close\r\n\r\n";
        send(client_socket, bad_gateway_response, strlen(bad_gateway_response), 0);
        return;
    }

    // Read the response from the video server and forward it to the client
    while ((bytes_read = recv(remote_socket, buffer, BUFFER_SIZE, 0)) > 0) {
        if (send(client_socket, buffer, bytes_read, 0) < 0) {
            perror("Failed to send to client");
            break;
        }
    }

    // Cleanup
    close(remote_socket);
    // char response[] = "HTTP/1.0 501 Not Implemented\r\n\r\n";
    // send(client_socket, response, strlen(response), 0);
}