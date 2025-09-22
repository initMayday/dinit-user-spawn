#!/bin/bash
# Script to generate PKGBUILD files for different branches

set -euo pipefail

#> Ensure correct working directory
cd "$(dirname "${BASH_SOURCE[0]}")"

branches=("dev" "master")

for branch in "${branches[@]}"; do
    if [[ "$branch" == "template" ]]; then
        echo "Woah there buddy! We aren't overwriting the template directory!"
        continue
    fi

    rm -rf "$branch"
    mkdir "$branch"

    for script in template/*; do
        filename=$(basename "$script")
        sed "s/BRANCHTYPE/$branch/g" "$script" > "$branch/$filename"
    done
done

echo "Successfully generated branch packages"

