import os
import sys
from pathlib import Path
import imageio.v2 as imageio  # avoid deprecation warning

def create_animation(exposure_dir):
    exposure_path = Path(exposure_dir).resolve()
    color_dir = exposure_path / 'processed' / 'color'
    
    if not color_dir.exists():
        print(f"Error: {color_dir} does not exist.")
        return

    image_files = sorted(
        [f for f in color_dir.iterdir() if f.suffix.lower() in ['.png', '.jpg', '.jpeg']],
        key=lambda f: f.name
    )

    if not image_files:
        print(f"No images found in {color_dir}")
        return

    images = []
    for file in image_files:
        try:
            images.append(imageio.imread(file))
        except Exception as e:
            print(f"Skipping {file.name}: {e}")

    if not images:
        print("No valid images to create animation.")
        return

    # Output in the main exposure directory
    animation_name = exposure_path.name + '_animation.gif'
    output_path = exposure_path / animation_name

    imageio.mimsave(output_path, images, duration=0.25)

    print(f"✓ Animation saved: {output_path}")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 create-animation.py /path/to/exposure-directory")
        sys.exit(1)

    create_animation(sys.argv[1])