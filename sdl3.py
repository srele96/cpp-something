import subprocess
import os
from pathlib import Path
import shutil
import sys

BASE_DIR = Path(__file__).resolve().parent

def copyOnLinux(src_dir, dest_dir, pattern="libSDL3.so*"):
	src_path = Path(src_dir)
	dest_path = Path(dest_dir)

	for file in src_path.glob(pattern):
		target_file = dest_dir / file.name
		try:
			if target_file.exists() or target_file.is_symlink():
				target_file.unlink()

			shutil.copy2(file, dest_path / file.name, follow_symlinks=False)
			print(f"Successfully copied: {file.name}")

		except PermissionError:
			print(f"Failed: Permission denied to write to {dest_dir}")
			sys.exit()
		except FileNotFoundError:
			print(f"Failed: Source file {file.name} disappeared during copy")
			sys.exit()
		except shutil.SameFileError:
			print(f"Notice: {file.name} is already at the destination.")
			sys.exit()
		except OSError as e:
			print(f"System error during copy of {file.name}: {e}")
			sys.exit()

def main():
	sdl_src_path = BASE_DIR / "SDL3"
	build_path = sdl_src_path / "build"

	if not build_path.exists():
		build_path.mkdir(exist_ok=True)

	generate_cmd = [
		"cmake",
		"-S", str(sdl_src_path),
		"-B", str(build_path)
	]

	print(f"Running command: {' '.join(generate_cmd)}")

	try:
		subprocess.run(generate_cmd, check=True)
	except subprocess.CalledProcessError as e:
		print(f"Command failed with return code {e.returncode}. Fix the problem manually.")
		sys.exit()

	build_cmd = [
		"cmake",
		"--build", str(build_path),
		"--parallel"
	]

	print(f"Running command: {' '.join(build_cmd)}")

	try:
		subprocess.run(build_cmd, check=True)
	except subprocess.CalledProcessError as e:
		print(f"Command failed with return code {e.returncode}. Fix the problem manually.")
		sys.exit()

	if sys.platform == "linux":
		copyOnLinux(build_path, BASE_DIR)
	else:
		print("Please add copy command for your system.")

if __name__ == "__main__":
	main()
