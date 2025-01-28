#include <stdio.h>  /* standard input/output library */
#include <stdlib.h> /* Standard C Library */
#include <string.h> /* String operations library */
#include <ctype.h>  /* Library for useful character operations */
#include <limits.h> /* Library for definitions of common variable type characteristics */
#include <stdint.h>

#define MAX_LABEL_LEN 20
#define MAX_ARGS_LEN 21
#define MAX_SYMBOLS 255
#define MAX_OPCODE_LENGTH 8
#define NUM_OPCODES 29

#define MAX_LINE_LENGTH 255

enum
{
   DONE = 0xAAAAAAAA,
   OK,
   EMPTY_LINE
};

uint16_t aluOp(int opCodeIndex, char *args[], int currAddress);
uint16_t memoryReadAndWriteOp(int opCodeIndex, char *args[], int currAddress);
uint16_t shiftOp(int opCodeIndex, char *args[], int currentAddress);
uint16_t branchOp(int opCodeIndex, char *args[], int currAddress);
uint16_t trapOp(int opCodeIndex, char *args[], int currAddress);
uint16_t returnsOp(int opCodeIndex, char *args[], int currAddress);
uint16_t leaOp(int opCodeIndex, char *args[], int currAddress);
uint16_t jsrrAndjmpOp(int opCodeIndex, char *args[], int currAddress);
uint16_t jsrOp(int opCodeIndex, char *args[], int currAddress);
uint16_t fillOp(int opCodeIndex, char *args[], int currAddress);
uint16_t haltOp(int opCodeIndex, char *args[], int currAddress);
uint16_t nopOp(int opCodeIndex, char *args[], int currAddress);
int isOpCode(char *token);
int generateSymbolTable(FILE *pInfile, char *fileName);
int fileParser(FILE *pInfile, char *fileName, FILE *outFile, char *outFileName);
int toNum(char *str);
typedef struct
{
   int address;
   char label[MAX_LABEL_LEN + 1]; /* Question for the reader: Why do we need to add 1? */
} TableEntry;

typedef struct
{
   char opCodeName[MAX_OPCODE_LENGTH];
   uint16_t binCode;
   //char binCode[7 + 1];
   int numArgs;
   uint16_t (*operation)(int, char**, int);
} OpCode;

OpCode OpCodes[NUM_OPCODES] = {
    {"add", 0b0001, 3, aluOp}, // 1
    {"and", 0b0101, 3, aluOp}, // 5

    {"br",   0b0000111, 1, branchOp},
    {"brn",  0b0000100, 1, branchOp}, 
    {"brz",  0b0000010, 1, branchOp}, 
    {"brp",  0b0000001, 1, branchOp}, 
    {"brzp", 0b0000011, 1, branchOp}, 
    {"brnp", 0b0000101, 1, branchOp}, 
    {"brnz", 0b0000110, 1, branchOp}, 
    {"brnzp",0b0000111, 1, branchOp}, 


   {"jmp",   0b1100, 1, jsrrAndjmpOp},
    {"jsr",   0b01001, 1, jsrOp},
    {"jsrr",  0b0100, 1, jsrrAndjmpOp},

    {"ldb",   0b0010, 3, memoryReadAndWriteOp},
    {"ldw",   0b0110, 3, memoryReadAndWriteOp},
    {"lea",   0b1110, 2, leaOp},

    {"not",   0b1001, 2, aluOp},

    {"ret",   0b1100, 0, returnsOp},
    {"rti",   0b1000, 0, returnsOp},

    {"lshf",  0b1101, 3, shiftOp},
    {"rshfl", 0b1101, 3, shiftOp},
    {"rshfa", 0b1101, 3, shiftOp},

    {"stb",   0b0011, 3, memoryReadAndWriteOp},
    {"stw",   0b0111, 3, memoryReadAndWriteOp},
    {"trap",  0b1111, 1, trapOp},
    {"halt", 0b1111, 1, haltOp},
    {"xor",   0b1001, 3, aluOp},
    {"nop", 0b0, 0, nopOp},
    {".fill", 0b0, 1, fillOp}

};

TableEntry symbolTable[MAX_SYMBOLS];
int symbolIndex;



int main(int argc, char *argv[])
{
   char* prgName = NULL;
   char *iFileName = NULL;
   char *oFileName = NULL;
   /*argv[1] = "opcodeTestsubset1.asm";
   argv[2] = "output.obj";*/
   
   prgName = argv[0];
   iFileName =  argv[1];
   oFileName = argv[2];

   printf("program name = '%s'\n", prgName);
   printf("input file name = '%s'\n", iFileName);
   printf("output file name = '%s'\n", oFileName);

   FILE *infile = NULL;
   FILE *outfile = NULL;

   /* open the source file */

   infile = fopen(argv[1], "r");
   outfile = fopen(argv[2], "w");

   if (!infile)
   {
      printf("Error: Cannot open file %s\n", argv[1]);
      exit(4);
   }

   if (!outfile)
   {
      printf("Error: Cannot open file %s\n", argv[2]);
      exit(4);
   }

   int ret = generateSymbolTable(infile, iFileName);
   ret = fileParser(infile, iFileName, outfile, oFileName);

   fclose(infile);
   fclose(outfile);
}

int isOpCode(char *token)
{
   for (int i = 0; i < NUM_OPCODES; i++)
   {
      if (strcmp(token, OpCodes[i].opCodeName) == 0)
      {
         return i; // true
      }
   }
   return -1; // false
}

int parseSymbolOnLine(FILE *pInfile, char *pLine, char **pLabel, int *LocationCounter, int *symbolIndex)
{

   char *lPtr;
   int i;
   if (!fgets(pLine, MAX_LINE_LENGTH, pInfile))
   {
      return (DONE);
   }

   /* convert entire line to lowercase */
   for (i = 0; i < strlen(pLine); i++)
   {
      pLine[i] = tolower(pLine[i]);
   }

   *pLabel = pLine + strlen(pLine);

   lPtr = pLine;

   while (*lPtr != ';' && *lPtr != '\0' && *lPtr != '\n')
   {
      lPtr++;
   }

   *lPtr = '\0';
   if (!(lPtr = strtok(pLine, "\t\n ,")))
   {
      return (EMPTY_LINE);
   }

   if (isOpCode(lPtr) == -1) /*Not an OpCode - Therefore either Label or psudo Op*/
   {
      if (lPtr[0] == '.')
      { // pseudo-op
         if (strcmp(lPtr, ".orig") == 0)
         { // If it's ".ORIG"
            lPtr = strtok(NULL, "\t\n ,");
            if (lPtr)
            {
               *LocationCounter = toNum(lPtr); // Update LocationCounter with the address
            }
            return OK;
         }
         if (strcmp(lPtr, ".end") == 0)
         {
            return (DONE);
         }
      }

      // Line contains a label
      else
      {

         symbolTable[*symbolIndex].address = *LocationCounter;
         strncpy(symbolTable[*symbolIndex].label, lPtr, MAX_LABEL_LEN);
         *symbolIndex += 1;
         if (!(lPtr = strtok(NULL, "\t\n ,")))
         {             // its possible that the Label does not have an instruction on the same line,
            return OK; // therefore we should not increment the LC bc the next instruction will be corresponding the adddress of the label
         }
      }
   }
   *LocationCounter += 2;
   return (OK);
}

int generateSymbolTable(FILE *pInfile, char *fileName){

   char lLine[MAX_LINE_LENGTH + 1], *lLabel;

   int lRet;
   int LC = 0;
   symbolIndex = 0;

   pInfile = fopen(fileName, "r"); /* open the input file */

   do
   {
      lRet = parseSymbolOnLine(pInfile, lLine, &lLabel, &LC, &symbolIndex);

   } while (lRet != DONE);

   return 0;
}

int readAndParse(FILE *pInfile, char *pLine, char **pLabel, char **pOpcode, char *args[4])
{
   static uint16_t locationCounter = 0;
   char *lRet, *lPtr;
   int i;

   if (!fgets(pLine, MAX_LINE_LENGTH, pInfile))
   {
      return (DONE);
   }

   /* convert entire line to lowercase */
   for (i = 0; i < strlen(pLine); i++)
   {
      pLine[i] = tolower(pLine[i]);
   }

   *pLabel = *pOpcode = args[0] = args[1] = args[2] = args[3] = pLine + strlen(pLine);

   lPtr = pLine;

   while (*lPtr != ';' && *lPtr != '\0' && *lPtr != '\n')
   {
      lPtr++;
   }

   *lPtr = '\0';

   if (!(lPtr = strtok(pLine, "\t\n ,")))
   {
      return (EMPTY_LINE);
   }

   // found psuedoOp. In this code we treat the .fill psuedo Op as an opCode
   if (strcmp(lPtr, ".orig") == 0)
   { //.orig
      lPtr = strtok(NULL, "\t\n ,");
      if (lPtr)
      {
         locationCounter = toNum(lPtr); // Update LocationCounter with the address
         //printf("0x%04X\n", locationCounter);
         return locationCounter;
      }
      return OK;
   }
   /*  if (strcmp(lPtr, ".fill") == 0)
   { //.fill
      lPtr = strtok(NULL, "\t\n ,");
      // write to file the number that this gets
      printf(lPtr);
      locationCounter += 2;
      return OK;
   } */
   if(strcmp(lPtr, ".end") == 0)
   { //.end
      return DONE;
   }
   

   if (isOpCode(lPtr) == -1 && lPtr[0] != '.') /* found a label */
   {
      *pLabel = lPtr;
      if (!(lPtr = strtok(NULL, "\t\n ,")))
         return (OK);
   }

   *pOpcode = lPtr;
   int opCodeIndex = isOpCode(lPtr);
   for (int i = 0; i < OpCodes[opCodeIndex].numArgs; i++)
   {
      lPtr = strtok(NULL, "\t\n ,");
      if (lPtr){
         args[i] = lPtr;
      }

      else{
         break;
      }
   }

   uint16_t binaryInstruction = OpCodes[opCodeIndex].operation(opCodeIndex, args, locationCounter);
   locationCounter += 2;
   return binaryInstruction;
}


int fileParser(FILE *pInfile, char *fileName, FILE *outFile, char* outFileName)
{
   char lLine[MAX_LINE_LENGTH + 1], *lLabel, *lOpcode, *lArg1,
       *lArg2, *lArg3, *lArg4;

   char* args[4];
   int lRet;
   FILE *lInfile;

   lInfile = fopen(fileName, "r"); /* open the input file */

   do
   {
      lRet = readAndParse(lInfile, lLine, &lLabel, &lOpcode, args);
      if(lRet != DONE && lRet != EMPTY_LINE){
         fprintf(outFile, "0x%04X\n", lRet);
      }
   } while (lRet != DONE);
   return 0;
}


int toNum(char *pStr)
{
   char *t_ptr;
   char *orig_pStr;
   int t_length, k;
   int lNum, lNeg = 0;
   long int lNumLong;

   orig_pStr = pStr;
   if (*pStr == '#') /* decimal */
   {
      pStr++;
      if (*pStr == '-') /* dec is negative */
      {
         lNeg = 1;
         pStr++;
      }
      t_ptr = pStr;
      t_length = strlen(t_ptr);
      for (k = 0; k < t_length; k++)
      {
         if (!isdigit(*t_ptr))
         {
            printf("Error: invalid decimal operand, %s\n", orig_pStr);
            exit(4);
         }
         t_ptr++;
      }
      lNum = atoi(pStr);
      if (lNeg)
         lNum = -lNum;

      return lNum;
   }
   else if (*pStr == 'x') /* hex     */
   {
      pStr++;
      if (*pStr == '-') /* hex is negative */
      {
         lNeg = 1;
         pStr++;
      }
      t_ptr = pStr;
      t_length = strlen(t_ptr);
      for (k = 0; k < t_length; k++)
      {
         if (!isxdigit(*t_ptr))
         {
            printf("Error: invalid hex operand, %s\n", orig_pStr);
            exit(4);
         }
         t_ptr++;
      }
      lNumLong = strtol(pStr, NULL, 16); /* convert hex string into integer */
      lNum = (lNumLong > INT_MAX) ? INT_MAX : lNumLong;
      if (lNeg)
         lNum = -lNum;
      return lNum;
   }
   else
   {
      printf("Error: invalid operand, %s\n", orig_pStr);
      exit(4); /* This has been changed from error code 3 to error code 4, see clarification 12 */
   }
}


int isRegister(char* arg){
   if(strlen(arg) == 2){
      if(arg[0] == 'r' && isdigit(arg[1])){
         int regNum = arg[1]-'0';
         if(regNum >= 0 && regNum <= 7){
            return 1;
         }
      }
   }
   return 0;
}
int getSymbolAddress(char* label){
   for(int i = 0; i < symbolIndex; i++){
      if(strcmp(label, symbolTable[i].label) ==0){
         return symbolTable[i].address;
      }
   }
   return -1;
}

int getOffset(char* label, int currAddress){
    int labelAddress = getSymbolAddress(label);
   int offset = 0;
   if(labelAddress == -1){
      offset = toNum(label);
   }
   else{   
      offset = (labelAddress - (currAddress +2))/2;
   }
   return offset;
}



uint16_t aluOp(int opCodeIndex, char *args[], int currAddress){
   
   uint16_t binaryInstruction = 0;

   binaryInstruction += OpCodes[opCodeIndex].binCode<<12;
   int dr = args[0][1] - '0'; //arg will contain a register i.e r1. Therefore args[0][1] corresponds to the number of reg1
   int sr1 = args[1][1] - '0';
   binaryInstruction += dr<<9;
   binaryInstruction += sr1<<6;

   if(isRegister(args[2])){ //alu operation uses 2 registers and a destination register
      int sr2 = args[2][1] - '0';
      binaryInstruction += (sr2 & 0x1F);
   }
   else{ //imm5
      if(strcmp(OpCodes[opCodeIndex].opCodeName, "not") == 0){ //if the opcode is not change the immediate 5 to 11111
         args[2] = "x1F";
      }
      binaryInstruction += 1<<5;
      binaryInstruction += (toNum(args[2]) & 0x1F);
   }
   //convert number to hex and then convert the hex to character format and then print to file
   //printf("0x%04X\n", binaryInstruction);
   return binaryInstruction;
   //fflush(stdout);
}

uint16_t memoryReadAndWriteOp(int opCodeIndex, char* args[], int currAddress){
   uint16_t binaryInstruction = 0;
   binaryInstruction += OpCodes[opCodeIndex].binCode<<12;
   int reg1 = args[0][1] - '0';
   int reg2 = args[1][1] - '0';
   binaryInstruction += reg1<<9;
   binaryInstruction += reg2<<6;

   int offset = getOffset(args[2], currAddress);
   binaryInstruction += (offset & 0b0111111);
   //printf("0x%04X\n", binaryInstruction);
   //fflush(stdout);
   return binaryInstruction;
   //convert num etc 
}

uint16_t shiftOp(int opCodeIndex, char* args[], int currentAddress){
   uint16_t binaryInstruction = 0;
   binaryInstruction += OpCodes[opCodeIndex].binCode<<12;
   int reg1 = args[0][1] - '0';
   int reg2 = args[1][1] - '0';
   binaryInstruction += reg1<<9;
   binaryInstruction += reg2<<6;
   char* shiftType = OpCodes[opCodeIndex].opCodeName;//args[0];
   if(strcmp(shiftType, "rshfl") == 0){
      binaryInstruction += (1<<4);
   }
   if(strcmp(shiftType, "rshfa") ==0){
      binaryInstruction += (3<<4);
   }
   binaryInstruction += (toNum(args[2]) & 0xF); //preventing sign extension. Keep shift amount to 4 bits
   //printf("0x%04X\n", binaryInstruction);
   //fflush(stdout);
   return binaryInstruction;

   //convert to hex etc
}

uint16_t branchOp(int opCodeIndex, char* args[], int currAddress){
   uint16_t binaryInstruction = 0;
   binaryInstruction += OpCodes[opCodeIndex].binCode<<9;
   int offset = getOffset(args[0], currAddress);
   binaryInstruction += (0x01FF & offset);//stop sign extension. Keep offset to 9 bits
//convert to hex
   //printf("0x%04X\n", binaryInstruction);
   //fflush(stdout);

   return binaryInstruction;

}

uint16_t trapOp(int opCodeIndex, char* args[], int currAddress){
   uint16_t binaryInstruction = 0;
   binaryInstruction += OpCodes[opCodeIndex].binCode<<12;
   binaryInstruction += (toNum(args[0]) & 0xFF);
   //printf("0x%04X\n", binaryInstruction);
   //fflush(stdout);

   return binaryInstruction;

}

uint16_t returnsOp(int opCodeIndex, char* args[], int currAddress){
   uint16_t binaryInstruction = 0;
   if(OpCodes[opCodeIndex].binCode == 0b1100){ //RET;
      binaryInstruction = 0b1100000111000000;
   }
   else{ //RTI
      binaryInstruction = 0b1000000000000000;
   }
   /*printf("0x%04X\n", binaryInstruction);
   fflush(stdout); */
   return binaryInstruction;
}

uint16_t leaOp(int opCodeIndex, char* args[], int currAddress){
   uint16_t binaryInstruction = 0;
   binaryInstruction += OpCodes[opCodeIndex].binCode<<12;
   int dr = args[0][1] - '0';
   binaryInstruction += dr<<9;
   int offset = getOffset(args[1], currAddress);
   binaryInstruction += (offset & 0x3F); //using only the botton  6 bits. Dont want sign extension
   /*printf("0x%04X\n", binaryInstruction);
   fflush(stdout);*/

   return binaryInstruction;
}

uint16_t jsrrAndjmpOp(int opCodeIndex, char* args[], int currAddress){
    uint16_t binaryInstruction = 0;
   binaryInstruction += OpCodes[opCodeIndex].binCode<<12;
   int baseR = args[0][1] - '0';
   binaryInstruction += baseR <<6;
   /*printf("0x%04X\n", binaryInstruction);
   fflush(stdout);*/

   return binaryInstruction;
}

uint16_t jsrOp(int opCodeIndex, char* args[], int currAddress){
     uint16_t binaryInstruction = 0;
   binaryInstruction += OpCodes[opCodeIndex].binCode<<11;
   int offset = getOffset(args[0], currAddress);
   binaryInstruction += (offset & 0x7FF);
   /*printf("0x%04X\n", binaryInstruction);
   fflush(stdout);*/

   return binaryInstruction;
}

uint16_t fillOp(int opCodeIndex, char* args[], int currAddress){
   uint16_t fillVal = (uint16_t)toNum(args[0]);
   /*printf("0x%04X\n", fillVal);
   fflush(stdout);*/
   return fillVal;
}

uint16_t haltOp(int opCodeIndex, char* args[], int currAddress){
   /*printf("0xF025\n");
   fflush(stdout); */

   return 0xF025;
}

uint16_t nopOp(int opCodeIndex, char* args[], int currAddress){
   /*printf("0x0000\n");
   fflush(stdout);*/
   return 0;
}