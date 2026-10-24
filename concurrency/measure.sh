#!/bin/bash
# Get the program name without the extension (if it is .out)
program_name=$(basename "$1" .out)

# Run the program and capture time and memory usage
/usr/bin/time -v ./$1 2> ${program_name}_usage.txt

# Extract the runtime (user + system time) and memory usage (max resident set size) from the output
runtime=$(grep -E "User time|System time" ${program_name}_usage.txt | awk '{sum += $4} END {print sum}')
memory=$(grep "Maximum resident set size" ${program_name}_usage.txt | awk '{print $6}')

# Output the results
echo "Total Runtime (seconds): $runtime"
echo "Max Memory Usage (KB): $memory"
