import os
import shutil
import re
from pathlib import Path

# Source and destination directories
src_dir = Path("C:\\Users\\thiba\\OneDrive\\Bureau\\AI_Project\\Rotate")
dst_dir = Path("C:\\Users\\thiba\\OneDrive\\Bureau\\AI_Project\\Split")

# Create Training and Validation subdirectories
train_dir = dst_dir / "Training"
val_dir = dst_dir / "Validation"

train_dir.mkdir(parents=True, exist_ok=True)
val_dir.mkdir(parents=True, exist_ok=True)

# Extract class label from filename (e.g., "123-5.jpg" -> "123")
def get_class_prefix(filename):
    match = re.match(r"(\d+)", filename)
    return match.group(1) if match else None

print(f"Source folder : {src_dir.resolve()}")
print(f"Destination folder : {dst_dir.resolve()}\n")
print("Starting sorting and copying...")

# Process each image file in source directory
for file in os.listdir(src_dir):

    # Skip non-image files
    if not file.lower().endswith((".jpg", ".jpeg", ".png")):
        continue

    src_path = src_dir / file
    prefix = get_class_prefix(file)

    if prefix is None:
        print(f" File ignored (no numeric prefix) : {file}")
        continue

    dest_folder = None
    
    # Training set: images ending in -1 to -6
    if re.search(r"-[1-6]\.(jpg|jpeg|png)$", file, re.IGNORECASE):
        dest_folder = train_dir / prefix
        target_set = "Training"
    
    # Validation set: images ending in -7
    elif re.search(r"-7\.(jpg|jpeg|png)$", file, re.IGNORECASE):
        dest_folder = val_dir / prefix
        target_set = "Validation"
    
    else:
        continue

    # Create class folder and copy image
    dest_folder.mkdir(parents=True, exist_ok=True)
    shutil.copy2(src_path, dest_folder / file)

print("\n Split completed successfully!")
print(f"Final directory: {dst_dir.resolve()}")
