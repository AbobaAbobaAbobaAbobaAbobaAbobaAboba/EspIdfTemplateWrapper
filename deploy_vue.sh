#!/bin/bash

# Check if correct number of arguments is provided
if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <source_directory> <destination_directory>"
    exit 1
fi

SOURCE_DIR="$1"
DEST_DIR="$2"

# Check if source directory exists
if [ ! -d "$SOURCE_DIR" ]; then
    echo "Error: Source directory '$SOURCE_DIR' does not exist"
    exit 1
fi

# Check if destination directory exists, create if it doesn't
if [ ! -d "$DEST_DIR" ]; then
    echo "Creating destination directory '$DEST_DIR'"
    mkdir -p "$DEST_DIR"
fi

# Clear destination directory
echo "Clearing destination directory '$DEST_DIR'"
rm -rf "$DEST_DIR"/*

# Copy contents from source to destination
echo "Copying contents from '$SOURCE_DIR' to '$DEST_DIR'"
cp -r "$SOURCE_DIR"/* "$DEST_DIR"

# Check if copy was successful
if [ $? -eq 0 ]; then
    echo "Copy operation completed successfully"
else
    echo "Error: Copy operation failed"
    exit 1
fi
