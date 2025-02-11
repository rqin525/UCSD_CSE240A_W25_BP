//========================================================//
//  CSE 240a Branch Lab                                   //
//                                                        //
//  Students need to implement various Branch Predictors  //
//========================================================//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "predictor.h"

// temp solution to compile
#include <iostream>
#include <sstream> // For std::istringstream

FILE *stream;
char *buf = NULL;
size_t len = 0;

// Print out the Usage information to stderr
//
void usage()
{
  fprintf(stderr, "Usage: predictor <options> [<trace>]\n");
  fprintf(stderr, "       bunzip2 -kc trace.bz2 | predictor <options>\n");
  fprintf(stderr, " Options:\n");
  fprintf(stderr, " --help       Print this message\n");
  fprintf(stderr, " --verbose    Print predictions on stdout\n");
  fprintf(stderr, " --<type>     Branch prediction scheme:\n");
  fprintf(stderr, "    static\n"
                  "    gshare\n"
                  "    tournament\n"
                  "    custom\n");
}

// Process an option and update the predictor
// configuration variables accordingly
//
// Returns True if Successful
//
int handle_option(char *arg)
{
  if (!strcmp(arg, "--static"))
  {
    bpType = STATIC;
  }
  else if (!strncmp(arg, "--gshare", 8))
  {
    bpType = GSHARE;
  }
  else if (!strncmp(arg, "--tournament", 12))
  {
    bpType = TOURNAMENT;
  }
  else if (!strncmp(arg, "--custom", 8))
  {
    bpType = CUSTOM;
  }
  else if (!strcmp(arg, "--verbose"))
  {
    verbose = 1;
  }
  else
  {
    return 0;
  }

  return 1;
}

// Reads a line from the input stream and extracts the
// PC and Outcome of a branch
//
// Returns True if Successful
//
/*int read_branch(uint32_t *pc, uint32_t *target, uint32_t *outcome, uint32_t *condition, uint32_t *call, uint32_t *ret, uint32_t *direct)
{
  if (getline(&buf, &len, stream) == -1)
  {
    return 0;
  }

  sscanf(buf, "0x%x\t0x%x\t%d\t%d\t%d\t%d\t%d\n", pc, target, outcome, condition, call, ret, direct);

  return 1;
}*/

// Function to read and parse a branch line
int read_branch(uint32_t* pc, uint32_t* target, uint32_t* outcome, uint32_t* condition,
                uint32_t* call, uint32_t* ret, uint32_t* direct) {
    static std::string buf; // Static buffer for reading lines
    static std::istream& stream = std::cin; // Use standard input stream by default

    // Read a line from the input stream
    if (!std::getline(stream, buf)) {
        return 0; // Return 0 if no more lines or an error occurs
    }

    // Parse the line using a string stream
    std::istringstream iss(buf);
    if (!(iss >> std::hex >> *pc >> *target >> std::dec >> *outcome >> *condition
                >> *call >> *ret >> *direct)) {
        return 0; // Return 0 if parsing fails
    }

    return 1; // Return 1 if parsing succeeds
}

int main(int argc, char *argv[])
{
  // Set defaults
  stream = stdin;
  bpType = STATIC;
  verbose = 0;

  // Process cmdline Arguments
  for (int i = 1; i < argc; ++i)
  {
    if (!strcmp(argv[i], "--help"))
    {
      usage();
      exit(0);
    }
    else if (!strncmp(argv[i], "--", 2))
    {
      if (!handle_option(argv[i]))
      {
        printf("Unrecognized option %s\n", argv[i]);
        usage();
        exit(1);
      }
    }
    else
    {
      // Use as input file
      stream = fopen(argv[i], "r");
    }
  }

  // Initialize the predictor
  init_predictor();

  uint32_t num_branches = 0;
  uint32_t mispredictions = 0;
  uint32_t pc = 0;
  uint32_t target = 0;
  uint32_t outcome = NOTTAKEN;
  uint32_t condition = 0;
  uint32_t call = 0;
  uint32_t ret = 0;
  uint32_t direct = 0;

  // Reach each branch from the trace
  while (read_branch(&pc, &target, &outcome, &condition, &call, &ret, &direct))
  {
    if (condition == 1)
    {
      num_branches++;
      // Make a prediction and compare with actual outcome
      uint32_t prediction = make_prediction(pc, target, direct);
      if (prediction != outcome)
      {
        mispredictions++;
      }
      if (verbose != 0)
      {
        printf("%d\n", prediction);
      }
    }
    // Train the predictor
    train_predictor(pc, target, outcome, condition, call, ret, direct);
  }

  // Print out the mispredict statistics
  printf("Branches:        %10d\n", num_branches);
  printf("Incorrect:       %10d\n", mispredictions);
  float mispredict_rate = 1000 * ((float)mispredictions / (float)num_branches);
  printf("Misprediction Rate: %7.3f\n", mispredict_rate);

  // Cleanup
  fclose(stream);
  free(buf);

  return 0;
}
