#!/bin/bash
# benchmark_sweep.sh
# Sweep iterations from 1000 to 5000 in steps of 500
# Run each test 6 times, keep last 5 results

OUTFILE=results.csv
FUSE_PATH="/mnt/rtfs/foo"
NFS_PATH="/mnt/nfsshare/foo"

# Header
echo "system,op,iterations,trial,rwtest_time_ms,real_time_s" > $OUTFILE

# Function to run one test
run_test() {
    local SYSTEM=$1
    local OP=$2
    local FILE=$3
    local ITERATIONS=$4
    local TRIAL=$5

    OUTPUT=$( { /usr/bin/time -p ./rwtest $OP $ITERATIONS $FILE; } 2>&1 )

    RWTEST_MS=$(echo "$OUTPUT" | grep "Total" | awk '{print $(NF-1)}')
    REAL_S=$(echo "$OUTPUT" | grep "^real" | awk '{print $2}')

    echo "$SYSTEM,$OP,$ITERATIONS,$TRIAL,$RWTEST_MS,$REAL_S" >> $OUTFILE
}

# Main loop: iterations from 1000 to 5000 in steps of 500
for ITER in $(seq 1000 500 5000); do
    for SYS in FUSE NFS; do
        if [ "$SYS" = "FUSE" ]; then
            FILE=$FUSE_PATH
        else
            FILE=$NFS_PATH
        fi

        for OP in write read; do
            echo "Running $SYS $OP $ITER iterations..."
            # Run 6 times, skip first trial
            for TRIAL in $(seq 1 6); do
                run_test $SYS $OP $FILE $ITER $TRIAL
            done
        done
    done
done

# Keep only last 5 trials per (system,op,iterations)
# (Filter out trial=1)
awk -F, 'NR==1 || $4 > 1' $OUTFILE > tmp && mv tmp $OUTFILE

echo "Done. Results saved to $OUTFILE"
