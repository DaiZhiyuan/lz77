# Overview

lz77 (MIT license) is an ANSI C/C90 implementation of Lempel-Ziv 77 algorithm of lossless data compression.

# Lossless compression benchmark corpora

introduction to the three main lossless compression benchmark corpora：
- Canterbury: This is one of the most commonly used corpora for benchmarking lossless compression algorithms. It was created by the University
of Canterbury. It contains 11 files of different types, with a total size of around 3.2 MB. The files cover texts, source code, literary works
and more, allowing comprehensive evaluation of lossless compression performance.
- Silesia: This larger corpus was created by the Silesian University of Technology in Poland. It contains 205 files across 5 different partitio
ns, with more diverse file types and a total size over 200 MB. It focuses on evaluating lossless compression on large files.
- Enwik9: This corpus is based on the English Wikipedia. It contains a single, approximately 1 GB sized text file. It is used to test the compr
ession ratio and throughput of lossless data compression algorithms when dealing with extremely large inputs.

In summary, the three corpora cover small to large file scenarios, and are all commonly used benchmarks for evaluating lossless compression alg
orithms.

# Round-trip check

check steps:
1. read the content of the file.
2. compress it first using `lz77_compress` API compressor.
3. decompress the output with `lz77_decompress` API decompressor.
4. compare the result with the original file content.

```
● ./test_lz77
Test round-trip for lz77

   canterbury/alice29.txt     152089  ->      85455  (56.19%)
  canterbury/asyoulik.txt     125179  ->      74529  (59.54%)
       canterbury/cp.html      24603  ->      12134  (49.32%)
      canterbury/fields.c      11150  ->       4734  (42.46%)
   canterbury/grammar.lsp       3721  ->       1782  (47.89%)
   canterbury/kennedy.xls    1029744  ->     405371  (39.37%)
    canterbury/lcet10.txt     426754  ->     233287  (54.67%)
  canterbury/plrabn12.txt     481861  ->     300518  (62.37%)
          canterbury/ptt5     513216  ->      81298  (15.84%)
           canterbury/sum      38240  ->      20520  (53.66%)
       canterbury/xargs.1       4227  ->       2471  (58.46%)
          silesia/dickens   10192446  ->    6060465  (59.46%)
          silesia/mozilla   51220480  ->   26987850  (52.69%)
               silesia/mr    9970564  ->    5073702  (50.89%)
              silesia/nci   33553445  ->    6924012  (20.64%)
          silesia/ooffice    6152192  ->    4274280  (69.48%)
             silesia/osdb   10085684  ->    6639678  (65.83%)
          silesia/reymont    6627202  ->    3303967  (49.85%)
            silesia/samba   21606400  ->    8341626  (38.61%)
              silesia/sao    7251944  ->    6387784  (88.08%)
          silesia/webster   41458703  ->   20276326  (48.91%)
            silesia/x-ray    8474240  ->    8197558  (96.74%)
              silesia/xml    5345280  ->    1387943  (25.97%)
         enwik/enwik8.txt  100000000  ->   55578364  (55.58%)
```

# Phyzip Compression and Decompression Test Cases

Prepare a variety of input data samples:
- enwik/enwik8.txt

md5sum: a1fa5ffddb56f4953e226637dabbb36a
size: 100M

## Compression
```
● phy_zip
phyzip: high-speed file compression tool

Usage: phyzip [options] input-file output-file

Options:
  -v    show program version

● time phy_zip /root/lz77/dataset/enwik/enwik8.txt enwik8.lz
real    0m1.029s

● du -sh enwik8.lz
54M     enwik8.lz
```

## Decompression
```
● phy_unzip
phyunzip: uncompress phyzip archive

Usage: phyunzip archive-file

● phy_unzip enwik8.lz

● pwd
/tmp

● md5sum enwik8.txt
a1fa5ffddb56f4953e226637dabbb36a  enwik8.txt
```
