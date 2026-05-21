# Notes on ZIP format

Here's what I learned while exploring the ZIP format:

## 0. ZIP format

From [ZIP specification](standard.txt):

```plaintext
[local file header 1]
[file data 1]
[data descriptor 1]
. 
.
.
[local file header n]
[file data n]
[data descriptor n]
[archive decryption header] (EFS)
[archive extra data record] (EFS)
[central directory]
[zip64 end of central directory record]
[zip64 end of central directory locator] 
[end of central directory record]
```

## 1. We can stream ZIP local file data

1. Start reading ZIP archive
2. Identify the header type
3. If header type is
[LOC](https://en.wikipedia.org/wiki/ZIP_(file_format)#Local_file_header),
decompress & compute CRC32 as you read.
4. In the end compare computed CRC32 with the one from the header 

## 2. We can read files concurrently

1. Locate central directory with the help of [EOCD
   header](https://en.wikipedia.org/wiki/ZIP_(file_format)#End_of_central_directory_record_(EOCD))
2. Set `n` cursors at the corresponding
   [LOC](https://en.wikipedia.org/wiki/ZIP_(file_format)#Local_file_header)
   offset, where `n` is the "Number of central directory records on this disk"
   from [EOCD
   header](https://en.wikipedia.org/wiki/ZIP_(file_format)#End_of_central_directory_record_(EOCD))
   (you probably need unique file descriptor for each cursor)
3. For each cursor spawn thread (or use another mechanism) & read concurrently

## Some good music

Eminem - Shake That

