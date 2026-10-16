# LZW File Compression Utility

A lossless text compressor and decompressor written from scratch in **C**, using an LZW-style dictionary algorithm and a compact binary output format. It ships as two small command-line programs, `lencode` and `ldecode`, with no external dependencies.

![Language: C](https://img.shields.io/badge/language-C-blue)
![Type: CLI utility](https://img.shields.io/badge/type-CLI%20utility-lightgrey)

## Highlights

- **Encoder and decoder written from scratch** in C using only the standard library.
- **32,768-entry dictionary** with **reset-on-full**, so long inputs keep compressing instead of stalling once the table fills.
- **Compact variable-width output (8/16-bit):** single bytes for literals and 2-byte references (15-bit dictionary index) for repeated phrases, told apart by a flag bit.
- **Hash tables with chaining** (65,536 buckets) in both encoder and decoder, giving fast average-case dictionary lookups with no dynamic allocation.
- **Decoder replays the encoder's dictionary construction** on the text it recovers, so both sides stay in sync without storing the dictionary in the file. This includes the tricky case where a code refers to the entry currently being built.
- **Automated round-trip test script** that checks every result byte-for-byte with `cmp`.

## How it works

LZW compression replaces repeated sequences with short references into a dictionary that is built while reading the input. Because the decoder can rebuild the same dictionary from the codes alone, the dictionary is never written to the file.

**Output format**

| Code | Size | Layout |
|------|------|--------|
| Literal byte | 1 byte | `0xxxxxxx` |
| Dictionary reference | 2 bytes | `1xxxxxxx xxxxxxxx` (15-bit index) |

The top bit of the first byte says whether a literal or a reference follows.

**Design decisions**

- **Only phrases of 3+ bytes become references.** A 2-byte reference to a 2-byte phrase saves nothing, so shorter phrases are written out as literals.
- **Reset-on-full.** When all 32,768 entries are used, the dictionary is cleared and rebuilt. The decoder does the same at exactly the same point, so the streams stay synchronised and the compressor adapts to the current part of the file.
- **Hash tables backed by a fixed node pool.** Entries are looked up by (prefix, next byte) through a chained hash table, so no dynamic allocation is needed. Using the same table in the decoder made decoding roughly 70 to 100 times faster on large inputs than a linear dictionary search.
- **Self-referencing codes.** The decoder handles a reference to the entry that is still being constructed (the classic LZW edge case) explicitly.

## Build and run

```bash
# Build
gcc -O2 -Wall -o lencode lencode.c
gcc -O2 -Wall -o ldecode ldecode.c

# Compress
./lencode input.txt compressed.lzw

# Decompress
./ldecode compressed.lzw restored.txt

# Verify the round trip
cmp input.txt restored.txt && echo "Identical"
```

## Testing

```bash
./test.sh
```

The script builds both programs, then compresses, decompresses and compares each of these inputs byte-for-byte with the original:

- an empty file and a single-byte file
- highly repetitive text
- C source code
- large ASCII input that fills the dictionary and forces resets

## Results

| Input | Original | Compressed | Compressed size |
|-------|----------|------------|-----------------|
| Highly repetitive text | 140,000 B | 2,771 B | 2.0% of original |
| Source code (concatenated, 1 MB) | 1,000,000 B | 381,254 B | 38.1% |
| License texts (concatenated) | 300,000 B | 127,334 B | 42.4% |

All of the above decompress back to identical files.

## Limitations

- **Plain ASCII text only.** The top bit of each byte is used as the literal/reference flag, so input bytes of `0x80` or above (binary files, non-ASCII UTF-8 text) are not supported and will not round-trip. Supporting them would need an escape mechanism.
- **Fixed 2-byte references.** Classic LZW grows the code width gradually, which compresses better on small files.

## Possible extensions

- Add an escape byte so the full 0–255 byte range is supported.
- Grow the code width gradually (9 to 15 bits) with bit-level packing.
- Add Huffman coding on top, working towards a DEFLATE-style compressor, and benchmark against `gzip`.

## What I learned

- Designing a compact binary format, and keeping an encoder and decoder in lockstep, including across dictionary resets.
- Handling the LZW edge case where a code refers to an entry the decoder hasn't finished building.
- Using a hash table with a fixed node pool for fast lookups without dynamic allocation, and seeing the decoder speed up dramatically once it used one.
- Testing a compressor properly with round trips and byte-for-byte comparison, including edge cases and dictionary resets.

## Author

**Nahian Khan**: Commerce (Information Systems) / Computer Science, UNSW
[LinkedIn](https://linkedin.com/in/nahian-khan-b4098525a)
