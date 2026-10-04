#include <stdio.h>
#include <stdlib.h>

#define MAX_DICT_SIZE 32768

typedef struct {
    int prefix;
    int prefix_is_index;
    unsigned char ch;
    int length;
} Entry;

Entry dictionary[MAX_DICT_SIZE];
int next_index = 0;

#define HASH_TABLE_SIZE 65536

typedef struct Node {
    int index;
    struct Node *next;
} Node;

static Node hash_nodes[MAX_DICT_SIZE];
static Node *hash_table[HASH_TABLE_SIZE];
static int next_node = 0;

static void resetDictionary(void);
static unsigned int hashKey(int prefix, int prefix_is_index, unsigned char ch);
static int findDictionary(int prefix, int prefix_is_index, unsigned char ch);
static int addDictionary(int prefix, int prefix_is_index, unsigned char ch);
static void outputLiteral(FILE *output, unsigned char ch);
static void outputIndex(FILE *output, int index);
static void outputPhrase(FILE *output, int phrase, int is_index);

static void resetDictionary(void) {
    int i;

    next_index = 0;
    next_node = 0;

    for (i = 0; i < HASH_TABLE_SIZE; i++) {
        hash_table[i] = NULL;
    }
}

static unsigned int hashKey(int prefix, int prefix_is_index, unsigned char ch) {
    unsigned int value = (unsigned int) prefix;

    value = value * 131u + (unsigned int) prefix_is_index;
    value = value * 257u + (unsigned int) ch;
    return value % HASH_TABLE_SIZE;
}

static int findDictionary(int prefix, int prefix_is_index, unsigned char ch) {
    unsigned int bucket = hashKey(prefix, prefix_is_index, ch);
    Node *current = hash_table[bucket];

    while (current != NULL) {
        if (dictionary[current->index].prefix == prefix &&
            dictionary[current->index].prefix_is_index == prefix_is_index &&
            dictionary[current->index].ch == ch) {
            return current->index;
        }
        current = current->next;
    }

    return -1;
}

static int addDictionary(int prefix, int prefix_is_index, unsigned char ch) {
    unsigned int bucket;

    if (next_index >= MAX_DICT_SIZE) {
        return -1;
    }

    dictionary[next_index].prefix = prefix;
    dictionary[next_index].prefix_is_index = prefix_is_index;
    dictionary[next_index].ch = ch;
    dictionary[next_index].length = prefix_is_index ? dictionary[prefix].length + 1 : 2;

    bucket = hashKey(prefix, prefix_is_index, ch);
    hash_nodes[next_node].index = next_index;
    hash_nodes[next_node].next = hash_table[bucket];
    hash_table[bucket] = &hash_nodes[next_node];
    next_node++;

    return next_index++;
}

static void outputLiteral(FILE *output, unsigned char ch) {
    fputc((int) ch, output);
}

static void outputIndex(FILE *output, int index) {
    unsigned char first = (unsigned char) (0x80 | ((index >> 8) & 0x7f));
    unsigned char second = (unsigned char) (index & 0xff);

    fputc(first, output);
    fputc(second, output);
}

static void outputPhrase(FILE *output, int phrase, int is_index) {
    if (!is_index) {
        outputLiteral(output, (unsigned char) phrase);
        return;
    }

    if (dictionary[phrase].length >= 3) {
        outputIndex(output, phrase);
        return;
    }

    outputPhrase(output, dictionary[phrase].prefix, dictionary[phrase].prefix_is_index);
    outputLiteral(output, dictionary[phrase].ch);
}

int lencode(const char *input_file, const char *output_file) {
    FILE *input;
    FILE *output;
    int previous_phrase;
    int previous_is_index;
    int ch;

    if (input_file == NULL || output_file == NULL) {
        return EXIT_FAILURE;
    }

    input = fopen(input_file, "rb");
    if (input == NULL) {
        return EXIT_FAILURE;
    }

    output = fopen(output_file, "wb");
    if (output == NULL) {
        fclose(input);
        return EXIT_FAILURE;
    }

    resetDictionary();

    ch = fgetc(input);

    if (ch == EOF) {
        fclose(input);
        fclose(output);
        return 0;
    }

    previous_phrase = (unsigned char) ch;
    previous_is_index = 0;

    while ((ch = fgetc(input)) != EOF) {
        unsigned char c = (unsigned char) ch;
        int index;

        if (next_index == MAX_DICT_SIZE) {
            outputPhrase(output, previous_phrase, previous_is_index);
            resetDictionary();
            previous_phrase = (int) c;
            previous_is_index = 0;
            continue;
        }

        index = findDictionary(previous_phrase, previous_is_index, c);

        if (index != -1) {
            previous_phrase = index;
            previous_is_index = 1;
            continue;
        }

        outputPhrase(output, previous_phrase, previous_is_index);

        if (addDictionary(previous_phrase, previous_is_index, c) == -1) {
            resetDictionary();
        }

        previous_phrase = c;
        previous_is_index = 0;
    }

    outputPhrase(output, previous_phrase, previous_is_index);

    fclose(input);
    fclose(output);

    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 3) return 1;
    return lencode(argv[1], argv[2]);
}
