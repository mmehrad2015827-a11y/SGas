#include "server.h"
#include "../../include/sgas.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #include <io.h>
  typedef int socklen_io_t;
  #define CLOSESOCK  closesocket
  #define DUPFD      _dup
  #define DUPFD2     _dup2
  #define FILENO     _fileno
  #define CLOSEFD    _close
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  typedef int SOCKET;
  #define INVALID_SOCKET (-1)
  #define SOCKET_ERROR   (-1)
  #define CLOSESOCK      close
  #define DUPFD          dup
  #define DUPFD2         dup2
  #define FILENO         fileno
  #define CLOSEFD        close
#endif

#define BUF_SIZE      (64 * 1024)
#define MAX_REQ_BODY  (256 * 1024)

/* ---------- helpers ---------- */

static char* read_file(const char* path, size_t* out_size) {
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz < 0) { fclose(f); return NULL; }
    char* buf = (char*)malloc((size_t)sz + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t n = fread(buf, 1, (size_t)sz, f);
    buf[n] = '\0';
    fclose(f);
    if (out_size) *out_size = n;
    return buf;
}

static const char* content_type(const char* path) {
    const char* ext = strrchr(path, '.');
    if (!ext) return "application/octet-stream";
    if (strcmp(ext, ".html") == 0) return "text/html; charset=utf-8";
    if (strcmp(ext, ".css")  == 0) return "text/css; charset=utf-8";
    if (strcmp(ext, ".js")   == 0) return "application/javascript; charset=utf-8";
    if (strcmp(ext, ".json") == 0) return "application/json; charset=utf-8";
    return "text/plain; charset=utf-8";
}

static void send_all(SOCKET s, const char* data, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        int n = send(s, data + sent, (int)(len - sent), 0);
        if (n <= 0) return;
        sent += (size_t)n;
    }
}

static void http_response(SOCKET s, int status, const char* ctype,
                          const char* body, size_t blen) {
    const char* stext = (status == 200) ? "OK"
                       : (status == 404) ? "Not Found"
                       : (status == 400) ? "Bad Request"
                       : (status == 405) ? "Method Not Allowed"
                       : "Internal Server Error";
    char head[512];
    int hlen = snprintf(head, sizeof(head),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Cache-Control: no-store\r\n"
        "Connection: close\r\n"
        "\r\n",
        status, stext, ctype, blen);
    send_all(s, head, (size_t)hlen);
    if (body && blen) send_all(s, body, blen);
}

/* ---------- capture SGas output into a buffer ---------- */

typedef struct {
    char*  data;
    size_t len;
    size_t cap;
} Buf;

static void buf_init(Buf* b) { b->data = NULL; b->len = 0; b->cap = 0; }
static void buf_free(Buf* b) { free(b->data); b->data = NULL; b->len = 0; b->cap = 0; }

static void buf_append(Buf* b, const char* s, size_t n) {
    if (b->len + n + 1 > b->cap) {
        size_t nc = b->cap ? b->cap * 2 : 1024;
        while (nc < b->len + n + 1) nc *= 2;
        b->data = (char*)realloc(b->data, nc);
        b->cap = nc;
    }
    memcpy(b->data + b->len, s, n);
    b->len += n;
    b->data[b->len] = '\0';
}

/* Run SGas source and capture stdout+stderr into `out`.
 * Returns the SGas exit code. */
static int run_captured(const char* source, Buf* out) {
    /* Save fds */
    int saved_out = DUPFD(FILENO(stdout));
    int saved_err = DUPFD(FILENO(stderr));
    if (saved_out < 0 || saved_err < 0) {
        if (saved_out >= 0) CLOSEFD(saved_out);
        if (saved_err >= 0) CLOSEFD(saved_err);
        return -1;
    }

    FILE* tmp = tmpfile();
    if (!tmp) {
        CLOSEFD(saved_out);
        CLOSEFD(saved_err);
        return -1;
    }
    int tmpfd = FILENO(tmp);

    fflush(stdout);
    fflush(stderr);
    DUPFD2(tmpfd, FILENO(stdout));
    DUPFD2(tmpfd, FILENO(stderr));

    int rc = sgas_run_string(source);

    fflush(stdout);
    fflush(stderr);
    DUPFD2(saved_out, FILENO(stdout));
    DUPFD2(saved_err, FILENO(stderr));
    CLOSEFD(saved_out);
    CLOSEFD(saved_err);

    /* Read captured text */
    fseek(tmp, 0, SEEK_END);
    long n = ftell(tmp);
    fseek(tmp, 0, SEEK_SET);
    if (n < 0) n = 0;
    if (n > 0) {
        char* data = (char*)malloc((size_t)n + 1);
        if (data) {
            size_t got = fread(data, 1, (size_t)n, tmp);
            data[got] = '\0';
            buf_append(out, data, got);
            free(data);
        }
    }
    fclose(tmp);
    return rc;
}

/* ---------- HTTP handler ---------- */

static void handle_client(SOCKET client, const char* static_dir) {
    char req[BUF_SIZE];
    int  total = 0;
    int  n;

    /* read request headers + body in one go (loop until headers end) */
    while ((n = recv(client, req + total, (int)sizeof(req) - 1 - total, 0)) > 0) {
        total += n;
        req[total] = '\0';
        if (strstr(req, "\r\n\r\n")) break;
        if (total >= (int)sizeof(req) - 1) break;
    }
    if (total <= 0) return;

    /* Parse first line: METHOD PATH VERSION */
    char method[16] = {0};
    char path[512]  = {0};
    sscanf(req, "%15s %511s", method, path);

    /* find header/body split */
    char* body_start = strstr(req, "\r\n\r\n");
    size_t header_len = body_start ? (size_t)(body_start - req) + 4 : (size_t)total;
    size_t body_len   = (size_t)total - header_len;
    if (body_len > MAX_REQ_BODY) body_len = MAX_REQ_BODY;

    /* -------- POST /run -------- */
    if (strcmp(method, "POST") == 0 && strcmp(path, "/run") == 0) {
        /* read remaining body if not fully received */
        char* body = (char*)malloc(MAX_REQ_BODY + 1);
        if (!body) { http_response(client, 500, "text/plain", "oom", 3); return; }

        size_t have = body_len;
        if (have > 0) memcpy(body, req + header_len, have);
        while (have < MAX_REQ_BODY) {
            int r = recv(client, body + have, (int)(MAX_REQ_BODY - have), 0);
            if (r <= 0) break;
            have += (size_t)r;
            if (have >= MAX_REQ_BODY) break;
        }
        body[have] = '\0';

        Buf out; buf_init(&out);
        int rc = run_captured(body, &out);
        free(body);

        if (!out.data) buf_append(&out, "", 0);

        int status = (rc == 0) ? 200 : 200; /* both return body */
        http_response(client, status, "text/plain; charset=utf-8",
                      out.data ? out.data : "", out.len);
        buf_free(&out);
        return;
    }

    /* -------- GET static files -------- */
    if (strcmp(method, "GET") != 0) {
        http_response(client, 405, "text/plain", "Method not allowed", 18);
        return;
    }

    /* default */
    if (strcmp(path, "/") == 0) strcpy(path, "/index.html");

    /* block path traversal */
    if (strstr(path, "..")) {
        http_response(client, 400, "text/plain", "Bad request", 11);
        return;
    }

    /* build full path */
    char full[1024];
    snprintf(full, sizeof(full), "%s%s", static_dir, path);

    size_t flen = 0;
    char* data = read_file(full, &flen);
    if (!data) {
        http_response(client, 404, "text/plain", "Not found", 9);
        return;
    }
    http_response(client, 200, content_type(full), data, flen);
    free(data);
}

/* ---------- server main loop ---------- */

int sgas_server_run(int port, const char* static_dir) {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        fprintf(stderr, "WSAStartup failed\n");
        return 1;
    }
#endif

    SOCKET listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener == INVALID_SOCKET) {
        fprintf(stderr, "socket() failed\n");
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    int yes = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR,
               (const char*)&yes, sizeof(yes));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); /* فقط localhost */
    addr.sin_port        = htons((unsigned short)port);

    if (bind(listener, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        fprintf(stderr, "bind() failed (port %d در حال استفاده؟)\n", port);
        CLOSESOCK(listener);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    if (listen(listener, 8) == SOCKET_ERROR) {
        fprintf(stderr, "listen() failed\n");
        CLOSESOCK(listener);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    printf("\n  SGas Playground\n");
    printf("  ──────────────────────────────────\n");
    printf("  آدرس:    http://localhost:%d/\n", port);
    printf("  فایل‌ها: %s\n", static_dir);
    printf("  توقف:    Ctrl+C\n\n");
    fflush(stdout);

    for (;;) {
        SOCKET client = accept(listener, NULL, NULL);
        if (client == INVALID_SOCKET) {
            /* interrupted - exit */
            break;
        }
        handle_client(client, static_dir);
        CLOSESOCK(client);
    }

    CLOSESOCK(listener);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}