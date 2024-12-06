set -e

function main() {
    if [[ $# -lt 2 ]]; then
        printf "\nMissing parameters\n"
    else Cell2Fire/Cell2Fire $@
    fi
}

main "$@"
