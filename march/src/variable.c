#include "/home/yashgajalwar/Desktop/erasure/march/include/variables.h"

struct Node **arr;
int count = 0;
struct Map myMap;

unsigned char gen[(NUM_DATA + NUM_PARITY) * (NUM_DATA + NUM_PARITY)];
unsigned char g_tbls[(NUM_DATA + NUM_PARITY) * (NUM_DATA + NUM_PARITY) * 32];
unsigned char *databuffs;
unsigned char *paritybuffs[NUM_PARITY];
unsigned char *datachunks[NUM_DATA];

// int main() {
//     // Your main code here
//     return 0;
// }
