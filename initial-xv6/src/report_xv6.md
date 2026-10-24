# LAZYFORK REPORT AND ANALYSIS

**Memory Efficiency**: Traditional fork implementations duplicate all memory pages from the parent process to the child, which can consume a large amount of memory, particularly for large processes. With Copy-On-Write (COW), both the parent and child processes initially share the same memory pages. New memory pages are only allocated when one of the processes attempts to modify them, significantly reducing memory usage.

**Performance Gains**: COW can lead to significant performance gains by avoiding immediate duplication of memory pages. This is particularly beneficial when the child process does not modify much data or quickly executes a new program after forking, such as in common shell operations. The reduced memory copying lightens the load on both the CPU and memory, enabling faster process creation and context switching.

**Optimized Page Fault Handling**:  Page faults, which occur when a process tries to write to a read-only page, trigger the system to create a copy of the page only when necessary. This "lazy" allocation approach optimizes resource usage by preventing the system from allocating memory prematurely, reducing unnecessary page duplication and enhancing overall performance.

## Further Optimization Areas for COW:

**Selective Page Sharing**: Some pages, such as those containing static data or read-only code segments, may not need to be duplicated and can be shared permanently. Fine-tuning COW to detect these types of pages and handle them differently could reduce page faults and improve efficiency

**Efficient Page Allocation**: Allocating new pages during a page fault and copying data can still be resource-intensive. Enhancing the speed of memory allocation, or maintaining a pool of pre-allocated pages for COW faults, could further minimize the time required to resolve page faults.

**Analysis**: For this we have created a system call to check and print the number of page faults which occurs. In lazytest test cases we can see that for simpletest we get single digit page faults and for threetest which is a complex test we get thousands of page faults. Additionally we have created a read only test case file called test_1. For read only test we should get zero page faults and while testing it is showed that we get zero page faults.

output for lazy test:<br>
$ lazytest <br>
simple: ok<br>
3<br>
simple: ok<br>
1<br>
three: ok<br>
18839<br>
three: ok<br>
18836<br>
three: ok<br>
18836<br>
file: ok<br>
ALL COW TESTS PASSED<br>

output for read only:<br>
test_1<br>
1100<br>
Read-only test passed<br>
Read-only test passed<br>
1100<br>

this output indicates zero page faults for read only
