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
 
typedef struct {
    int have_previous;
    int previous_phrase;
    int previous_is_index;
} DecodeState;
 
static void resetDictionary(void);
static unsigned int hashKey(int prefix, int prefix_is_index, unsigned char ch);
static int readCode(FILE *input, int *is_index);
static unsigned char firstCharacter(int code, int is_index);
static int findDictionary(int prefix, int prefix_is_index, unsigned char ch);
static int addDictionary(int prefix, int prefix_is_index, unsigned char ch);
static void outputLiteral(FILE *output, unsigned char ch);
static void outputPhrase(FILE *output, int phrase, int is_index);
static void processCharacter(FILE *output, unsigned char ch, DecodeState *state);
static void feedPhrase(FILE *output, int phrase, int is_index, DecodeState *state);
 
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
 
static int readCode(FILE *input, int *is_index) {
    int first;
    int second;
 
    first = fgetc(input);
    if (first == EOF) {
        return EOF;
    }
 
    if ((first & 0x80) == 0) {
        *is_index = 0;
        return first;
    }
 
    second = fgetc(input);
    if (second == EOF) {
        return EOF;
    }
 
    *is_index = 1;
    return ((first & 0x7f) << 8) | second;
}
 
static unsigned char firstCharacter(int code, int is_index) {
    if (!is_index) {
        return (unsigned char) code;
    }
 
    return dictionary[code].prefix_is_index
        ? firstCharacter(dictionary[code].prefix, 1)
        : (unsigned char) dictionary[code].prefix;
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
 
static void outputPhrase(FILE *output, int phrase, int is_index) {
    if (!is_index) {
        outputLiteral(output, (unsigned char) phrase);
        return;
    }
 
    if (dictionary[phrase].prefix_is_index) {
        outputPhrase(output, dictionary[phrase].prefix, 1);
    } else {
        outputLiteral(output, (unsigned char) dictionary[phrase].prefix);
    }
 
    outputLiteral(output, dictionary[phrase].ch);
}
 
static void processCharacter(FILE *output, unsigned char ch, DecodeState *state) {
    int candidate_index;
 
    if (next_index == MAX_DICT_SIZE) {
        if (state->have_previous) {
            outputPhrase(output, state->previous_phrase, state->previous_is_index);
        }
        resetDictionary();
        state->have_previous = 0;
    }
 
    if (!state->have_previous) {
        state->previous_phrase = (int) ch;
        state->previous_is_index = 0;
        state->have_previous = 1;
        return;
    }
 
    candidate_index = findDictionary(state->previous_phrase, state->previous_is_index, ch);
    if (candidate_index != -1) {
        state->previous_phrase = candidate_index;
        state->previous_is_index = 1;
        return;
    }
 
    outputPhrase(output, state->previous_phrase, state->previous_is_index);
 
    if (addDictionary(state->previous_phrase, state->previous_is_index, ch) == -1) {
        resetDictionary();
    }
 
    state->previous_phrase = (int) ch;
    state->previous_is_index = 0;
    state->have_previous = 1;
}
 
static void feedPhrase(FILE *output, int phrase, int is_index, DecodeState *state) {
    if (!is_index) {
        processCharacter(output, (unsigned char) phrase, state);
        return;
    }
 
    if (phrase == next_index) {
        feedPhrase(output, state->previous_phrase, state->previous_is_index, state);
        processCharacter(output, firstCharacter(state->previous_phrase, state->previous_is_index), state);
        return;
    }
 
    if (dictionary[phrase].prefix_is_index) {
        feedPhrase(output, dictionary[phrase].prefix, 1, state);
    } else {
        processCharacter(output, (unsigned char) dictionary[phrase].prefix, state);
    }
 
    processCharacter(output, dictionary[phrase].ch, state);
}
 
int ldecode(const char *input_file, const char *output_file) {
    FILE *input;
    FILE *output;
    int current_phrase;
    int current_is_index;
    DecodeState state;
 
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
    state.have_previous = 0;
 
    while ((current_phrase = readCode(input, &current_is_index)) != EOF) {
        feedPhrase(output, current_phrase, current_is_index, &state);
    }
 
    if (state.have_previous) {
        outputPhrase(output, state.previous_phrase, state.previous_is_index);
    }
 
    fclose(input);
    fclose(output);
 
    return 0;
}
 
int main(int argc, char *argv[]) {
    if (argc != 3) return 1;
    return ldecode(argv[1], argv[2]);
}

