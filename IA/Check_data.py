import os
from pathlib import Path

directories_to_scan = [
    Path("C:\\Users\\thiba\\OneDrive\\Bureau\\AI_Project\\Rotate"),
    Path("C:\\Users\\thiba\\OneDrive\\Bureau\\AI_Project\\Split") 
]

IMAGE_EXTENSIONS = ('.jpg', '.jpeg', '.png')


def count_all_images(root_dir: Path):

    if not root_dir.exists():
        print(f" Error: The directory '{root_dir.resolve()}' does not exist.")
        return 0

    total_count = 0

    for _, _, filenames in os.walk(root_dir):
        image_files = [f for f in filenames if f.lower().endswith(IMAGE_EXTENSIONS)]
        total_count += len(image_files)

    return total_count

if __name__ == "__main__":

    for folder in directories_to_scan:
        image_count = count_all_images(folder)

        print("\n" + "="*50)
        print(f" Folder : {folder.name}")
        print("="*50)
        print(f"Number of images : **{image_count}**")
