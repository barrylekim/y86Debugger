#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/uio.h>
#include <unistd.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <strings.h>

#include "instruction.h"
#include "printRoutines.h"

#define ERROR_RETURN -1
#define SUCCESS 0

#define MAX_LINE 256

static void addBreakpoint(uint64_t address);
static void deleteBreakpoint(uint64_t address);
static void deleteAllBreakpoints(void);
static int  hasBreakpoint(uint64_t address);

struct Node
{
  unsigned int data;
  struct Node *next;
};

struct LinkedList {
  struct Node *head;
  struct Node *tail;
  int size;
};

struct LinkedList *breakpoints;

int main(int argc, char **argv) {
  
  int fd;
  struct stat st;
  
  machine_state_t state;
  y86_instruction_t nextInstruction;
  memset(&state, 0, sizeof(state));

  char line[MAX_LINE + 1], previousLine[MAX_LINE + 1] = "";
  char *command, *parameters;
  int c;

  // Verify that the command line has an appropriate number of
  // arguments
  if (argc < 2 || argc > 3) {
    fprintf(stderr, "Usage: %s InputFilename [startingPC]\n", argv[0]);
    return ERROR_RETURN;
  }

  // First argument is the file to read, attempt to open it for
  // reading and verify that the open did occur.
  fd = open(argv[1], O_RDONLY);

  if (fd < 0) {
    fprintf(stderr, "Failed to open %s: %s\n", argv[1], strerror(errno));
    return ERROR_RETURN;
  }

  if (fstat(fd, &st) < 0) {
    fprintf(stderr, "Failed to stat %s: %s\n", argv[1], strerror(errno));
    close(fd);
    return ERROR_RETURN;
  }

  state.programSize = st.st_size;

  // If there is a 2nd argument present it is an offset so convert it
  // to a numeric value.
  if (3 <= argc) {
    errno = 0;
    state.programCounter = strtoul(argv[2], NULL, 0);
    if (errno != 0) {
      perror("Invalid program counter on command line");
      close(fd);
      return ERROR_RETURN;
    }
    if (state.programCounter > state.programSize) { 
      fprintf(stderr, "Program counter on command line (%lu) "
	      "larger than file size (%lu).\n",
	      state.programCounter, state.programSize);
      close(fd);
      return ERROR_RETURN;     
    }
  }

  // Maps the entire file to memory. This is equivalent to reading the
  // entire file using functions like fread, but the data is only
  // retrieved on demand, i.e., when the specific region of the file
  // is needed.
  state.programMap = mmap(NULL, state.programSize, PROT_READ | PROT_WRITE,
			  MAP_PRIVATE, fd, 0);
  if (state.programMap == MAP_FAILED) {
    fprintf(stderr, "Failed to map %s: %s\n", argv[1], strerror(errno));
    close(fd);
    return ERROR_RETURN;
  }

  // Move to first non-zero byte
  while (!state.programMap[state.programCounter]) state.programCounter++;
  
  printf("# Opened %s, starting PC 0x%lX\n", argv[1], state.programCounter);

  fetchInstruction(&state, &nextInstruction);
  printInstruction(stdout, &nextInstruction);

  // List of break points

  breakpoints = malloc(sizeof(struct LinkedList));
  breakpoints->head = NULL;
  breakpoints->tail = NULL;
  breakpoints->size = 0;
  
  while(1) {
    
    // Show prompt, but only if input comes from a terminal
    if (isatty(STDIN_FILENO))
      printf("> ");

    // Read one line, if EOF break loop
    if (!fgets(line, sizeof(line), stdin))
      break;

    // If line could not be read entirely
    if (!strchr(line, '\n')) {
      // Read to the end of the line
      while ((c = fgetc(stdin)) != EOF && c != '\n');
      if (c == '\n') {
	printErrorCommandTooLong(stdout);
	continue;
      }
      else {
	// In this case there is an EOF at the end of a line.
	// Process line as usual.
      }
    }

    // Obtain the command name, separate it from the arguments.
    command = strtok(line, " \t\n\f\r\v");
    // If line is blank, repeat previous command.
    if (!command) {
      strcpy(line, previousLine);
      command = strtok(line, " \t\n\f\r\v");
      // If there's no previous line, do nothing.
      if (!command) continue;
    }

    // Get the arguments to the command, if provided.
    parameters = strtok(NULL, "\n\r");

    sprintf(previousLine, "%s %s\n", command, parameters ? parameters : "");

    /* THIS PART TO BE COMPLETED BY THE STUDENT */
    // Check what command is inputed
    if (!strcasecmp(command, "quit") || !strcasecmp(command, "exit")) {
      if (parameters != NULL) {
        printErrorCommandTooLong(stdout);
      } else {
        // Free all resources and exit program
        break;
      }
    } else if (!strcasecmp(command, "step")) {
      if (parameters != NULL) {
        printErrorCommandTooLong(stdout);
      } else {
        if (executeInstruction(&state, &nextInstruction) && fetchInstruction(&state, &nextInstruction)) {
          printInstruction(stdout, &nextInstruction);
        } else {
          if (nextInstruction.icode == I_INVALID) {
            printErrorInvalidInstruction(stdout,&nextInstruction);
          } else if (nextInstruction.icode == I_TOO_SHORT) {
            printErrorShortInstruction(stdout, &nextInstruction);
          } else if (nextInstruction.icode == I_HALT) {
            printInstruction(stdout, &nextInstruction);
          }
        }
      }
    } else if (!strcasecmp(command, "run")) {
      if (parameters != NULL) {
        printErrorCommandTooLong(stdout);
      } else {
        while (!hasBreakpoint(state.programCounter)) { 
          // Execute
          int exec = executeInstruction(&state, &nextInstruction);
          if (exec == 0) {
            // Print error if invalid
            printErrorInvalidInstruction(stdout, &nextInstruction);
            break;
          } else {
            // Fetch next instruction
            int fetch = fetchInstruction(&state, &nextInstruction);
            // Check if valid
            if (fetch == 0) {
              if (nextInstruction.icode == I_INVALID) {
                printErrorInvalidInstruction(stdout,&nextInstruction);
              } else if (nextInstruction.icode == I_TOO_SHORT) {
                printErrorShortInstruction(stdout, &nextInstruction);
              } else if (nextInstruction.icode == I_HALT) {
                printInstruction(stdout, &nextInstruction);
                break;
              }
              break;
            } else {
              printInstruction(stdout, &nextInstruction);
            }
          }
        }
      }
    } else if (!strcasecmp(command, "next")) {
      if (parameters != NULL) {
        printErrorCommandTooLong(stdout);
      } else {
        if (nextInstruction.icode == I_CALL) {
          // Remember stack pointer
          int sp = state.registerFile[R_RSP];
          while (!hasBreakpoint(state.programCounter)) { 
            // Execute
            int exec = executeInstruction(&state, &nextInstruction);
            if (exec == 0) {
              // Print error if invalid
              printErrorInvalidInstruction(stdout, &nextInstruction);
              break;
            } else {
              if (sp == state.registerFile[R_RSP]) {
                // Returned, stop execution
                break;
              }
              // Fetch next instruction
              int fetch = fetchInstruction(&state, &nextInstruction);
              // Check if valid
              if (fetch == 0) {
                if (nextInstruction.icode == I_INVALID) {
                  printErrorInvalidInstruction(stdout,&nextInstruction);
                } else if (nextInstruction.icode == I_TOO_SHORT) {
                  printErrorShortInstruction(stdout, &nextInstruction);
                } else if (nextInstruction.icode == I_HALT) {
                  printInstruction(stdout, &nextInstruction);
                  break;
                }
                break;
              } else {
                printInstruction(stdout, &nextInstruction);
              }
            }
          }
        } else {
          // Same as step if not CALL
          if (executeInstruction(&state, &nextInstruction) && fetchInstruction(&state, &nextInstruction)) {
            printInstruction(stdout, &nextInstruction);
          } else {
            if (nextInstruction.icode == I_INVALID) {
              printErrorInvalidInstruction(stdout,&nextInstruction);
            } else if (nextInstruction.icode == I_TOO_SHORT) {
              printErrorShortInstruction(stdout, &nextInstruction);
            } else if (nextInstruction.icode == I_HALT) {
              printInstruction(stdout, &nextInstruction);
            }
          }
        }
      }
    } else if (!strcasecmp(command, "jump")) {
      printf("%d\n", isNumber(parameters));
      if (parameters == NULL || !(isNumber(parameters))) {
        printErrorInvalidCommand(stdout, command, parameters);
      } else {
        // Change PC to given address
        state.programCounter = (uint64_t) atoi(parameters);
        // Check if valid
        if (!fetchInstruction(&state, &nextInstruction)) {
          if (nextInstruction.icode == I_HALT) {
            printInstruction(stdout, &nextInstruction);
          } else {
            printErrorInvalidInstruction(stdout, &nextInstruction);
          }
        } else {
          printInstruction(stdout, &nextInstruction);
        }
      }
    } else if (!strcasecmp(command, "break")) {
      if (parameters == NULL || !(isNumber(parameters))) {
        printErrorInvalidCommand(stdout, command, parameters);
      } else {
        addBreakpoint((uint64_t) atoi(parameters));
      }
    } else if (!strcasecmp(command, "delete")) {
      if (parameters == NULL || !(isNumber(parameters))) {
        printErrorInvalidCommand(stdout, command, parameters);
      } else {
        deleteBreakpoint((uint64_t) atoi(parameters));
      }
    } else if (!strcasecmp(command, "examine")) {
      if (parameters == NULL || !(isNumber(parameters))) {
        printErrorInvalidCommand(stdout, command, parameters);
      } else {
        // Print memmory at given location
        printMemoryValueQuad(stdout, &state, (uint64_t) atoi(parameters));
      }
    } else if (!strcasecmp(command, "registers")) {
      // Loop through registers and print
      int reg_num = 0;
      while (reg_num < 15) {
        printRegisterValue(stdout, &state, (y86_register_t) reg_num);
        reg_num++;
      }
    } else if (!strcasecmp(command, "listbreaks")) {
      struct Node *curr = breakpoints->head;
      while (curr != NULL) {
        printf("%d\n", curr->data);
        curr = curr->next;
      }
    } else {
      printErrorInvalidCommand(stdout, command, parameters);
    }
  }
  
  deleteAllBreakpoints();
  free(breakpoints);
  munmap(state.programMap, state.programSize);
  close(fd);
  return SUCCESS;
}

/* Adds an address to the list of breakpoints. If the address is
 * already in the list, it is not added again. */
static void addBreakpoint(uint64_t address) {
    if (!hasBreakpoint(address)) {
    struct Node *node = malloc(sizeof(struct Node));
    node->data = address;
    node->next = NULL;

    if (breakpoints->size != 0) {
      // Add to existing list
      breakpoints->tail->next = node;
      breakpoints->tail = node;
      breakpoints->size++;
    } else {
      // Empty list
      breakpoints->head = node;
      breakpoints->tail = node;
      breakpoints->size++;
    } 
  }
}

/* Deletes an address from the list of breakpoints. If the address is
 * not in the list, nothing happens. */
static void deleteBreakpoint(uint64_t address) {
  struct Node *curr = breakpoints->head;
  struct Node *prev;
  
  if (curr->data == address) {
    breakpoints->head = curr->next;
    breakpoints->size--;
    free(curr);
    return;
  }

  // Search list and delete
  while (curr != NULL) {
    if (curr->data == address) {
      prev->next = curr->next;
      breakpoints->size--;
      if (breakpoints->tail == curr) {
        breakpoints->tail = prev;
      }
      free(curr);
      return;
    } else {
      prev = curr;
      curr = curr->next;
    }
  }
}

/* Deletes and frees all breakpoints. */
static void deleteAllBreakpoints(void) {
  if (breakpoints->head != NULL) {
    // Free the list
    struct Node *curr = breakpoints->head;
    struct Node *temp;
    while (curr != NULL) {
      temp = curr->next;
      free(curr);
      curr = temp;
    } 
  }
}

/* Returns true (non-zero) if the address corresponds to a breakpoint
 * in the list of breakpoints, or false (zero) otherwise. */
static int hasBreakpoint(uint64_t address) {
  if (breakpoints->size == 0) {
    return 0;
  } else {
    // Serach in list
    struct Node *curr = breakpoints->head;
    while (curr != NULL) {
      if (curr->data == address) {
        return 1;
      }
      curr = curr->next;
    }
    return 0;
  }
}

int isNumber(char *str) {
  for (int i = 0; i < strlen(str); i++) {
    if (!isdigit(str[i])) {
      return 0;
    }
  }
  return 1;
}
