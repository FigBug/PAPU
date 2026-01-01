#!/bin/bash
set -e

cd "$(dirname "$0")"

# Read version from VERSION file
VERSION=$(cat VERSION | tr -d '[:space:]')

if [ -z "$VERSION" ]; then
    echo "Error: VERSION file is empty or not found"
    exit 1
fi

TAG="v${VERSION}"

echo "Creating tag: $TAG"

# Check if tag already exists
if git rev-parse "$TAG" >/dev/null 2>&1; then
    echo "Error: Tag $TAG already exists"
    exit 1
fi

# Create and push the tag
git tag -a "$TAG" -m "Release $VERSION"
git push origin "$TAG"

echo "Tag $TAG created and pushed successfully"
