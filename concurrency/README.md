# concurrency

## LazyRW

If two process are available to be performed on the same file at some time, they are done at random if only one can happen at a time.

## Distributed Merge Sort

- Created p threads, split the files among the p threads.
- Performed Merge Sort on each thread.
- Then Merged each thread together 2 at a time.

### Pros

- Faster than regular merge sort as by a factor of p (nlogn/p)
- As number of threads is predefined memory overhead isnt too large

### Cons

- The memory and time used for creation of these threads is greater than the time saved for smaller number of files.
- Slower than using thread per merge operation however, however creating a thread per merge operation isnt feasible as it leads to a large number of threads.

### Execution analysis

#### Small

- n=50
- time(s): 0.0013
- memory usage(KB): 2048

#### Medium

- n=5000
- time(s): 0.0063
- memory usage(KB): 3968

#### Large

- n=50000
- time(s): 0.0541
- memory usage(KB): 22096

![Execution time for Merge Sort](TimeMerge.png)

We can see the slope decreases as no of files increases, this is as the time saved by parallel processing using threads makes up for the time taken in setting up the threads as number of files increases.
<br>
Memory Usage increases as we need larger arrays to merge sort a larger number of files.

## Distributed Count Sort

- Created p threads, split the files among the p threads.
- The specified column (ID, name, or timestamp) is hashed to generate integer keys, which will be used for sorting.
- Each thread independently counts the occurrences of each hashed value within its assigned portion.
- The threads then compute a prefix sum on their local counts. This determines the starting index for each unique key in the global sorted array.
- Using the calculated prefix sum, each thread distributes its portion of the data into a new global array. The elements are placed according to their hashed keys, ensuring they are in the correct position
- The threads then fill up the main sorted array based on their respective ranges. Each thread handles a segment from index i*(n/p) to (i+1)*(n/p), where n is the total number of elements.
- The algorithm may become inefficient if the total size of the data (p \* b) is significantly larger than the number of unique elements (n). This is particularly true when sorting strings or timestamps, where hashing can result in many collisions or sparse data distributions.

### Pros

- Reduces the time to O (n/p)

### Cons

- The algorithm may become inefficient if the total size of the data (p \* b) is significantly larger than the number of unique elements (n). This is particularly true when sorting strings or timestamps, where hashing can result in many collisions or sparse data distributions.
- large overhead costs, especially with large and complex datasets.

### Assumption

Assumed that file names are only 5 characters of a-z to make hashing easier.

### Execution analysis

#### Small

- n=5
- time(s): 1.4869
- memory usage(KB): 314112

#### Medium

- n=25
- time(s): 1.4754
- memory usage(KB): 313856

#### Large

- n=40
- time(s): 1.4763
- memory usage(KB): 314240

#### Explanation

- Max Memory is limited by the number of items/range of the field that need to be hashed, We have assumed that 4 \* 1e8 values can be allowed for each field.
- Overall memory and time usage can be lowered by dynamically allocating memory depending on range of values.

![Execution time for Merge Sort](TimeCount.png)
![Execution time for Merge Sort](bargraph_mem.png)

### Summary of Findings

The report compares the performance of **Distributed Merge Sort** and **Distributed Count Sort** algorithms when handling varying numbers of files. Here's a breakdown of the key findings:

#### 1. Distributed Merge Sort

- **Performance**:
  - This method showed significant performance improvements with increasing file counts. For smaller datasets (`n=50`), the overhead of creating multiple threads outweighed the benefits, leading to minimal speedup.
  - For larger datasets (`n=50000`), it achieved a substantial reduction in execution time due to parallel processing (`O(n log n / p)` complexity).
- **Memory Usage**:
  - As the number of files increased, the memory footprint also grew due to the larger arrays required for merge operations. However, the memory overhead was controlled by limiting the number of threads.
- **Optimization Potential**:
  - Optimizing thread management or dynamically adjusting the number of threads based on dataset size could further enhance performance, especially for smaller datasets.

#### 2. Distributed Count Sort

- **Performance**:
  - This method achieved a linear speedup (`O(n/p)`) by splitting the work across threads but struggled with smaller datasets.
  - Surprisingly, it showed a near-constant execution time (around 1.47 seconds) across all file sizes, which suggests that the overhead for counting and prefix sum operations dominated the execution time.
- **Memory Usage**:
  - The memory usage was extremely high (over 300MB), even for small datasets, due to the large counting arrays required for hashing fields like IDs, names, and timestamps. The performance did not scale well due to this significant memory overhead.
- **Optimization Potential**:
  - Potential optimizations include reducing memory usage by dynamically allocating memory based on the actual range of hashed values.
  - Additionally, implementing a more efficient hashing mechanism or using a different approach for sparse data distributions could improve speed and reduce overhead.

### Recommendations for Large Datasets

- **Merge Sort**:
  - Consider using adaptive thread management or hybrid approaches (e.g., switching to single-threaded sorting for smaller subarrays) to reduce thread overhead.
- **Count Sort**:
  - Implement dynamic memory allocation based on the range of unique values to reduce unnecessary memory usage.
  - Exploring alternative counting techniques or adaptive hashing strategies could also minimize collisions and improve efficiency.
