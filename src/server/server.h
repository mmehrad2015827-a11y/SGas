#ifndef SGAS_SERVER_H
#define SGAS_SERVER_H

/* Start the SGas HTTP playground server.
 *   port       - TCP port to listen on (e.g. 8080)
 *   static_dir - path to the Aew/ folder (HTML/CSS/JS)
 * Returns 0 on clean shutdown, non-zero on error. */
int sgas_server_run(int port, const char* static_dir);

#endif