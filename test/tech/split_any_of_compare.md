# shrinking boost dependency proof

`split_any_of_compare.cpp` implements a comparison of boost::split_any_of with a custom split_any_of function.
The code tests both functions on the same input and compares their outputs to ensure they behave identically.
Then we can remove boost::algorithm from our codebase with confidence, lightening our dependencies and build times.

To run in isolation:

    podman build -f split_any_of_compare.Containerfile -t fire2a-split-any-of-test .
    podman run --rm fire2a-split-any-of-test

Not isolated:

    g++ -std=c++17 -I./include split_any_of_compare.cpp -o split_any_of_compare
    ./split_any_of_compare
