
#include <inttypes.h>
#include "base64.h"

#ifdef BASE64_TEST
#include <stdio.h>
#endif


// char_val_table contains the values that the encoded chars represent
// so 'A' == 0 since its the base64 table index val B == 1, C == 2 etc
// -1 means that there is no base64 value for that char
static int char_val_table[128] = {
//   0   1   2   3   4   5   6   7   8   9
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 0
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 10
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 20
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 30
    -1, -1, -1, -1, 62, -1, -1, 63, 52, 53, // 40
    54, 55, 56, 57, 58, 59, 60, 61, -1, -1, // 50
    -1, 64, -1, -1, -1,  0,  1,  2,  3,  4, // 60
     5,  6,  7,  8,  9, 10, 11, 12, 13, 14, // 70
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, // 80
    25, -1, -1, -1, -1, -1, -1, 26, 27, 28, // 90
    29, 30, 31, 32, 33, 34, 35, 36, 37, 38, // 100
    39, 40, 41, 42, 43, 44, 45, 46, 47, 48, // 110
    49, 50, 51, -1, -1, -1, -1, -1          // 120
};

// char_table contains the Base64 index table
static char char_table[] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
    'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
    'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
    'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
    'w', 'x', 'y', 'z', '0', '1', '2', '3',
    '4', '5', '6', '7', '8', '9', '+', '/'
};


/**
 * @brief encode string with base64 endcoding
 *
 * @param input the message we want to encode
 * @param output outputbuffer. the encoded message is put here, terminated with null
 * @param len lenght of the input
 * @return int 1 if success
 */
int base64encode(const uint8_t *input, char *output, int len) {

    int i,j;
    int letter_bits, table_index, tmpint;

    // offset is used for keeping trank the len dif between input and output
    int offset = -1;

    // padding_amount sets the amount of '=' in the end of the output
    int padding_amount = len % 3;

    // Go through 3 letters per loop
    for (i = 0; i < len; i += 3) {
        letter_bits = 0;
        offset++;
        // Combine the bits from three letters to sinle int
        for (j = 0; j < 3 && (j + i) < len; j++) {
            letter_bits += input[i + j] << 8 * (2 - j);
        }

        // Translate the first 6 bits to Base64 letter
        tmpint = letter_bits;
        table_index = tmpint >> 18;
        output[0 + i + offset] = char_table[table_index];

        // Translate the second 6 bits to Base64 letter
        tmpint = letter_bits;
        tmpint = tmpint & 0x0003f000;
        table_index = tmpint >> 12;
        output[1 + i + offset] = char_table[table_index];

        // Translate the third 6 bits to Base64 letter
        // This letter is overridden with '=' if padding_amount == 2
        tmpint = letter_bits;
        tmpint = tmpint & 0x00000fc0;
        table_index = tmpint >> 6;
        output[2 + i + offset] = char_table[table_index];

        // Translate the fourth 6 bits to Base64 letter
        // This letter is overridden with '=' if padding_amount > 0
        tmpint = letter_bits;
        table_index = tmpint & 0x000003f;
        output[3 + i + offset] = char_table[table_index];

    }

    // Finally set the padding and terminate the string with null
    if (padding_amount == 2) {
        output[-1 + i + offset] = '=';
        output[i + offset] = '=';
    } else if (padding_amount ==  1) {
        output[i + offset] = '=';
    }
    output[1 + i + offset] = '\0';

    return 1;
}


/**
 * @brief decode base64 encoded string
 *
 * @param input the message we want to decode
 * @param output outputbuffer. the decoded message is put here, terminated with null
 * @param len lenght of the input
 * @return int 1 if success, 0 if failed (fail is from non base64 char)
 */
int base64decode(const char *input, uint8_t *output, int len) {
    int i,j;
    int letter_bits, char_val, tmpint;

    // offset is used for keeping trank the len dif between input and output
    int offset = 1;

    // Check the padding in the end of the string
    int padding_amount = 0;
    if (input[len - 1] == '=') padding_amount++;
    if (input[len - 2] == '=') padding_amount++;

    // Go through 4 letters per loop
    for (i = 0; i < len; i += 4) {
        letter_bits = 0;
        offset--;
        // Combine the bits from four letter table values to single int
        for (j = 0; j < 4 && (j + i) < len; j++) {
            tmpint = char_val_table[(int)input[i + j]];
            if (tmpint == -1) return 0;
            letter_bits += tmpint << 6 * (3 - j);
        }

        // Get the char from the first 8 bits
        tmpint = letter_bits;
        char_val = tmpint >> 16;
        output[0 + i + offset] = (char)char_val;

        // Get the char from the second 8 bits
        tmpint = letter_bits;
        tmpint = tmpint & 0x0000ff00;
        char_val = tmpint >> 8;
        output[1 + i + offset] = (char)char_val;

        // Get the char from the last 8 bits
        tmpint = letter_bits;
        tmpint = tmpint & 0x000000ff;
        output[2 + i + offset] = (char)tmpint;

    }

    // Finally set the null. This ignores the padding
    if (padding_amount == 2) {
        output[-3 + i + offset] = '\0';
    } else if (padding_amount ==  1) {
        output[-2 + i + offset] = '\0';
    } else {
        output[-1 + i + offset] = '\0';
    }
    return 1;
}

#ifdef BASE64_TEST
int main(){
    uint8_t lsd[] = {'M', 'a', 'n', 'M', 'a', 'n'};
    char buf[9];
    base64encode(lsd, buf, 6);
    printf("encoded: %s\n", buf);

    char buf2[9];
    uint8_t lsd2[] = {'M', 'a', 'n', 'M', 'a', 'n'};
    base64encode(lsd2, buf2, 5);
    printf("encoded: %s\n", buf2);

    char buf3[9];
    uint8_t lsd3[] = {'M', 'a', 'n', 'M', 'a', 'n'};
    base64encode(lsd3, buf3, 4);
    printf("encoded: %s\n", buf3);

    char buf4[30];
    base64encode((uint8_t*)"any carnal pleasure.", buf4, 20);
    printf("encoded: %s\n", buf4);

    uint8_t buf5[30];
    base64decode("TWFuTWFu", buf5, 8);
    printf("decoded: %s\n", buf5);

    uint8_t buf6[30];
    base64decode("YW55IGNhcm5hbCBwbGVhc3Vy", buf6, 24);
    printf("decoded: %s\n", buf6);

    uint8_t buf7[30];
    base64decode("YW55IGNhcm5hbCBwbGVhc3VyZS4=", buf7, 28);
    printf("decoded: %s\n", buf7);

    uint8_t buf8[30];
    if (base64decode("ääYW55IGNhcm5hbCBwbGVhc3VyZQ==", buf8, 28))
        printf("decoded: %s\n", buf8);

}
#endif
