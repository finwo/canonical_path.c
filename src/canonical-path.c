#ifdef __cplusplus
extern "C" {
#endif

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#ifndef NULL
#define NULL ((void*)0)
#endif

#if defined(_WIN32) || defined(_WIN64)
#define REALPATH(x) _fullpath(NULL, x, PATH_MAX)
#define STRDUP(x) _strdup(x)
#else
#define REALPATH(x) realpath(x, NULL)
#define STRDUP(x) strdup(x)
#endif

#include "canonical-path.h"

// Origin: https://stackoverflow.com/a/60923396/2928176
// Modified to work cross-platform

////////////////////////////////////////////////////////////////////////////////
// Return the input path in a canonical form. This is achieved by expanding all
// symbolic links, resolving references to "." and "..", and removing duplicate
// "/" characters.
//
// If the file exists, its path is canonicalized and returned. If the file,
// or parts of the containing directory, do not exist, path components are
// removed from the end until an existing path is found. The remainder of the
// path is then appended to the canonical form of the existing path,
// and returned. Consequently, the returned path may not exist. The portion
// of the path which exists, however, is represented in canonical form.
//
// If successful, this function returns a C-string, which needs to be freed by
// the caller using free().
//
// ARGUMENTS:
//   file_path
//   File path, whose canonical form to return.
//
// RETURNS:
//   On success, returns the canonical path to the file, which needs to be freed
//   by the caller.
//
//   On failure, returns NULL.
////////////////////////////////////////////////////////////////////////////////
char *canonical_path(const char *file_path) {
  char *canonical_file_path  = NULL;
  unsigned int file_path_len = strlen(file_path);

  if (file_path_len > 0) {
    canonical_file_path = REALPATH(file_path);
    if (canonical_file_path == NULL && errno == ENOENT) {
      // The file was not found. Back up to a segment which exists,
      // and append the remainder of the path to it.
      char *file_path_copy = NULL;
      if (file_path[0] == '/'                ||
          (strncmp(file_path, "./", 2) == 0) ||
          (strncmp(file_path, "../", 3) == 0)
      ) {
        // Absolute path, or path starts with "./" or "../"
        file_path_copy = STRDUP(file_path);
      } else {
        // Relative path
        file_path_copy = (char*)malloc(strlen(file_path) + 3);
        strcpy(file_path_copy, "./");
        strcat(file_path_copy, file_path);
      }

      // Remove path components from the end, until an existing path is found
      for (int char_idx = strlen(file_path_copy) - 1;
           char_idx >= 0 && canonical_file_path == NULL;
           --char_idx
      ) {
        if (file_path_copy[char_idx] == '/') {
          // Remove the slash character
          file_path_copy[char_idx] = '\0';

          canonical_file_path = REALPATH(file_path_copy);
          if (canonical_file_path != NULL) {
            // An existing path was found. Append the remainder of the path
            // to a canonical form of the existing path.
            char *combined_file_path = (char*)malloc(strlen(canonical_file_path) + strlen(file_path_copy + char_idx + 1) + 2);
            strcpy(combined_file_path, canonical_file_path);
            strcat(combined_file_path, "/");
            strcat(combined_file_path, file_path_copy + char_idx + 1);
            free(canonical_file_path);
            canonical_file_path = combined_file_path;
          } else {
            // The path segment does not exist. Replace the slash character
            // and keep trying by removing the previous path component.
            file_path_copy[char_idx] = '/';
          }
        }
      }

      free(file_path_copy);
    }
  }

  return canonical_file_path;
}

#ifdef __cplusplus
} // extern "C"
#endif
