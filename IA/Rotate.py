# Import libraries for image processing
import os
from PIL import Image, ExifTags
import shutil

# Source directory containing original images
input_root_dir = 'C:\\Users\\thiba\\OneDrive\\Bureau\\AI_Project\\Faces' 
# Output directory for corrected images
output_root_dir = 'C:\\Users\\thiba\\OneDrive\\Bureau\\AI_Project\\Rotate' 

# Apply EXIF orientation correction to images
def rotate_and_save(image_path, output_path):

    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    
    try:

        img = Image.open(image_path)
        img_rotated = False

        # Find EXIF orientation tag
        orientation_key = next((tag for tag, value in ExifTags.TAGS.items() if value == 'Orientation'), None)
        
        exif = img._getexif()
        orientation = exif.get(orientation_key) if exif and orientation_key else None

        # Apply rotation based on EXIF orientation value
        if orientation and orientation != 1:

            print(f"Orientation found ({orientation}) for {os.path.basename(image_path)}. Applying correction.")
            
            # Apply correct transformation based on EXIF orientation value (1-8)
            if orientation == 2:
                img = img.transpose(Image.FLIP_LEFT_RIGHT)
            elif orientation == 3:
                img = img.transpose(Image.ROTATE_180)
            elif orientation == 4:
                img = img.transpose(Image.FLIP_TOP_BOTTOM)
            elif orientation == 5:
                img = img.transpose(Image.ROTATE_270).transpose(Image.FLIP_LEFT_RIGHT)
            elif orientation == 6:
                img = img.transpose(Image.ROTATE_270)
            elif orientation == 7:
                img = img.transpose(Image.ROTATE_90).transpose(Image.FLIP_LEFT_RIGHT)
            elif orientation == 8:
                img = img.transpose(Image.ROTATE_90)
            
            img_rotated = True
        else:
            print(f"No corrective orientation needed for {os.path.basename(image_path)} (missing EXIF or 'Top-Left').")

        # Save corrected image or copy original
        if img_rotated:
            img.save(output_path)
            print(f"Saved the CORRECTED image to {output_path}")
        else:
            # No rotation needed, just copy the file
            img.close()
            shutil.copy2(image_path, output_path)
            print(f"Copied the ORIGINAL image without modification to {output_path}")

    except Exception as e:

        # Handle errors by copying original file
        print(f"ERROR or unsupported file for PIL: {os.path.basename(image_path)}: {e}. Copying the original.")
        try:
            shutil.copy2(image_path, output_path)
        except Exception as copy_e:
            print(f"Final copy failed for {os.path.basename(image_path)}: {copy_e}")

def main():
    # Validate input directory
    if not os.path.exists(input_root_dir):
        print(f"Error: Input directory '{input_root_dir}' does not exist.")
        print("Check the path.")
        return

    # Check/create output directory
    if os.path.exists(output_root_dir):
        print(f"Warning: Output directory '{output_root_dir}' already exists.")
        print("Existing files will be overwritten.")
    else:
        os.makedirs(output_root_dir, exist_ok=True)
        print(f"Output directory created: {output_root_dir}")

    print(f"\nStarting image processing in '{input_root_dir}'...")
    processed_count = 0

    # Walk through all subdirectories and process JPG/JPEG files
    for dirpath, dirnames, filenames in os.walk(input_root_dir):
        for filename in filenames:

            # Process only JPG/JPEG images
            if filename.lower().endswith(('.jpg', '.jpeg')):
                image_path = os.path.join(dirpath, filename)
            
                # Maintain directory structure in output
                relative_path = os.path.relpath(image_path, input_root_dir)
                output_image_path = os.path.join(output_root_dir, relative_path)

                rotate_and_save(image_path, output_image_path)
                processed_count += 1
    
    print(f"\n Processing complete. {processed_count} JPG/JPEG images processed or copied.")
    print(f"The corrected/copied images were saved in: '{output_root_dir}'")

if __name__ == "__main__":
    main()
