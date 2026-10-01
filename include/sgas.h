/* include/sgas.h - Public header for the SGas language.
 * Only this header should be included by embedders.
 */
#ifndef SGAS_H
#define SGAS_H

#ifdef __cplusplus
extern "C" {
#endif

#define SGAS_VERSION_MAJOR 0
#define SGAS_VERSION_MINOR 1
#define SGAS_VERSION_PATCH 0
#define SGAS_VERSION "0.1.0"

/* Forward declaration of the VM. Full definition lives in src/vm/vm.h */
typedef struct VM VM;

/* Entry points ---------------------------------------------------------- */

/* Run a .sgas source file. Returns process exit code. */
int sgas_run_file(const char* path);

/* Run a .sgas source string. Returns 0 on success, non-zero on error. */
int sgas_run_string(const char* source);

/* Library version string. */
const char* sgas_version(void);

#ifdef __cplusplus
}
#endif

#endif /* SGAS_H */