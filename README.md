# malloc Assignment

## Description

built my own implementation of malloc and free. Implemented a library that interacts with the operating system to perform heap management on behalf of a user process as demonstrated in class. 

### AI

Used Chatgpt O1 Preview Model and left its code in malloc_ai.c then implemented it in malloc.c
Code must pass the following tests: ffnf, bfwf, test1, test2, test3, and test4 and it must be invoked in the exam manner as noted below. 

## Building and Running the Code

The code compiles into four shared libraries and six test programs. To build the code, change to your top level assignment directory and type:
```
make
```
Once you have the library, you can use it to override the existing malloc by using
LD_PRELOAD. The following example shows running the ffnf test using the First Fit shared library:
```
$ env LD_PRELOAD=lib/libmalloc-ff.so tests/ffnf
```

To run the other heap management schemes replace libmalloc-ff.so with the appropriate library:
```
Best-Fit: libmalloc-bf.so
First-Fit: libmalloc-ff.so
Next-Fit: libmalloc-nf.so
Worst-Fit: libmalloc-wf.so
```
## Program Requirements

1. Implement splitting and coalescing of free blocks. If two free blocks are adjacent then
combine them. If a free block is larger than the requested size then split the block into two.
2. Implement three additional heap management strategies: Next Fit, Worst Fit, Best Fit (First
Fit has already been implemented for you).
3. Counters exist in the code for tracking of the following events:

* Number of times the user calls malloc successfully
* Number of times the user calls free successfully
* Number of times we reuse an existing block
* Number of times we request a new block
* Number of times we split a block
* Number of times we coalesce blocks
* Number blocks in free list
* Total amount of memory requested
* Maximum size of the heap

The code will print the statistics (THESE STATS ARE FAKE) upon exit and should look like similar to:
```
mallocs: 8
frees: 8
reuses: 1
grows: 5
splits: 1
coalesces: 1
blocks: 5
requested: 7298
max heap: 4096
```

4. Implement realloc and calloc.

5.  **Design and develop a suite of programs and capture execution time for your four implementations of the arena allocator and compare their performance against the same programs using malloc()**. suite of tests evaluate the programs based on:
* Performance
* Relative comparision of number of splits and heap growth
* Heap fragmentation
* Max heap size
6. A report generated with the findings:
* Executive summary
* Description of the algorithms implemented.
* Test implementation
* Test results for all five candidates ( malloc and your four algorithm implementations )
* Explanation and interpretation of the results including any anomalies in the test results.
* Conclusion on AI performance.
1. Did the AI assistant help?
2. Did it hurt?
3. Where did the AI tool excel?
4. Where did it fail?
5. Do you feel you learned more, less, or the same if you had implemented it fully on your own?
6. If you leaned more, what was do you think you learned more of?
The report must be submitted as a PDF file.  Any other formats will result in a grade of 0 for the report.
The rubric for grading your report is contained within the rubric directory.
