import os
import sys

readme_path = "../README.md"

if __name__ == "__main__":
    print("start...")
    with open(readme_path, 'r', encoding="utf-8") as f:
        data = f.readlines()
        for line in data:
            print(line)