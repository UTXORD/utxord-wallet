#!/usr/bin/env bash

TAG_NAME="$1"
TAG_MESSAGE="$2"

if [ "$#" -ne 2 ]; then
    echo "Usage:"
    echo "    $0 <tag-label> <tag-message>"
    exit
fi

git push --delete origin "${TAG_NAME}" ; git tag -d "${TAG_NAME}" ; \
git tag -a -m "$TAG_MESSAGE" "${TAG_NAME}"; git push --follow-tags