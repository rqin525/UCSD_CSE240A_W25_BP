//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
#include <math.h>
#include "predictor.h"

//
// TODO:Student Information
//
const char *studentName = "Richard Qin";
const char *studentID = "A69031813";
const char *email = "riqin@ucsd.edu";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = {"Static", "Gshare",
                         "Tournament", "Custom"};

// define number of bits required for indexing the BHT here.
int ghistoryBits = 17; // Number of bits used for Global History
int bpType;            // Branch Prediction Type
int verbose;

int lhistoryBits = 10; // number of bits for local history
int pcIndexBits = 10; // bits to index LHT
int muxBits = 12; // bits for mux

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

//
// TODO: Add your own Branch Predictor data structures here
//
// gshare
uint8_t *bht_gshare; // global predictor
uint64_t ghistory; // GHR: tracks outcomes of last N branches
// Tournament
uint8_t *bht_tourney; // global predictor
// use same GHR
uint8_t *lht; // LHT: store recent outcomes specific to branch (pc)
uint8_t *local_pht;
uint8_t *mux; // choose between global and local predictor;
              // 0=strong local, 1=weak local, 2=weak global, 3=strong global


//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Initialize the predictor
//

// gshare functions
void init_gshare()
{
  int bht_entries = 1 << ghistoryBits;
  bht_gshare = (uint8_t *)malloc(bht_entries * sizeof(uint8_t));
  int i = 0;
  for (i = 0; i < bht_entries; i++)
  {
    bht_gshare[i] = WN;
  }
  ghistory = 0;
}

uint8_t gshare_predict(uint32_t pc)
{
  // get lower ghistoryBits of pc
  uint32_t bht_entries = 1 << ghistoryBits;
  uint32_t pc_lower_bits = pc & (bht_entries - 1);
  uint32_t ghistory_lower_bits = ghistory & (bht_entries - 1);
  uint32_t index = pc_lower_bits ^ ghistory_lower_bits;
  switch (bht_gshare[index])
  {
  case WN:
    return NOTTAKEN;
  case SN:
    return NOTTAKEN;
  case WT:
    return TAKEN;
  case ST:
    return TAKEN;
  default:
    printf("Warning: Undefined state of entry in GSHARE BHT!\n");
    return NOTTAKEN;
  }
}

void train_gshare(uint32_t pc, uint8_t outcome)
{
  // get lower ghistoryBits of pc
  uint32_t bht_entries = 1 << ghistoryBits;
  uint32_t pc_lower_bits = pc & (bht_entries - 1);
  uint32_t ghistory_lower_bits = ghistory & (bht_entries - 1);
  uint32_t index = pc_lower_bits ^ ghistory_lower_bits;

  // Update state of entry in bht based on outcome
  switch (bht_gshare[index])
  {
  case WN:
    bht_gshare[index] = (outcome == TAKEN) ? WT : SN;
    break;
  case SN:
    bht_gshare[index] = (outcome == TAKEN) ? WN : SN;
    break;
  case WT:
    bht_gshare[index] = (outcome == TAKEN) ? ST : WN;
    break;
  case ST:
    bht_gshare[index] = (outcome == TAKEN) ? ST : WT;
    break;
  default:
    printf("Warning: Undefined state of entry in GSHARE BHT!\n");
    break;
  }

  // Update history register
  ghistory = ((ghistory << 1) | outcome);
}

// Tournament functions
void init_tournament()
{
  int bht_entries = 1 << ghistoryBits;
  int lht_entries = 1 << pcIndexBits;
  int mux_entries = 1 << muxBits;

  // table setup
  bht_tourney = (uint8_t *)malloc(bht_entries * sizeof(uint8_t));
  local_pht = (uint8_t *)malloc(bht_entries * sizeof(uint8_t));
  lht = (uint8_t *)malloc(lht_entries * sizeof(uint8_t));
  mux = (uint8_t *)malloc(mux_entries * sizeof(uint8_t));

  // init predictors (weakly not taken)
  for (int i = 0; i < bht_entries; i++) {
    bht_tourney[i] = WN;
    local_pht[i] = WN;
  }
  for (int i = 0; i < lht_entries; i++) {
    lht[i] = 0;
  }

  // init mux (weak local predictor)
  for (int i = 0; i < mux_entries; i++) {
    mux[i] = 1;
  }
  ghistory = 0;
}

uint8_t tournament_predict(uint32_t pc)
{
  uint32_t bht_entries = 1 << ghistoryBits;
  uint32_t lht_entries = 1 << pcIndexBits;
  uint32_t mux_entries = 1 << muxBits;

  uint32_t global_index = (pc ^ ghistory) & (bht_entries - 1);
  uint32_t local_index = lht[pc & (lht_entries - 1)] & (bht_entries - 1);
  uint32_t mux_index = pc & (mux_entries - 1);

  // predict!
  uint8_t global_pred = (bht_tourney[global_index] >= WT) ? TAKEN : NOTTAKEN;
  uint8_t local_pred = (local_pht[local_index] >= WT) ? TAKEN : NOTTAKEN;
  uint8_t choice = mux[mux_index];

  // choose between predictors
  return (choice >= 2) ? global_pred : local_pred;
}

void train_tournament(uint32_t pc, uint8_t outcome)
{
  uint32_t bht_entries = 1 << ghistoryBits;
  uint32_t lht_entries = 1 << pcIndexBits;
  uint32_t mux_entries = 1 << muxBits;

  uint32_t global_index = (pc ^ ghistory) & (bht_entries - 1);
  uint32_t local_index = lht[pc & (lht_entries - 1)] & (bht_entries - 1);
  uint32_t mux_index = pc & (mux_entries - 1);

  // predict!
  uint8_t global_pred = (bht_tourney[global_index] >= WT) ? TAKEN : NOTTAKEN;
  uint8_t local_pred = (local_pht[local_index] >= WT) ? TAKEN : NOTTAKEN;

  // global update state
  if (outcome == TAKEN) {
    if (bht_tourney[global_index] < ST) bht_tourney[global_index]++;
  } else {
    if (bht_tourney[global_index] > SN) bht_tourney[global_index]--;
  }

  // local update state
  if (outcome == TAKEN) {
    if (local_pht[local_index] < ST) local_pht[local_index]++;
  } else {
    if (local_pht[local_index] > SN) local_pht[local_index]--;
  }

  // mux update state
  if (global_pred == outcome && local_pred != outcome) {
    if (mux[mux_index] < 3) mux[mux_index]++;
  } else if (global_pred != outcome && local_pred == outcome) {
    if (mux[mux_index] > 0) mux[mux_index]--;
  }

  // update LHT
  // (old bits) | outcome
  // bit mask ((1 << lhistoryBits) - 1) to make sure does not exceed bit width
  lht[pc & (lht_entries - 1)] = ((lht[pc & (lht_entries - 1)] << 1) | outcome);

  // update global history register
  ghistory = ((ghistory << 1) | outcome);
}

void cleanup_gshare()
{
  free(bht_gshare);
}

void cleanup_tournament()
{
  free(bht_tourney);
  free(local_pht);
  free(lht);
  free(mux);
}

void init_predictor()
{
  switch (bpType)
  {
  case STATIC:
    break;
  case GSHARE:
    init_gshare();
    break;
  case TOURNAMENT:
    init_tournament();
    break;
  case CUSTOM:
    break;
  default:
    break;
  }
}

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint32_t make_prediction(uint32_t pc, uint32_t target, uint32_t direct)
{

  // Make a prediction based on the bpType
  switch (bpType)
  {
  case STATIC:
    return TAKEN;
  case GSHARE:
    return gshare_predict(pc);
  case TOURNAMENT:
    return tournament_predict(pc);
  case CUSTOM:
    return NOTTAKEN;
  default:
    break;
  }

  // If there is not a compatable bpType then return NOTTAKEN
  return NOTTAKEN;
}

// Train the predictor the last executed branch at PC 'pc' and with
// outcome 'outcome' (true indicates that the branch was taken, false
// indicates that the branch was not taken)
//

void train_predictor(uint32_t pc, uint32_t target, uint32_t outcome, uint32_t condition, uint32_t call, uint32_t ret, uint32_t direct)
{
  if (condition)
  {
    switch (bpType)
    {
    case STATIC:
      return;
    case GSHARE:
      return train_gshare(pc, outcome);
    case TOURNAMENT:
      return train_tournament(pc, outcome);
    case CUSTOM:
      return;
    default:
      break;
    }
  }
}
